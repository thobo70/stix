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

#define HTABSIZEBITS 4

#define HTABSIZE (1 << HTABSIZEBITS)
#define HTABMASK (HTABSIZE - 1)
#define HTABVALUE(dev, inum) ((inum) & HTABMASK)
#define HTAB(dev, inum) hashtab[HTABVALUE(dev, inum)]

iinode_t iinode[NINODES];

iinode_t *hashtab[HTABSIZE];
iinode_t *freefirst = NULL;
iinode_t *freelast = NULL;

void init_buffers(void)
{
  memset(iinode, 0, sizeof(iinode));
  memset(hashtab, 0, sizeof(hashtab));

  for ( int i ; i < NBUFFER ; ++i ) {
    iinode_t *inode = &iinode[i];
    if (!freelast)
      freelast = inode;
    if (freefirst)
      freefirst->fprev = inode;
    inode->fnext = freefirst;
    freefirst = inode; 
  }
}


iinode_t *iget(ldev_t dev, ninode_t inum)
{
  iinode_t *found = NULL;

  for(;;) {
    for ( found = HTAB(dev, inum) ; found ; found = found->hnext )
      if ((found->dev == dev) && (found->inum == inum))
        break;
    if (found) {
      if (found->locked) {
        waitfor(INODELOCKED);
        continue;
      }
      /// TODO: mount points !!!
      remove_from_free_list(found);
      found->nref++;
      return found;
    }
    if (!freefirst) {
      /// TODO: error no free inodes
      return NULL;
    }
  }
}


void iput(void)
{

}

void bmap(void)
{

}

void ialloc(void)
{

}

void ifree(void)
{

}


void namei(void)
{

}
