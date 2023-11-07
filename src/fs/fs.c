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



void mknode(const char *path, ftype_t ftype)
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

  iinode_t *ii = ialloc(in.fs, ftype);
  if (ii == NULL) {
    iput(pi);
    return;   // error already set by ialloc
  }

  linki(ii, path);
  iput(ii);
  iput(pi);
}



void mkdir(const char *path)
{
  ASSERT(path != NULL);
  mknode(path, DIRECTORY);
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