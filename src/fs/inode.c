/**
 * @file inode.c
 * @author Thomas Boos
 * @brief handles inode stuff
 * @version 0.1
 * @date 2023-09-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "inode.h"
#include "buf.h"
#include "pc.h"
#include "utils.h"

#define SUPERBLOCKINODE(fs)  0   /// TODO: replace with real first inode block from dev
#define LDEVFROMFS(fs)  0        /// TODO: ldev to fs converting

#define HTABSIZEBITS 4

#define HTABSIZE (1 << HTABSIZEBITS)
#define HTABMASK (HTABSIZE - 1)
#define HTABVALUE(fs, inum) ((inum) & HTABMASK)
#define HTAB(fs, inum) ihashtab[HTABVALUE(dev, inum)]

#define NINODESBLOCK  (BLOCKSIZE / sizeof(dinode_t))
#define INODEBLOCK(fs, inum) (((inum - 1) / NINODESBLOCK) + SUPERBLOCKINODE(fs))
#define INODEOFFSET(inum) (((inum - 1) % NINODESBLOCK) * sizeof(dinode_t))

#define NREFSPERBLOCK (BLOCKSIZE / sizeof(block_t))

iinode_t iinode[NINODES];

iinode_t *ihashtab[HTABSIZE];
iinode_t *ifreelist = NULL;



void freeblocklevel(int level, fsnum_t fs, block_t bl)
{
  if (level) {
    --level;
    bhead_t *b = bread(LDEVFROMFS(fs), bl);
    block_t *brefs = (block_t *)b->buf->mem;
    for ( int i = 0 ; (i < NREFSPERBLOCK) && brefs[i] ; ++i )
      freeblocklevel(level, fs, brefs[i]);
    brelse(b);
  }
  freeblock(fs, bl);
}


/**
 * @brief free all blocks assigned to this inode
 * 
 * @param inode 
 */
void free_all_blocks(iinode_t *inode)
{
  int level = 0;

  for ( int i = 0 ; i < NBLOCKREFS ; ++i ) {
    block_t bl = inode->dinode.blockrefs[i];
    if (bl == 0)
      break;
    if (i >= STARTREFSLEVEL)
      ++level;
    freeblocklevel(level, inode->fs, bl);
  }
}


/**
 * @brief write inode to disk
 * 
 * @param inode 
 */
void update_inode_on_disk(iinode_t *inode)
{
  bhead_t *b = bread(LDEVFROMFS(inode->fs), INODEBLOCK(inode->fs, inode->inum));
  mcpy(&b->buf->mem[INODEOFFSET(inode->inum)], &inode->dinode, sizeof(dinode_t));
  b->dwrite = true;
  bwrite(b);
  brelse(b);
  inode->modified = false;
}

/**
 * @brief remove inode from free list
 * 
 * @param inode inode to remove
 */
void remove_inode_from_freelist(iinode_t *inode)
{
  if (!inode->fnext)
    return;
  if (inode->fnext == inode) {
    ifreelist = NULL;
  } else {
    if (inode == ifreelist) 
      ifreelist = inode->fnext;
    inode->fprev->fnext = inode->fnext;
    inode->fnext->fprev = inode->fprev;
  }
  inode->fnext = NULL;
  inode->fprev = NULL;
}


/**
 * @brief add indoe to free list
 * 
 * @param inode     inode put to free list
 * @param asFirst   true if to add in front of list, otherwise false
 */
void add_inode_to_freelist(iinode_t *inode, int asFirst)
{
  if (ifreelist) {
    inode->fprev = ifreelist->fprev;
    inode->fnext = ifreelist;
    ifreelist->fprev->fnext = inode;
    ifreelist->fprev = inode;
    if (asFirst)
      ifreelist = inode;
  } else {
    ifreelist = inode;
    inode->fnext = inode;
    inode->fprev = inode;
  }
}


/**
 * @brief move inot to new hash queue
 * 
 * @param inode the inode to move
 * @param fs    fs of new queue
 * @param inum  inum of new queue
 */
void move_inode_to_hashqueue(iinode_t *inode, fsnum_t fs, ninode_t inum)
{
  iinode_t *p = HTAB(fs, inum);
  if (inode->hnext) {
    if (inode->hnext == inode)
      HTAB(inode->fs, inode->inum) = NULL;
    else
      HTAB(inode->fs, inode->inum) = inode->hnext;
    inode->hprev->hnext = inode->hnext;
    inode->hnext->hprev = inode->hprev;
  }
  inode->fs = fs;
  inode->inum = inum;
  if (p) {
    p->hprev->hnext = inode;
    p->hprev = inode;
  } else {
    inode->hprev = inode;
    inode->hnext = inode;    
    HTAB(fs, inum) = inode;
  }
}



void ifree(iinode_t *inode)
{

}



/**
 * @brief init inodes
 * 
 */
void init_inodes(void)
{
  memset(iinode, 0, sizeof(iinode));
  memset(ihashtab, 0, sizeof(ihashtab));

  for ( int i ; i < NBUFFER ; ++i ) 
    add_inode_to_freelist(&iinode[i], 0);
}


/**
 * @brief searches inode in hashlist or sets up a new one
 * 
 * @param fs 
 * @param inum 
 * @return iinode_t* inode or NULL if not found and no free inode left
 */
iinode_t *iget(fsnum_t fs, ninode_t inum)
{
  iinode_t *found = NULL;
  bhead_t *bhead = NULL;

  for(;;) {
    for ( found = HTAB(fs, inum) ; found ; found = found->hnext )
      if ((found->fs == fs) && (found->inum == inum))
        break;
    if (found) {
      if (found->locked) {
        waitfor(INODELOCKED);
        continue;
      }
      /// @todo mount points !!!
      remove_inode_from_freelist(found);
      found->nref++;
      return found;
    }
    found = ifreelist;
    if (!found) {
      /// @todo error no free inodes
      return NULL;
    }
    remove_inode_from_freelist(found);
    move_inode_to_hashqueue(found, fs, inum);
    bhead = bread(LDEVFROMFS(inode->fs), INODEBLOCK(inum, SUPERBLOCKINODE(fs)));
    mcpy(&found->dinode, &bhead->buf->mem[INODEOFFSET(inum)], sizeof(dinode_t));
    brelse(bhead);
    found->nref = 1;
    return found;
  }
}



/**
 * @brief releases inode
 * 
 * @param inode 
 */
void iput(iinode_t *inode)
{
  inode->locked = true;
  inode->nref--;
  if (inode->nref == 0) {
    if (inode->dinode.nlinks == 0) {
      free_all_blocks(inode);
      ifree(inode);
    }
    if (inode->modified)
      update_inode_on_disk(inode);
    add_inode_to_freelist(inode, false);
  }
  inode->locked = false;
  wakeall(INODELOCKED);
}

void bmap(void)
{

}

void ialloc(void)
{

}

void namei(void)
{

}
