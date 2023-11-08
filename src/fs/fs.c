/**
 * @file fs.c
 * @author Thomas Boos
 * @brief file system
 * @version 0.1
 * @date 2023-09-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "tdefs.h"

#include "fs.h"
#include "inode.h"
#include "utils.h"
#include "buf.h"
#include "pc.h"


filetab_t filetab[MAXFILETAB];  ///< file table

void init_fs()
{
  mset(filetab, 0, sizeof(filetab));
}


int getftabent(iinode_t* ii)
{
  ASSERT(ii != NULL);
  for (int i = 0; i < MAXFILETAB; i++) {
    if (filetab[i].inode == NULL) {
      filetab[i].inode = ii;
      filetab[i].refs = 1;
      return i;
    }
  }
  return -1;
}


void putftabent(int f)
{
  ASSERT(f >= 0 && f < MAXFILETAB);
  ASSERT(filetab[f].inode != NULL);
  filetab[f].refs--;
  if (filetab[f].refs == 0) {
    iput(filetab[f].inode);
    filetab[f].inode = NULL;
    /// @todo potentional race condition if another process is waiting for a free filetab entry
  }
} 

int freefdesc(void)
{
  for (int i = 0; i < MAXOPENFILES; i++) {
    if (active->u->fdesc[i].ftabent == NULL) {
      return i;
    }
  }
  return -1;
}


const char *basename(const char *path)
{
  ASSERT(path);
  const char *p = path + snlen(path, MAXPATH);
  while (p > path && *p != '/')
    p--;
  if (*p == '/')
    p++;
  return p;
}


void unlinki(iinode_t *ipdir, const char *bname)
{
  ASSERT(ipdir);
  ASSERT(bname);
  ASSERT(ipdir->dinode.ftype == DIRECTORY);
  ASSERT(ipdir->dinode.fsize % sizeof(dirent_t) == 0);
  int n = ipdir->dinode.fsize / sizeof(dirent_t);
  int bl = snlen(bname, sizeof(DIRNAMEENTRY));
  for (int i = 0; i < n; i++) {
    bmap_t b = bmap(ipdir, i * sizeof(dirent_t));
    ASSERT(b.fsblock > 0);
    bhead_t *bh = bread(LDEVFROMINODE(ipdir), b.fsblock);
    dirent_t *de = (dirent_t *)&bh->buf->mem[b.offblock];
    if (sncmp(de->name, bname, bl) == 0) {
      iinode_t *ii = iget(ipdir->fs, de->inum);
      if (ii == NULL) {
        brelse(bh);
        return;
      }
      while(ii->locked) {
        waitfor(INODELOCKED);
      }
      ii->locked = true;
      ii->dinode.nlinks--;
      ii->modified = true;
      ii->locked = false;
      wakeall(INODELOCKED);
      iput(ii);
      de->inum = 0;
      bh->dwrite = true;
      bwrite(bh);
      brelse(bh);
      break;
    }
    brelse(bh);
  }
}



void unlink(const char *path)
{
  ASSERT(path != NULL);
  namei_t in = namei(path);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return;
  }
  if (in.i->dinode.ftype == DIRECTORY) {
    /// @todo error is directory
    iput(in.i);
    return;
  }
  iput(in.i);
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return;   // error already set by iget
  ASSERT(pi->dinode.ftype == DIRECTORY);
  unlinki(pi, basename(path));
  iput(pi);
}



void linki(iinode_t *ii, const char *newpath)
{
  ASSERT(ii);
  ASSERT(newpath);

  namei_t in = namei(newpath);
  if (in.i != NULL) {
    iput(in.i);
    /// @todo error link already exists
    return;
  }
  if (in.p == 0)    // parent dir not found
    return;   // error already set by namei
  if (ii->fs != in.fs) {
    /// @todo error different file systems
    return;
  }
  iinode_t *pi = iget(in.fs, in.p);
  ASSERT(pi != NULL);
  ASSERT(pi->dinode.ftype == DIRECTORY);
  ASSERT(pi->dinode.fsize % sizeof(dirent_t) == 0);
  int n = pi->dinode.fsize / sizeof(dirent_t);
  for (int i = 0; i <= n; i++) {  // find free entry, therefore <= n, if no unused entry found, i == n and bmap will allocate a new block if necessary
    bmap_t b = bmap(pi, i * sizeof(dirent_t));
    if (b.fsblock == 0) {  // no new blocks left on device
      iput(pi);
      return;
    }
    bhead_t *bh = bread(LDEVFROMINODE(pi), b.fsblock);
    dirent_t *de = (dirent_t *)&bh->buf->mem[b.offblock];
    if (de->inum == 0) {
      de->inum = ii->inum;
      sncpy(de->name, basename(newpath), sizeof(DIRNAMEENTRY));
      while (ii->locked) {
        waitfor(INODELOCKED);
      }
      ii->locked = true;      
      ii->dinode.nlinks++;
      ii->modified = true;
      ii->locked = false;
      wakeall(INODELOCKED);
      bh->dwrite = true;
      bwrite(bh);
      brelse(bh);
      if (i == n) { // new directory entry
        while(pi->locked) {
          waitfor(INODELOCKED);
        }
        pi->locked = true;
        pi->dinode.fsize += sizeof(dirent_t);
        pi->modified = true;
        pi->locked = false;
        wakeall(INODELOCKED);
      }
      /// @todo reset error if necessary
      break;
    }
    brelse(bh);
  }
  iput(pi);
}



void link(const char *path, const char *newpath)
{
  ASSERT(path != NULL);
  ASSERT(newpath != NULL);
  namei_t in = namei(path);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return;   // error already set by namei
  }
  if (in.i->dinode.ftype == DIRECTORY) {
    /// @todo error is directory
    iput(in.i);
    return;
  }
  
  linki(in.i, newpath);
  iput(in.i);
}



void mknode(const char *path, ftype_t ftype, fmode_t fmode)
{
  ASSERT(path != NULL);
  ASSERT(ftype >= REGULAR && ftype <= FIFO);
  namei_t in = namei(path);
  if (in.i != NULL) {
    iput(in.i);
    /// @todo error link already exists
    return;
  }
  if (in.p == 0)    // parent dir not found
    return;   // error already set by namei
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return;   // error already set by iget

  iinode_t *ii = ialloc(in.fs, ftype, fmode);
  if (ii == NULL) {
    iput(pi);
    return;   // error already set by ialloc
  }

  linki(ii, path);
  iput(ii);
  iput(pi);
}



void mkdir(const char *path, fmode_t fmode)
{
  ASSERT(path != NULL);
  mknode(path, DIRECTORY, fmode);
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return;
  }
  if (in.p == 0)    // parent dir not found
    return;   // error already set by namei
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return;   // error already set by iget

  bmap_t b = bmap(in.i, 0);
  if (b.fsblock == 0) {  // no new blocks left on device
    unlinki(pi, basename(path));
    iput(in.i);
    iput(pi);
    return;
  }
  bhead_t *bh = bread(LDEVFROMINODE(in.i), b.fsblock);
  dirent_t *de = (dirent_t *)bh->buf->mem;
  de[0].inum = in.i->inum;
  sncpy(de[0].name, ".", sizeof(DIRNAMEENTRY));
  de[1].inum = pi->inum;
  sncpy(de[1].name, "..", sizeof(DIRNAMEENTRY));
  while(in.i->locked) {
    waitfor(INODELOCKED);
  }
  in.i->locked = true;
  in.i->dinode.fsize = 2 * sizeof(dirent_t);
  in.i->dinode.nlinks++;
  in.i->modified = true;
  in.i->locked = false;
  wakeall(INODELOCKED);
  while(pi->locked) {
    waitfor(INODELOCKED);
  }
  pi->locked = true;
  pi->dinode.nlinks++;
  pi->modified = true;
  pi->locked = false;
  wakeall(INODELOCKED);
  bh->dwrite = true;
  bwrite(bh);
  brelse(bh);
  iput(in.i);
  iput(pi);
}



void rmdir(const char *path)
{
  ASSERT(path != NULL);
  namei_t in = namei(path);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return;   // error already set by namei
  }
  if (in.i->dinode.ftype != DIRECTORY) {
    /// @todo error is not directory
    iput(in.i);
    return;
  }
  if (in.i->dinode.nlinks > 3) {
    /// @todo error directory not empty
    iput(in.i);
    return;
  }
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL) {
    iput(in.i);
    return;   // error already set by iget
  }
  unlinki(in.i, ".");
  unlinki(in.i, "..");
  iput(in.i);
  unlinki(pi, basename(path));
  iput(pi);
}



int open(const char *fname, omode_t omode, fmode_t fmode)
{
  ASSERT(fname != NULL);
  ASSERT(omode >= OREAD && omode <= OSYNC);
  int fdesc = freefdesc();
  if (fdesc < 0) {
    /// @todo error no free filetab entry
    return -1;
  }
  namei_t in = namei(fname);
  if (in.i == NULL) {
    if (omode & OCREATE) {
      mknode(fname, REGULAR, fmode);
      in = namei(fname);
      if (in.i == NULL) {
        /// @todo error link does not exists
        return -1;
      }
    } else {
      iput(in.i);
      return -1;   // error already set by namei
    }
  }
  if (in.i->dinode.ftype == DIRECTORY) {
    /// @todo error is directory
    iput(in.i);
    return -1;
  }
  int f = getftabent(in.i);
  if (f < 0) {
    iput(in.i);
    /// @todo error no free filetab entry
    /// @todo remove node if created
    return -1;
  }
  if (in.i->dinode.ftype == REGULAR) {
    if (omode & OTRUNC) {
      while(in.i->locked) {
        waitfor(INODELOCKED);
      }
      in.i->locked = true;
      free_all_blocks(in.i);
      in.i->dinode.fsize = 0;
      in.i->modified = true;
      in.i->locked = false;
      wakeall(INODELOCKED);
    }
    if (omode & OAPPEND) {
      filetab[f].offset = in.i->dinode.fsize;
    } else {
      filetab[f].offset = 0;
    }
  }
  active->u->fdesc[fdesc].ftabent = &filetab[f];
  active->u->fdesc[fdesc].omode = omode;
  return fdesc;
}



void close(int fdesc)
{
  ASSERT(fdesc >= 0 && fdesc < MAXOPENFILES);
  ASSERT(active->u->fdesc[fdesc].ftabent != NULL);
  putftabent(fdesc);
  active->u->fdesc[fdesc].ftabent = NULL;
}



int write(int fdesc, byte_t *buf, fsize_t nbytes)
{
  int written = 0;
  ASSERT(active->u->fdesc[fdesc].ftabent != NULL);
  if (fdesc < 0 || fdesc >= MAXOPENFILES) {
    /// @todo error invalid file descriptor
    return -1;
  }
  if (buf == NULL) {
    /// @todo error invalid buffer
    return -1;
  }
  if (!(active->u->fdesc[fdesc].omode & OWRITE)) {
    /// @todo error file not open for writing
    return -1;
  }
  iinode_t *ii = active->u->fdesc[fdesc].ftabent->inode;
  switch(ii->dinode.ftype) {
    case REGULAR:
      while(ii->locked) {
        waitfor(INODELOCKED);
      }
      ii->locked = true;
      while(nbytes) {
        bmap_t b = bmap(ii, active->u->fdesc[fdesc].ftabent->offset);
        if (b.fsblock == 0) {  // no new blocks left on device
          ii->locked = false;
          wakeall(INODELOCKED);
          return written;
        }
        bhead_t *bh = bread(LDEVFROMINODE(ii), b.fsblock);
        sizem_t n = MIN(nbytes, b.nbytesleft);
        mcpy(&bh->buf->mem[b.offblock], buf, n);
        bh->dwrite = !(active->u->fdesc[fdesc].omode & OSYNC);
        bwrite(bh);
        brelse(bh);
        nbytes -= n;
        buf += n;
        written += n;
        active->u->fdesc[fdesc].ftabent->offset += n;
        if (active->u->fdesc[fdesc].ftabent->offset > ii->dinode.fsize) {
          ii->dinode.fsize = active->u->fdesc[fdesc].ftabent->offset;
          ii->modified = true;
        }
      }
      ii->modified = true;
      ii->locked = false;
      wakeall(INODELOCKED);
      return written;
    case CHARACTER:
      /// @todo error is character device
      return -1;
    case BLOCK:
      /// @todo error is block device
      return -1;
    case FIFO:
      /// @todo error fifo
      return -1;
    default:
      /// @todo error invalid file type
      return -1;
  }
}



int read(int fdesc, byte_t *buf, fsize_t nbytes)
{
  int read = 0;
  ASSERT(active->u->fdesc[fdesc].ftabent != NULL);
  if (fdesc < 0 || fdesc >= MAXOPENFILES) {
    /// @todo error invalid file descriptor
    return -1;
  }
  if (buf == NULL) {
    /// @todo error invalid buffer
    return -1;
  }
  if (!(active->u->fdesc[fdesc].omode & OREAD)) {
    /// @todo error file not open for reading
    return -1;
  }
  iinode_t *ii = active->u->fdesc[fdesc].ftabent->inode;
  sizem_t maxleft = ii->dinode.fsize - active->u->fdesc[fdesc].ftabent->offset;
  nbytes = MIN(nbytes, maxleft);
  switch(ii->dinode.ftype) {
    case REGULAR:
      while(ii->locked) {
        waitfor(INODELOCKED);
      }
      ii->locked = true;
      while(nbytes) {
        bmap_t b = bmap(ii, active->u->fdesc[fdesc].ftabent->offset);
        if (b.fsblock == 0) {  // no new blocks left on device
          ii->locked = false;
          wakeall(INODELOCKED);
          return read;
        }
        bhead_t *bh = bread(LDEVFROMINODE(ii), b.fsblock);
        sizem_t n = MIN(nbytes, b.nbytesleft);
        mcpy(buf, &bh->buf->mem[b.offblock], n);
        brelse(bh);
        nbytes -= n;
        buf += n;
        read += n;
        active->u->fdesc[fdesc].ftabent->offset += n;
      }
      ii->locked = false;
      wakeall(INODELOCKED);
      return read;
    case CHARACTER:
      /// @todo error is character device
      return -1;
    case BLOCK:
      /// @todo error is block device
      return -1;
    case FIFO:
      /// @todo error fifo
      return -1;
    default:
      /// @todo error invalid file type
      return -1;
  }
}

