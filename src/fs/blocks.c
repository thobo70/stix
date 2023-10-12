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
#include "fs.h"
#include "utils.h"


isuperblock_t isblock[MAXFS];


isuperblock_t *getisblock(fsnum_t fs)
{
  ASSERT(fs < MAXFS);
  return &isblock[fs];
}


void freeblock(fsnum_t fs, block_t  bl)
{
  ASSERT(fs < MAXFS);
  ASSERT(bl > 0);
  /// TODO set bit for block bl in bitfield of fs to 0
}

