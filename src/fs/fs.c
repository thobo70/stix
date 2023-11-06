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



void unlink(const char *path)
{
  ASSERT(path != NULL);
  namei_t in = namei(path);
  iinode_t *pi;   // parent inode

  if (in.i == NULL || in.p == 0) {
    if (in.i != NULL)
      iput(in.i);
    return;
  }

  pi = iget(in.i->fs, in.p);
  ASSERT(pi != NULL);
  ASSERT(pi->dinode.ftype == DIRECTORY);
  ASSERT(pi->dinode.fsize % sizeof(dirent_t) == 0);
  int n = pi->dinode.fsize / sizeof(dirent_t);
  const char *bn = basename(path);
  int bl = snlen(bn, sizeof(DIRNAMEENTRY));
  for (int i = 0; i < n; i++) {
    bmap_t b = bmap(pi, i * sizeof(dirent_t));
    ASSERT(b.fsblock != 0);
    bhead_t *bh = bread(LDEVFROMINODE(pi), b.fsblock);
    dirent_t *de = (dirent_t *)bh->buf->mem;
    if (sncmp(de->name, bn, bl) == 0) {
      ASSERT(de->inum == in.i->inum);
      de->inum = 0;
      in.i->dinode.nlinks--;
      in.i->modified = true;
      bh->dwrite = true;
      bwrite(bh);
      brelse(bh);
      break;
    }
  }
  iput(pi);
  iput(in.i);
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
    dirent_t *de = (dirent_t *)bh->buf->mem;
    if (de->inum == 0) {
      de->inum = ii->inum;
      sncpy(de->name, basename(newpath), sizeof(DIRNAMEENTRY));
      ii->dinode.nlinks++;
      ii->modified = true;
      bh->dwrite = true;
      bwrite(bh);
      brelse(bh);
      if (i == n) { // new directory entry
        pi->dinode.fsize += sizeof(dirent_t);
        pi->modified = true;
      }
      break;
    }
  }
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
  linki(in.i, newpath);
  iput(in.i);
}



void mkdir(const char *path)
{
  ASSERT(path != NULL);
  namei_t in = namei(path);
  if (in.i != NULL)
    iput(in.i);
  if (in.i != NULL) {
    /// @todo error link already exists
    return;
  }
  if (in.p == 0)    // parent dir not found
    return;   // error already set by namei

  iinode_t *ii = ialloc(in.fs, DIRECTORY);
  if (ii == NULL) 
    return;   // error already set by ialloc

  linki(ii, path);
  iput(ii);
}
