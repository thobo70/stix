#ifndef _INODE_H
#define _INODE_H

#include "tdefs.h"
#include "blocks.h"
#include "dd.h"

#define NINODES 50

#define NINODESBLOCK  (BLOCKSIZE / sizeof(dinode_t))
#define INODEBLOCK(inum, startblock) (((inum - 1) / NINODESBLOCK) + startblock)
#define INODEOFFSET(inum) (((inum - 1) % NINODESBLOCK) * sizeof(dinode_t))

enum ftype {
  REGULAR,
  DIRECTORY,
  CHARACTER,
  BLOCK,
  FIFO
};

typedef enum ftype ftype_t;

/// @brief inode stored on disk
typedef struct dinode_t {
  owner_t owner;
  group_t group;
  ftype_t ftype;
  fmode_t fmode;
  time_t tmod;
  time_t tinode;
  nlinks_t nlinks;
  fsize_t fsize;
} dinode_t;

/// @brief in core inode
typedef struct iinode_t
{
  dinode_t dinode;
  byte_t locked : 1;
  byte_t modified : 1;
  ldev_t dev;
  ninode_t inum;
  nref_t nref;
  struct iinode_t *hprev;
  struct iinode_t *hnext;
  struct iinode_t *fprev;
  struct iinode_t *fnext;
} iinode_t;



/**
 * @brief init inodes 
 * 
 */
void init_inodes(void);

/**
 * @brief find inode in hash or setup a new one
 * 
 * @param dev         device of filesystem containing the inode
 * @param inum        number of inode
 * @return iinode_t*  pointer to inode or NULL of free in core inodes available
 */
iinode_t *iget(ldev_t dev, ninode_t inum);

/**
 * @brief release access to in core inode
 * 
 * @param inode   in core inode
 */
void iput(iinode_t *inode);

#endif
