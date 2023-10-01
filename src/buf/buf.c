/**
 * @file buf.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief buffer handling
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 * @startuml
 * Alice -> Bob: test
 * Bob -> Bob: thinks
 * Bob -> Nena: really
 * Nena -> Alice: no
 * @enduml
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
bhead_t *freelist = NULL;

void sync_buffer_to_disk(bhead_t *b);
void sync_buffer_from_disk(bhead_t *b);

/**
 * @brief remove buffer from free list
 * 
 * @param b 
 */
void remove_buf_from_freelist(bhead_t *b)
{
  if (!b->infreelist)
    return;

  if (b == b->fnext)
    freelist = NULL;
  else {
    if (b == freelist)
      freelist = b->fnext;
    b->fnext->fprev = b->fprev;
    b->fprev->fnext = b->fnext;
  }

  b->infreelist = false;
}



void add_buf_to_freelist(bhead_t *b, int asFirst)
{
  if (b->infreelist)
    return;
  if (freelist) {
    b->fnext = freelist;
    b->fprev = freelist->fprev;
    freelist->fprev->fnext = b;
    freelist->fprev = b;
    if (asFirst)
      freelist = b;
  } else {
    freelist = b;
    b->fnext = b;
    b->fprev = b;
  }
  b->infreelist = true;
}



void init_buffers(void)
{
  memset(bufhead, 0, sizeof(bufhead));
  memset(hashtab, 0, sizeof(hashtab));

  for ( int i ; i < NBUFFER ; ++i ) 
    add_buf_to_freelist(&bufhead[i], false);
}


/**
 * @brief unlink b from old queue and add to new hash queue
 * 
 * @param b 
 * @param dev 
 * @param block 
 */
void move_buf_to_hashqueue(bhead_t *b, ldev_t dev, block_t block)
{
  bhead_t *p = HTAB(dev, block);

  b->valid = false;
  if (b->hnext) {
    if (b->hnext == b)
      HTAB(b->dev, b->block) = NULL;
    else
      HTAB(b->dev, b->block) = b->hnext;
    b->hprev->hnext = b->hnext;
    b->hnext->hprev = b->hprev;
  }
  b->dev = dev;
  b->block = block;
  if (p) {
    p->hprev->hnext = b;
    p->hprev = b;
  } else {
    b->hprev = b;
    b->hnext = b;
    HTAB(dev, block) = b;
  }
}


/**
 * @brief looks for a buffer containing block from dev in hash or tries to get a free buffer
 * 
 * @startuml
 * start
 * repeat
 *   if (block in hash queue) then (yes)
 *     if (buffer is busy) then (yes)
 *       : sleep(event buffer becomes free);
 *     else (no)
 *       : mark buffer as busy;
 *       : remove buffer from free list;
 *       : return buffer;
 *     stop
 *     endif
 *   else (no)
 *     if (freelist empty) then (yes)
 *       : sleep(event any buffer gets free);
 *     else (no)
 *       : remove buffer from free list;
 *       if (buffer marked for delayed write) then (yes)
 *         : async write buffer;
 *       else (no)
 *         : move buffer from old to new hash queue;
 *         : return buffer;
 *         stop
 *       endif
 *     endif
 *   endif
 * repeat while (endless)
 * @enduml
 * 
 * @param dev 
 * @param block 
 * @return bhead_t* 
 */
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
      remove_buf_from_freelist(found);
      return found;
    } else {
      found = freelist;
      if (!found) {
        waitfor(NOFREEBLOCKS);
        continue;
      }
      remove_buf_from_freelist(found);
      if (found->dwrite) {
        sync_buffer_to_disk(found);
        continue;
      }
      found->busy = true;
      move_buf_to_hashqueue(found, dev, block);
      return found;
    }
  }
}



void brelse(bhead_t *b)
{
  add_buf_to_freelist(b, !b->valid);
  b->busy = false;
  wakeall(BLOCKBUSY);
}



bhead_t *writequeue = NULL;

void buffer_synced(bhead_t *b, int err)
{
  b->dwrite = false;
  b->valid = (err == 0);
  add_buf_to_freelist(b, !b->valid);
}

/**
 * @brief trigger async write of b
 * 
 * @param b 
 */
void sync_buffer_to_disk(bhead_t *b)
{
  if (writequeue)
    writequeue->fprev = b;
  writequeue = b;
}

/**
 * @brief trigger async load of b
 * 
 * @param b 
 */
void sync_buffer_from_disk(bhead_t *b)
{

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



void bwrite(bhead_t *b)
{
  b->written = false;
  if (!b->dwrite) {
    sync_buffer_to_disk(b);
    while (!b->written)
      waitfor(BLOCKWRITE); 
  }
}