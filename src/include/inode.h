/**
 * @file inode.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief inode handling
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _INODE_H
#define _INODE_H

#include "tdefs.h"
#include "dd.h"
#include "clist.h"

#define NINODES 50          ///< number of inodes in system

#define NBLOCKREFS 21       ///< number of block references in inode
#define STARTREFSLEVEL 19   ///< index of start of reference levels > 0

enum ftype {
  IFREE = 0,
  REGULAR,
  DIRECTORY,
  CHARACTER,
  BLOCK,
  FIFO,
  IUNSPEC
};

typedef word_t ftype_t;

/// @brief inode stored on disk
typedef struct dinode_t {
  ftype_t ftype;
  user_t uid;
  group_t gid;
  fmode_t fmode;
  utime_t tmod;
  utime_t tinode;
  nlinks_t nlinks;
  fsize_t fsize;
  union {
    block_t blockrefs[NBLOCKREFS];
    ldev_t ldev;
  };
} _STRUCTATTR_ dinode_t;

/// @brief in core inode
typedef struct iinode_t
{
  dinode_t dinode;
  byte_t locked : 1;
  byte_t modified : 1;
  fsnum_t fs;
  ninode_t inum;
  nref_t nref;
  clist_t *rclist;
  clist_t *wclist;
  fsnum_t fsmnt;
  struct iinode_t *hprev;
  struct iinode_t *hnext;
  struct iinode_t *fprev;
  struct iinode_t *fnext;
} _STRUCTATTR_ iinode_t;

typedef struct stat_t {
  ftype_t ftype;
  user_t uid;
  group_t gid;
  fmode_t fmode;
  utime_t tmod;
  utime_t tinode;
  nlinks_t nlinks;
  fsize_t fsize;
  ldev_t ldev;
} _STRUCTATTR_ stat_t;

typedef struct bmap_t {
  block_t fsblock;
  fsize_t offblock;
  fsize_t nbytesleft;
  block_t rdablock;
} _STRUCTATTR_ bmap_t;

typedef struct namei_t {
  iinode_t *i;
  ninode_t p;
  fsnum_t fs;
} _STRUCTATTR_ namei_t;

#define NINODESBLOCK  ((block_t)( BLOCKSIZE / sizeof(dinode_t) ))


/**
 * @brief init inodes 
 * 
 */
void init_inodes(void);

iinode_t *ialloc(fsnum_t fs, ftype_t ftype, fmode_t fmode);
void ifree(iinode_t *inode);


/**
 * @brief searches inode in hashlist or sets up a new one
 * 
 * @param fs 
 * @param inum 
 * @return iinode_t* inode or NULL if not found and no free inode left
 */
iinode_t *iget(fsnum_t fs, ninode_t inum);

/**
 * @brief release access to in core inode
 * 
 * @param inode   in core inode
 */
void iput(iinode_t *inode);

namei_t namei(const char *p);
bmap_t bmap(iinode_t *inode, fsize_t pos);

void free_all_blocks(iinode_t *inode);

int activeinodes(fsnum_t fs);
int count_open_files_on_fs(fsnum_t fs);
int is_fs_busy_workdir(fsnum_t fs);

#endif
