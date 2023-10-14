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
#include "blocks.h"
#include "dd.h"

#define NINODES 50          ///< number of inodes in system

#define NBLOCKREFS 20       ///< number of block references in inode
#define STARTREFSLEVEL 18   ///< index of start of reference levels > 0

enum ftype {
  IUNSPEC = 0,
  IFREE,
  REGULAR,
  DIRECTORY,
  CHARACTER,
  BLOCK,
  FIFO
};

typedef word_t ftype_t;

/// @brief inode stored on disk
typedef struct dinode_t {
  ftype_t ftype;
  owner_t owner;
  group_t group;
  fmode_t fmode;
  utime_t tmod;
  utime_t tinode;
  nlinks_t nlinks;
  fsize_t fsize;
  block_t blockrefs[NBLOCKREFS];
} dinode_t;

/// @brief in core inode
typedef struct iinode_t
{
  dinode_t dinode;
  byte_t locked : 1;
  byte_t modified : 1;
  fsnum_t fs;
  ninode_t inum;
  nref_t nref;
  struct iinode_t *hprev;
  struct iinode_t *hnext;
  struct iinode_t *fprev;
  struct iinode_t *fnext;
} iinode_t;



typedef struct bmap_t {
  block_t fsblock;
  fsize_t offblock;
  fsize_t nbytesleft;
  block_t rdablock;
} bmap_t;



/**
 * @brief init inodes 
 * 
 */
void init_inodes(void);

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

iinode_t *namei(const char *p);

#endif
