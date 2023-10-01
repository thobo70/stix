/**
 * @file blocks.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief header of blocks handling
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _BLOCKS_H
#define _BLOCKS_H

#include "tdefs.h"

#define NFREEINODES 10
#define NFREEBLOCKS 10

typedef struct superblock {
    word_t type;
    word_t version;
    word_t locked : 1;
    word_t modified : 1;
    word_t notclean : 1;
    block_t inodes;
    block_t ibitmap;
    block_t bbitmap;
    block_t firstblock;
    word_t ninodes;
    word_t nblocks;
} superblock_t;

typedef struct isuperblock {
    superblock_t dsblock;
    ldev_t dev;
    word_t nfinodes;
    word_t finode[NFREEINODES];
    word_t nfblocks;
    word_t fblocks[NFREEBLOCKS];
} isuperblock_t;

void freeblock(fsnum_t fs, block_t  bl);

#endif
