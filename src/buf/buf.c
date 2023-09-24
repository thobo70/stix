/**
 * @file buf.c
 * @author Thomas Boos
 * @brief buffer handling
 * @version 0.1
 * @date 2023-09-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "utils.h"

#include "buf.h"
#include "pc.h"
#include "fs.h"

#define HTABSIZEBITS 4

#define HTABSIZE (1 << HTABSIZEBITS)
#define HTABMASK (HTABSIZE - 1)
#define HTABVALUE(dev, block) ((block) & HTABMASK)
#define HTAB(dev, block) hashtab[HTABVALUE(dev, block)]

bhead_t bufhead[NBUFFER];
buffer_t buf[NBUFFER];

bhead_t *hashtab[HTABSIZE];
bhead_t *freefirst = NULL;
bhead_t *freelast = NULL;


void init_buffers(void)
{
  memset(bufhead, 0, sizeof(bufhead));
  memset(hashtab, 0, sizeof(hashtab));

  for ( int i ; i < NBUFFER ; ++i ) {
    bhead_t *b = &bufhead[i];
    b->buf = &buf[i];
    if (!freelast)
      freelast = b;
    if (freefirst)
      freefirst->fprev = b;
    b->fnext = freefirst;
    freefirst = b; 
    b->infreelist = true;
  }
}


/**
 * @brief remove block from free list
 * 
 * @param b 
 */
void remove_from_free_list(bhead_t *b)
{
  bhead_t *p = b->fprev, *n = b->fnext;

  if (!b->infreelist)
    return;

  if (p)
    p->fnext = n;
  else
    freefirst = n;
  if (n)
    n->fnext = p;
  else
    freelast = p;

  b->infreelist = false;
}



void add_free_first(bhead_t *b)
{
  if (b->infreelist)
    return;
  b->fprev = NULL;
  b->fnext = freefirst;
  freefirst = b;
  if (!freelast)
    freelast = b;
  b->infreelist = true;
}


void add_free_last(bhead_t *b)
{
  if (b->infreelist)
    return;
  b->fnext = NULL;
  b->fprev = freelast;
  freelast = b;
  if (!freefirst)
    freefirst = b;
  b->infreelist = true;
}



void move_to_new_hashqueue(bhead_t *b, ldev_t dev, block_t block)
{
  bhead_t *p = b->hprev, *n = b->hnext;

  if (HTAB(b->dev, b->block) == b)
    HTAB(b->dev, b->block) = n;
  if (p)
    p->hnext = n;
  if (n)
    n->hprev = p;

  b->hprev = NULL;
  b->hnext = HTAB(dev, block);
  HTAB(dev, block) = b;
  b->dev = dev;
  b->block = block;
  b->valid = false;
}



bhead_t *getblk(ldev_t dev, block_t block)
{
  bhead_t *found = NULL;
  for(;;) {
    for ( found = HTAB(dev, block) ; found ; found = found->hnext )
      if ((found->dev == dev) && (found->block == block))
        break;
    if (found) {
      if (found->busy) {
        waitfor(BLOCKBUSY);
        continue;
      }
      found->busy = true;
      remove_from_free_list(found);
      return found;
    } else {
      found = freefirst;
      if (!found) {
        waitfor(NOFREEBLOCKS);
        continue;
      }
      remove_from_free_list(found);
      if (found->dwrite) {
        sync_buffer_to_disk(found);
        continue;
      }
      found->busy = true;
      move_to_new_hashqueue(found, dev, block);
      return found;
    }
  }
}



void brelse(bhead_t *b)
{
  if (b->valid)
    add_free_last(b);
  else
    add_free_first(b);
  b->busy = false;
  wakeall(BLOCKBUSY);
}



bhead_t *bread(ldev_t dev, block_t block)
{
  bhead_t *b = getblk(dev, block);

  if (b->valid)
    return b;

  sync_buffer_from_disk(b);
  while (!b->valid)
    waitfor(BLOCKREAD);

  return b;
}

bhead_t *bwrite = NULL;

void buffer_synced(bhead_t *b, int err)
{
  b->dwrite = false;
  b->valid = (err == 0);
  if (err)
    add_free_first(b);
  else
    add_free_last(b);
}


void sync_buffer_to_disk(bhead_t *b)
{
  if (bwrite)
    bwrite->fprev = b;
  bwrite = b;
}