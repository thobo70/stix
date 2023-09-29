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

#define SUPERBLOCKINODE(dev)  0   /// TODO: replace with real first inode block from dev

#define HTABSIZEBITS 4

#define HTABSIZE (1 << HTABSIZEBITS)
#define HTABMASK (HTABSIZE - 1)
#define HTABVALUE(dev, inum) ((inum) & HTABMASK)
#define HTAB(dev, inum) ihashtab[HTABVALUE(dev, inum)]

iinode_t iinode[NINODES];

iinode_t *ihashtab[HTABSIZE];
iinode_t *ifreelist = NULL;



/**
 * @brief free all blocks assigned to this inode
 * 
 * @param inode 
 */
void free_all_blocks(iinode_t *inode)
{

}


/**
 * @brief write inode to disk
 * 
 * @param inode 
 */
void update_inode_on_disk(iinode_t *inode)
{

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
 * @param dev   dev of new queue
 * @param inum  inum of new queue
 */
void move_inode_to_hashqueue(iinode_t *inode, ldev_t dev, ninode_t inum)
{
  iinode_t *p = HTAB(dev, inum);
  if (inode->hnext) {
    if (inode->hnext == inode)
      HTAB(inode->dev, inode->inum) = NULL;
    else
      HTAB(inode->dev, inode->inum) = inode->hnext;
    inode->hprev->hnext = inode->hnext;
    inode->hnext->hprev = inode->hprev;
  }
  inode->dev = dev;
  inode->inum = inum;
  if (p) {
    p->hprev->hnext = inode;
    p->hprev = inode;
  } else {
    inode->hprev = inode;
    inode->hnext = inode;    
    HTAB(dev, inum) = inode;
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
 * @param dev 
 * @param inum 
 * @return iinode_t* inode or NULL if not found and no free inode left
 */
iinode_t *iget(ldev_t dev, ninode_t inum)
{
  iinode_t *found = NULL;
  bhead_t *bhead = NULL;

  for(;;) {
    for ( found = HTAB(dev, inum) ; found ; found = found->hnext )
      if ((found->dev == dev) && (found->inum == inum))
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
    move_inode_to_hashqueue(found, dev, inum);
    bhead = bread(dev, INODEBLOCK(inum, SUPERBLOCKINODE(dev)));
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
      inode->dinode.ftype = 0;
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
