/**
 * @file blocks.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief handling of blocks on disk
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "blocks.h"
#include "buf.h"
#include "fs.h"
#include "pc.h"
#include "utils.h"


isuperblock_t isblock[MAXFS];



isuperblock_t *getisblock(fsnum_t fs)
{
  ASSERT(fs < MAXFS);
  return &isblock[fs];
}

#define BMAPBLOCK(idx)  (idx / (BLOCKSIZE * 8))
#define BMAPIDX(idx)    ((idx % (BLOCKSIZE * 8)) / 8)
#define BMAPMASK(idx)   (byte_t)(1 << (idx % 8))

bhead_t *balloc(fsnum_t fs)
{
  ASSERT(fs < MAXFS);

  block_t bidx, b = 0;
  int n = 0;
  bhead_t *bh = NULL;
  isuperblock_t *isbk = getisblock(fs);
  while (isbk->locked) {
    waitfor(SBLOCKBUSY);
  }
  isbk->locked = true;

  if ((isbk->nfblocks >= NFREEBLOCKS) || (isbk->fblocks[isbk->nfblocks] == 0)) {
    mset(isbk->fblocks, 0, sizeof(isbk->fblocks));
    for ( bidx = isbk->lastfblock ; bidx < isbk->dsblock.nblocks ; ++bidx ) {
      if (!bh || (b != BMAPBLOCK(bidx))) {
        if (bh) 
          brelse(bh);
        b = BMAPBLOCK(bidx);
        bh = breada(isbk->dev, b + isbk->dsblock.bbitmap, b + 1 + isbk->dsblock.bbitmap);
      }
      if (!(bh->buf->mem[BMAPIDX(bidx)] & BMAPMASK(bidx))) {
        isbk->fblocks[n++] = bidx;
        if (n >= NFREEBLOCKS)
          break;
      }
    }
    if (bh) 
      brelse(bh);
    isbk->nfblocks = 0;
  }

  bidx = isbk->fblocks[isbk->nfblocks];

  if (bidx == 0) {
    isbk->locked = false;
    wakeall(SBLOCKBUSY);
    /// @todo set error no free blocks in fs
    return NULL;
  }

  ++isbk->nfblocks;
  isbk->lastfblock = bidx;
  isbk->locked = false;
  wakeall(SBLOCKBUSY);

  bh = bread(isbk->dev, BMAPBLOCK(bidx) + isbk->dsblock.bbitmap);
  bh->buf->mem[BMAPIDX(bidx)] |= BMAPMASK(bidx);
  bh->dwrite = true;
  bwrite(bh);
  brelse(bh);

  bh = getblk(isbk->dev, bidx);
  mset(bh->buf->mem, 0, sizeof(bh->buf->mem));

  return bh;
}


void bfree(fsnum_t fs, block_t  bl)
{
  ASSERT(fs < MAXFS);
  ASSERT(bl > 0);

  isuperblock_t *isbk = getisblock(fs);
  int i;
  ASSERT(bl < isbk->dsblock.nblocks);
  while (isbk->locked) {
    waitfor(SBLOCKBUSY);
  }
  isbk->locked = true;

  if (isbk->nfblocks >= NFREEBLOCKS) {
    isbk->nfblocks = NFREEBLOCKS - 1;
    isbk->fblocks[isbk->nfblocks] = bl;
  } if (isbk->nfblocks > 0) {
    if (isbk->fblocks[isbk->nfblocks] > bl) {
      isbk->fblocks[--isbk->nfblocks] = bl;
    } else {
      for ( i = isbk->nfblocks + 1 ; i < NFREEBLOCKS ; ++i )
        if ((isbk->fblocks[i] > bl) || (isbk->fblocks[i] == 0)) {
          isbk->fblocks[i] = bl;
          break;
        }
    }
  } else {
      for ( i = 0 ; i < NFREEBLOCKS ; ++i )
        if ((isbk->fblocks[i] > bl) || (isbk->fblocks[i] == 0)) {
          isbk->fblocks[i] = bl;
          break;
        }
  }
  isbk->locked = false;
  wakeall(SBLOCKBUSY);

  bhead_t *bh = bread(isbk->dev, BMAPBLOCK(bl) + isbk->dsblock.bbitmap);
  bh->buf->mem[BMAPIDX(bl)] &= ~BMAPMASK(bl);
  bh->dwrite = true;
  bwrite(bh);
  brelse(bh);
}

