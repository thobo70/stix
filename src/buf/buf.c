/**
 * @file buf.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief buffer handling
 * @version 0.1
 * @date 2023-09-29
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
  ASSERT(b);
check_bfreelist();
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
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
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
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



void check_bfreelist(void)
{
  kprintf("#19 : %p %p\n", bufhead[19].hprev, bufhead[19].hnext);
  for ( int i = 0 ; i < NBUFFER ; ++i ) {
    bhead_t *b = &bufhead[i];
    ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  }
}



void init_buffers(void)
{
  mset(bufhead, 0, sizeof(bufhead));
  mset(hashtab, 0, sizeof(hashtab));

  for ( int i = 0 ; i < NBUFFER ; ++i ) {
    bufhead[i].buf = &buf[i];
    add_buf_to_freelist(&bufhead[i], false);
  }
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
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  bhead_t *p = HTAB(dev, block);

  b->valid = false;
  b->error = false;
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
check_bfreelist();
  if (p) {
    b->hprev = p->hprev;
    b->hnext = p;
    p->hprev->hnext = b;
    p->hprev = b;
check_bfreelist();
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
 *       : sleep(event\nbuffer becomes free);
 *     else (no)
 *       : mark buffer\nas busy;
 *       : remove buffer\nfrom free list;
 *       : return buffer;
 *     stop
 *     endif
 *   else (no)
 *     if (freelist empty) then (yes)
 *       : sleep(event\nany buffer\ngets free);
 *     else (no)
 *       : remove buffer\nfrom free list;
 *       if (buffer marked for\ndelayed write) then (yes)
 *         : async write buffer;
 *       else (no)
 *         : move buffer\nfrom old to\nnew hash queue;
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
    for ( found = HTAB(dev, block) ; found ; found = (found->hnext == HTAB(dev, block)) ? NULL : found->hnext )
      if ((found->dev.ldev == dev.ldev) && (found->block == block))
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


/**
 * @brief release buffer
 * 
 * @param b 
 */
void brelse(bhead_t *b)
{
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  add_buf_to_freelist(b, !b->valid);
  b->busy = false;
  wakeall(BLOCKBUSY);
}



void buffer_synced(bhead_t *b, int err)
{
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  b->dwrite = false;
  
  if (!b->busy)
    add_buf_to_freelist(b, err != 0);
  
  if (b->valid)
    wakeall(BLOCKREAD);
  else
    wakeall(BLOCKWRITE);  
  
  b->valid = (err == 0);
}

/**
 * @brief trigger async write of b
 * 
 * @param b 
 */
void sync_buffer_to_disk(bhead_t *b)
{
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  ASSERT(b->error == false);
  ASSERT(b->valid);
  ASSERT(b->written == false);
  // ASSERT(b->busy == true);       /// @todo why is this not true?
  ASSERT(b->infreelist == false);
  bdevstrategy(b->dev, b);
}

/**
 * @brief trigger async load of b
 * 
 * @param b 
 */
void sync_buffer_from_disk(bhead_t *b)
{
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  ASSERT(b->error == false);
  ASSERT(b->valid == false);
  ASSERT(b->busy == true);
  ASSERT(b->infreelist == false);
  bdevstrategy(b->dev, b);
}


/**
 * @brief read block into a buffer
 * 
 * @param dev 
 * @param block 
 * @return bhead_t* 
 */
bhead_t *bread(ldev_t dev, block_t block)
{
  bhead_t *b = getblk(dev, block);

  if (b->valid)
    return b;

  sync_buffer_from_disk(b);
  while (!b->valid && !b->error)
    waitfor(BLOCKREAD);

  return b;
}



/**
 * @brief reads bl1 and pre loads bl2
 * 
 * @param dev 
 * @param bl1 
 * @param bl2 
 * @return bhead_t*   buffer for bl1
 */
bhead_t *breada(ldev_t dev, block_t bl1, block_t bl2)
{
  bhead_t *b1, *b2;

  b1 = getblk(dev, bl1);
  if (!b1->valid)
    sync_buffer_from_disk(b1);

  b2 = getblk(dev, bl2);
  if (!b2->valid)
    sync_buffer_from_disk(b2);

  while (!b1->valid && !b1->error)
    waitfor(BLOCKREAD);

  brelse(b2);   /// @bug potential race condition

  return b1;
}



/**
 * @brief write block from buffer
 * 
 * @param b 
 */
void bwrite(bhead_t *b)
{
  ASSERT(b);
  ASSERT(((b->hnext != NULL) ? b->hprev != NULL : b->hprev == NULL));
  ASSERT(b->valid);     /// buffer must be valid
  b->written = false;
  if (!b->dwrite) {
    sync_buffer_to_disk(b);
    while (!b->written && !b->error)
      waitfor(BLOCKWRITE); 
  }
}
