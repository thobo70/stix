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
#include <stdio.h>

#include "fs.h"
#include "blocks.h"
#include "inode.h"
#include "utils.h"
#include "buf.h"
#include "pc.h"



typedef struct {    ///< @see igetpdir
  iinode_t *pi;     ///< parent inode
  ninode_t child;   ///< child inode number
} pdir_t;



filetab_t filetab[MAXFILETAB];  ///< file table



/**
 * @brief initialize file system table
 * 
 */
void init_fs()
{
  mset(filetab, 0, sizeof(filetab));
}



/**
 * @brief get empty file system entry and assign inode
 * 
 * @param ii inode
 * @return int file system number
 */
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



/**
 * @brief put file system entry
 * 
 * @param f file system entry
 */
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



/**
 * @brief get free file descriptor
 * 
 * @return int  file descriptor
 */
int freefdesc(void)
{
  for (int i = 0; i < MAXOPENFILES; i++) {
    if (active->u->fdesc[i].ftabent == NULL) {
      return i;
    }
  }
  return -1;
}



/**
 * @brief retrieve basename from path
 * 
 * @param path          path
 * @return const char*  basename
 */
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



/**
 * @brief unlink directory entry from directory with inode ipdir
 * 
 * @param ipdir   inode of directory
 * @param bname   basename of directory entry
 * @return int    0 on success, -1 on error
 */
int unlinki(iinode_t *ipdir, const char *bname)
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
        return -1;   // error already set by iget
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
  return 0;
}



/**
 * @brief unlink file path
 * 
 * @param path  path to file
 * @return int  0 on success, -1 on error
 */
int unlink(const char *path)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  }
  namei_t in = namei(path);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return -1;   // error already set by namei
  }
  if (in.i->dinode.ftype == DIRECTORY) {
    /// @todo error is directory
    iput(in.i);
    return -1;
  }
  iput(in.i);
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return -1;   // error already set by iget
  ASSERT(pi->dinode.ftype == DIRECTORY);
  int rtn = unlinki(pi, basename(path));
  iput(pi);
  return rtn;
}



/**
 * @brief link inode ii with newpath
 * 
 * @param ii        inode
 * @param newpath   new path
 * @return int      0 on success, -1 on error
 */
int linki(iinode_t *ii, const char *newpath)
{
  ASSERT(ii);
  ASSERT(newpath);
  namei_t in = namei(newpath);
  if (in.i != NULL) {
    iput(in.i);
    /// @todo error link already exists
    return -1;
  }
  if (in.p == 0)    // parent dir not found
    return -1;   // error already set by namei
  if (ii->fs != in.fs) {
    /// @todo error different file systems
    return -1;
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
      return -1;
    }
    bhead_t *bh = bread(LDEVFROMINODE(pi), b.fsblock);
    dirent_t *de = (dirent_t *)&bh->buf->mem[b.offblock];
    if (de->inum == 0) {
      de->inum = ii->inum;
      sncpy(de->name, basename(newpath), DIRNAMEENTRY);
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
  return 0;
}



/**
 * @brief link path with newpath
 * 
 * @param path      path
 * @param newpath   new path
 * @return int      0 on success, -1 on error
 */
int link(const char *path, const char *newpath)
{
  if (!path || !newpath) {
    /// @todo error invalid path or newpath
    return -1;
  }
  namei_t in = namei(path);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return -1;   // error already set by namei
  }
  if (in.i->dinode.ftype == DIRECTORY) {
    /// @todo error is directory
    iput(in.i);
    return -1;
  }
  
  int rtn = linki(in.i, newpath);
  iput(in.i);
  return rtn;
}



/**
 * @brief make node
 * 
 * @param path    path
 * @param ftype   file type
 * @param fmode   file mode
 * @return int    0 on success, -1 on error
 */
int mknode(const char *path, ftype_t ftype, fmode_t fmode)
{
  if (!path || (ftype != REGULAR && ftype != DIRECTORY && ftype != CHARACTER && ftype != BLOCK && ftype != FIFO)) {
    /// @todo error invalid parameters
    return -1;
  }
  namei_t in = namei(path);
  if (in.i != NULL) {
    iput(in.i);
    /// @todo error link already exists
    return -1;
  }
  if (in.p == 0)    // parent dir not found
    return -1;   // error already set by namei
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return -1;   // error already set by iget

  iinode_t *ii = ialloc(in.fs, ftype, fmode);
  if (ii == NULL) {
    iput(pi);
    return -1;   // error already set by ialloc
  }

  int rtn = linki(ii, path);
  iput(ii);
  iput(pi);
  return rtn;
}



/**
 * @brief make node with device numbers (like Unix mknod)
 * 
 * @param path    path to create
 * @param ftype   file type  
 * @param fmode   file mode
 * @param major   major device number (for BLOCK/CHARACTER devices)
 * @param minor   minor device number (for BLOCK/CHARACTER devices)
 * @return int    0 on success, -1 on error
 */
int mknod(const char *path, ftype_t ftype, fmode_t fmode, ldevmajor_t major, ldevminor_t minor)
{
  if (!path || (ftype != REGULAR && ftype != DIRECTORY && ftype != CHARACTER && ftype != BLOCK && ftype != FIFO)) {
    /// @todo error invalid parameters
    return -1;
  }
  namei_t in = namei(path);
  if (in.i != NULL) {
    iput(in.i);
    /// @todo error link already exists
    return -1;
  }
  if (in.p == 0)    // parent dir not found
    return -1;   // error already set by namei
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return -1;   // error already set by iget

  iinode_t *ii = ialloc(in.fs, ftype, fmode);
  if (ii == NULL) {
    iput(pi);
    return -1;   // error already set by ialloc
  }

  // Set device numbers for block and character devices
  if (ftype == BLOCK || ftype == CHARACTER) {
    ii->dinode.ldev.major = major;
    ii->dinode.ldev.minor = minor;
    ii->modified = true;
  }

  int rtn = linki(ii, path);
  iput(ii);
  iput(pi);
  return rtn;
}



/**
 * @brief make directory
 * 
 * @param path    path
 * @param fmode   file mode
 * @return int    0 on success, -1 on error
 */
int mkdir(const char *path, fmode_t fmode)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  }
  mknode(path, DIRECTORY, fmode);
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return -1;
  }
  if (in.p == 0)    // parent dir not found
    return -1;   // error already set by namei
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL)
    return -1;   // error already set by iget

  bmap_t b = bmap(in.i, 0);
  if (b.fsblock == 0) {  // no new blocks left on device
    unlinki(pi, basename(path));
    iput(in.i);
    iput(pi);
    return -1;
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
  return 0;
}



/**
 * @brief remove directory
 * 
 * @param path    path
 * @return int    0 on success, -1 on error
 */
int rmdir(const char *path)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  }
  namei_t in = namei(path);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return -1;   // error already set by namei
  }
  if (in.i->dinode.ftype != DIRECTORY) {
    /// @todo error is not directory
    iput(in.i);
    return -1;
  }
  if (in.i->dinode.nlinks > 3) {
    /// @todo error directory not empty
    iput(in.i);
    return -1;
  }
  iinode_t *pi = iget(in.fs, in.p);
  if (pi == NULL) {
    iput(in.i);
    return -1;   // error already set by iget
  }
  unlinki(in.i, ".");
  unlinki(in.i, "..");
  iput(in.i);
  int rtn = unlinki(pi, basename(path));
  iput(pi);
  return rtn;
}



/**
 * @brief open file
 * 
 * @param fname   file name
 * @param omode   open mode
 * @param fmode   file mode
 * @return int    file descriptor
 */
int open(const char *fname, omode_t omode, fmode_t fmode)
{
  if (!fname || (omode < OREAD || omode > OSYNC)) {
    /// @todo error invalid fname or omode
    return -1;
  }
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



/**
 * @brief close file
 * 
 * @param fdesc   file descriptor
 * @return int    0 on success, -1 on error
 */
int close(int fdesc)
{
  if (fdesc < 0 || fdesc >= MAXOPENFILES || !active->u->fdesc[fdesc].ftabent) {
    /// @todo error invalid file descriptor
    return -1;
  }
  // Calculate file table index from pointer
  int ftab_index = active->u->fdesc[fdesc].ftabent - filetab;
  putftabent(ftab_index);
  active->u->fdesc[fdesc].ftabent = NULL;
  return 0;
}



/**
 * @brief write buf to file
 * 
 * @param fdesc   file descriptor  
 * @param buf     buffer   
 * @param nbytes  number of bytes to write
 * @return int    number of bytes written, -1 on error
 */
int write(int fdesc, byte_t *buf, fsize_t nbytes)
{
  int written = 0;
  if (fdesc < 0 || fdesc >= MAXOPENFILES || !active->u->fdesc[fdesc].ftabent) {
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



/**
 * @brief read file to buf
 * 
 * @param fdesc   file descriptor  
 * @param buf     buffer   
 * @param nbytes  number of bytes to read
 * @return int    number of bytes read, -1 on error
 */
int read(int fdesc, byte_t *buf, fsize_t nbytes)
{
  int read = 0;
  if (fdesc < 0 || fdesc >= MAXOPENFILES || !active->u->fdesc[fdesc].ftabent) {
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



/**
 * @brief seek in file
 * 
 * @param fdesc   file descriptor  
 * @param offset  offset
 * @param whence  whence
 * @return int    offset on success, -1 on error
 */
int lseek(int fdesc, fsize_t offset, seek_t whence)
{
  if (fdesc < 0 || fdesc >= MAXOPENFILES || !active->u->fdesc[fdesc].ftabent) {
    /// @todo error invalid file descriptor
    return -1;
  }
  ASSERT(active->u->fdesc[fdesc].ftabent != NULL);
  iinode_t *ii = active->u->fdesc[fdesc].ftabent->inode;
  switch(ii->dinode.ftype) {
    case REGULAR:
      while(ii->locked) {
        waitfor(INODELOCKED);
      }
      ii->locked = true;
      switch(whence) {
        case SEEKSET:
          active->u->fdesc[fdesc].ftabent->offset = offset;
          break;
        case SEEKCUR:
          active->u->fdesc[fdesc].ftabent->offset += offset;
          break;
        case SEEKEND:
          active->u->fdesc[fdesc].ftabent->offset = ii->dinode.fsize + offset;
          break;
        default:
          /// @todo error invalid whence
          ii->locked = false;
          wakeall(INODELOCKED);
          return -1;
      }
      ii->locked = false;
      wakeall(INODELOCKED);
      return active->u->fdesc[fdesc].ftabent->offset;
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



/**
 * @brief rename file
 * 
 * @param oldpath   old path
 * @param newpath   new path
 * @return int      0 on success, -1 on error
 */
int rename(const char *oldpath, const char *newpath)
{
  if (!oldpath || !newpath) {
    /// @todo error invalid path
    return -1;
  }
  namei_t in = namei(oldpath);
  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return -1;   // error already set by namei
  }
  if (in.i->dinode.ftype == DIRECTORY) {
    /// @todo error is directory
    iput(in.i);
    return -1;
  }
  namei_t in2 = namei(newpath);
  if (in2.i != NULL) {
    iput(in.i);
    iput(in2.i);
    /// @todo error link already exists
    return -1;
  }
  if (in2.p == 0)    // parent dir not found
    return -1;   // error already set by namei
  iinode_t *pi = iget(in2.fs, in2.p);
  if (pi == NULL) {
    iput(in.i);
    return -1;   // error already set by iget
  }
  unlinki(pi, basename(newpath));
  int rtn = linki(in.i, newpath);
  iput(in.i);
  iput(pi);
  return rtn;
}



/**
 * @brief get file status from inode
 * 
 * @param i       inode
 * @param statbuf stat buffer 
 * @return int    0 on success, -1 on error
 */
void _stat(iinode_t *i, stat_t *statbuf)
{
  ASSERT(i);
  ASSERT(statbuf);
  statbuf->ftype = i->dinode.ftype;
  statbuf->fsize = i->dinode.fsize;
  statbuf->fmode = i->dinode.fmode;
  statbuf->nlinks = i->dinode.nlinks;
  statbuf->uid = i->dinode.uid;
  statbuf->gid = i->dinode.gid;
  statbuf->tinode = i->dinode.tinode;
  statbuf->tmod = i->dinode.tmod;
}



/**
 * @brief get file status
 * 
 * @param path    path
 * @param statbuf stat buffer 
 * @return int    0 on success, -1 on error
 */
int stat(const char *path, stat_t *statbuf)
{
  if (!path || !statbuf) {
    /// @todo error invalid path or statbuf
    return -1;
  } 
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return -1;
  }

  _stat(in.i, statbuf);
  iput(in.i);
  return 0;
}



/**
 * @brief get file status from file descriptor
 * 
 * @param fdesc   file descriptor
 * @param statbuf stat buffer 
 * @return int    0 on success, -1 on error
 */
int fstat(int fdesc, stat_t *statbuf)
{
  if (!statbuf) {
    /// @todo error invalid path or statbuf
    return -1;
  } 
  if (fdesc < 0 || fdesc >= MAXOPENFILES || !active->u->fdesc[fdesc].ftabent) {
    /// @todo error invalid file descriptor
    return -1;
  }
  _stat(active->u->fdesc[fdesc].ftabent->inode, statbuf);
  return 0;
}



/**
 * @brief change owner
 * 
 * @param path    path
 * @param uid     user id
 * @param gid     group id
 * @return int    0 on success, -1 on error
 */
int chown(const char *path, user_t uid, group_t gid)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  } 
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return -1;
  }
  while(in.i->locked) {
    waitfor(INODELOCKED);
  }
  in.i->locked = true;
  in.i->dinode.uid = uid;
  in.i->dinode.gid = gid;
  in.i->modified = true;
  in.i->locked = false;
  wakeall(INODELOCKED);
  iput(in.i);
  return 0;
}



/**
 * @brief change mode
 * 
 * @param path    path
 * @param fmode   file mode
 * @return int    0 on success, -1 on error
 */
int chmod(const char *path, fmode_t fmode)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  } 
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return -1;
  }
  while(in.i->locked) {
    waitfor(INODELOCKED);
  }
  in.i->locked = true;
  in.i->dinode.fmode = fmode;
  in.i->modified = true;
  in.i->locked = false;
  wakeall(INODELOCKED);
  iput(in.i);
  return 0;
}



/**
 * @brief change directory
 * 
 * @param path    path
 * @return int    0 on success, -1 on error
 */
int chdir(const char *path)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  } 
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return -1;
  }
  if (in.i->dinode.ftype != DIRECTORY) {
    /// @todo error is not directory
    iput(in.i);
    return -1;
  }
  iput(active->u->workdir);
  active->u->workdir = in.i;
  return 0;
}



/**
 * @brief change root directory
 * 
 * @param path    path
 * @return int    0 on success, -1 on error
 */
int chroot(const char *path)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  } 
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error link does not exists
    return -1;
  }
  if (in.i->dinode.ftype != DIRECTORY) {
    /// @todo error is not directory
    iput(in.i);
    return -1;
  }
  iput(active->u->fsroot);
  active->u->fsroot = in.i;
  return 0;
}



/**
 * @brief duplicate file descriptor
 * 
 * @param fdesc   file descriptor
 * @return int    new file descriptor
 */
int dup(int fdesc)
{
  if (fdesc < 0 || fdesc >= MAXOPENFILES || !active->u->fdesc[fdesc].ftabent) {
    /// @todo error invalid file descriptor
    return -1;
  }
  int fdesc2 = freefdesc();
  if (fdesc2 < 0) {
    /// @todo error no free filetab entry
    return -1;
  }
  active->u->fdesc[fdesc2] = active->u->fdesc[fdesc];
  active->u->fdesc[fdesc2].ftabent->refs++;
  return fdesc2;
}


/**
 * @brief get parent inode of directory (another directory)
 * 
 * assumption is that the 2nd entry in the directory is the link to the parent directory (..  entry)
 * 
 * @param ii 
 * @return pdir_t   parent directory inode and child inode number
 */
pdir_t igetpdir(iinode_t *ii)
{
  pdir_t rtn = {NULL, 0};
  ASSERT(ii);
  ASSERT(ii->dinode.ftype == DIRECTORY);
  ASSERT(ii->dinode.fsize % sizeof(dirent_t) == 0);
  ASSERT(ii->dinode.fsize >= 2 * sizeof(dirent_t));
  ASSERT(ii);
  if (ii->inum == 1) {            // root directory of fs
    isuperblock_t *isbk = getisblock(ii->fs);
    ASSERT(isbk);
    if (isbk->mounted) {     // is fs mounted ?
      rtn.pi = iget(isbk->pfs, isbk->pino);
      ASSERT(rtn.pi);
      rtn.child = isbk->mounted->inum;
      return rtn;
    } 
  }
  bmap_t b = bmap(ii, 1 * sizeof(dirent_t));    // 2nd entry in directory
  ASSERT(b.fsblock > 0);
  bhead_t *bh = bread(LDEVFROMINODE(ii), b.fsblock);
  dirent_t *de = (dirent_t *)&bh->buf->mem[b.offblock];
  ASSERT(de->inum > 0);
  ASSERT(sncmp(de->name, "..", DIRNAMEENTRY) == 0);
  rtn.pi = iget(ii->fs, de->inum);
  brelse(bh);
  ASSERT(rtn.pi);
  rtn.child = ii->inum;
  return rtn;
}



/**
 * @brief get current working directory path
 * 
 * @param buf   buffer to store path
 * @param len   buffer length
 * @return char*  path
 */
char *getcwd(char *buf, sizem_t len)
{
  if (!buf || len < 2) {
    /// @todo error invalid buffer
    return NULL;
  }
  iinode_t *ii = iget(active->u->workdir->fs, active->u->workdir->inum);
  ASSERT(ii);
  buf[--len] = '\0';
  while (true)
  {
    ASSERT(ii->dinode.ftype == DIRECTORY);
    ASSERT(ii->dinode.fsize % sizeof(dirent_t) == 0);
    if (ii == active->u->fsroot)
      break;
    pdir_t p = igetpdir(ii);
    ASSERT(p.pi);
    ASSERT(p.child > 0);
    iput(ii);
    ii = p.pi;
    int n = ii->dinode.fsize / sizeof(dirent_t);
    ASSERT(n >= 2);
    int found = false;
    for (int i = 0; i < n; i++) {
      bmap_t b = bmap(ii, i * sizeof(dirent_t));
      ASSERT(b.fsblock > 0);
      bhead_t *bh = bread(LDEVFROMINODE(ii), b.fsblock);    /// @todo can be optimized 
      dirent_t *de = (dirent_t *)&bh->buf->mem[b.offblock];
      if (de->inum == p.child) {
        sizem_t bl = snlen(de->name, DIRNAMEENTRY);
        if (len < bl + 1) {
          /// @todo error buffer too small
          iput(ii);
          return NULL;
        }
        mcpy(&buf[len - bl], de->name, bl);
        len -= bl;
        buf[--len] = '/';
        brelse(bh);
        found = true;
        break;
      }
      brelse(bh);
    }
    if (!found) {
      /// @todo error parent directory not found
      iput(ii);
      return NULL;
    }
  }
  if (len > 0) {
    if (buf[len] != 0)
      sncpy(buf, &buf[len], snlen(&buf[len], MAXPATH - len));
    else {
      buf[0] = '/';
      buf[1] = '\0';
    }
  }
  return buf;
}



/**
 * @brief sync file system
 * 
 * @return int 
 */
int sync(void)
{
  syncall_buffers(false);
  return 0;
}


/**
 * @brief Open a directory for reading
 * 
 * @param path Directory path to open
 * @return int File descriptor for directory operations, -1 on error
 */
int opendir(const char *path)
{
  if (!path) {
    /// @todo error invalid path
    return -1;
  }
  
  int fdesc = freefdesc();
  if (fdesc < 0) {
    /// @todo error no free file descriptor
    return -1;
  }
  
  namei_t in = namei(path);
  if (in.i == NULL) {
    /// @todo error path does not exist
    return -1;
  }
  
  if (in.i->dinode.ftype != DIRECTORY) {
    /// @todo error not a directory
    iput(in.i);
    return -1;
  }
  
  int f = getftabent(in.i);
  if (f < 0) {
    iput(in.i);
    /// @todo error no free filetab entry
    return -1;
  }
  
  // Set up file table entry for directory reading
  filetab[f].offset = 0;  // Start reading from beginning of directory
  filetab[f].flags = OREAD;  // Directory is opened read-only
  
  // Set up file descriptor
  active->u->fdesc[fdesc].ftabent = &filetab[f];
  active->u->fdesc[fdesc].omode = OREAD;
  
  return fdesc;
}

/**
 * @brief Close a directory file descriptor
 * 
 * @param fd Directory file descriptor
 * @return int 0 on success, -1 on error
 */
int closedir(int fd)
{
  if (fd < 0 || fd >= MAXOPENFILES || !active->u->fdesc[fd].ftabent) {
    /// @todo error invalid file descriptor
    return -1;
  }
  
  filetab_t *ft = active->u->fdesc[fd].ftabent;
  if (!ft->inode || ft->inode->dinode.ftype != DIRECTORY) {
    /// @todo error not a directory file descriptor
    return -1;
  }
  
  // Use the standard close mechanism
  putftabent(fd);
  active->u->fdesc[fd].ftabent = NULL;
  
  return 0;
}

/**
 * @brief Read next directory entry
 * 
 * @param fd Directory file descriptor
 * @param buf Buffer to store directory entry
 * @return int 1 on success (entry read), 0 on end of directory, -1 on error
 */
int readdir(int fd, dirent_t *buf)
{
  if (fd < 0 || fd >= MAXOPENFILES || !active->u->fdesc[fd].ftabent || !buf) {
    /// @todo error invalid parameters
    return -1;
  }
  
  filetab_t *ft = active->u->fdesc[fd].ftabent;
  if (!ft->inode || ft->inode->dinode.ftype != DIRECTORY) {
    /// @todo error not a directory file descriptor
    return -1;
  }
  
  iinode_t *dir_inode = ft->inode;
  
  // Check if we've reached the end of the directory
  if (ft->offset >= dir_inode->dinode.fsize) {
    return 0;  // End of directory
  }
  
  // Ensure directory size is valid (multiple of dirent_t size)
  ASSERT(dir_inode->dinode.fsize % sizeof(dirent_t) == 0);
  
  // Calculate which directory entry we're reading
  fsize_t entry_offset = ft->offset;
  
  // Map the directory entry location to a block
  bmap_t b = bmap(dir_inode, entry_offset);
  if (b.fsblock == 0) {
    /// @todo error reading directory block
    return -1;
  }
  
  // Read the block containing the directory entry
  bhead_t *bh = bread(LDEVFROMINODE(dir_inode), b.fsblock);
  if (!bh) {
    /// @todo error reading directory block
    return -1;
  }
  
  // Get pointer to the directory entry in the buffer
  dirent_t *de = (dirent_t *)&bh->buf->mem[b.offblock];
  
  // Copy the directory entry to user buffer
  mcpy(buf, de, sizeof(dirent_t));
  
  // Release the buffer
  brelse(bh);
  
  // Advance to next directory entry
  ft->offset += sizeof(dirent_t);
  
  return 1;  // Successfully read an entry
}
