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

struct iinode_t;

#include "buf.h"
#include "inode.h"
#include "tdefs.h"

#define NFREEINODES 50
#define NFREEBLOCKS 50

typedef struct superblock {
    dword_t magic;
    word_t type;
    word_t version;
    word_t notclean : 1;
    block_t inodes;
    block_t bbitmap;
    block_t firstblock;
    ninode_t ninodes;
    block_t nblocks;
} superblock_t;

typedef struct isuperblock {
    superblock_t dsblock;
    word_t locked : 1;
    word_t modified : 1;
    word_t inuse : 1;
    fsnum_t fs;
    ldev_t dev;
    struct iinode_t *mounted;
    fsnum_t pfs;        // parent fs of mounted fs
    ninode_t pino;      // parent inode of mounting inode (mounted)
    word_t nfinodes;
    ninode_t finode[NFREEINODES];
    ninode_t lastfinode;
    word_t nfblocks;
    block_t fblocks[NFREEBLOCKS];
    block_t lastfblock;
} isuperblock_t;

fsnum_t init_isblock(ldev_t dev);

isuperblock_t *getisblock(fsnum_t fs);

bhead_t *balloc(fsnum_t fs);

void bfree(fsnum_t fs, block_t  bl);

#endif
