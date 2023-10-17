#ifndef _TESTS1_H
#define _TESTS1_H

#include "buf.h"
#include "inode.h"
#include "blocks.h"
#include "fs.h"

#define SIMNINODES  (NINODESBLOCK * 2)
#define SIMINODEBLOCKS (SIMNINODES / NINODESBLOCK)

#define SIMNBLOCKS  128
#define SIMBMAPBITS (BLOCKSIZE * 8)
#define SIMBMAPBLOCKS ((SIMNBLOCKS / SIMBMAPBITS) + (((SIMNBLOCKS % SIMBMAPBITS) == 0) ? 0 : 1))

typedef struct simfs {
  buffer_t block0;
  union {
    buffer_t b;
    superblock_t super;
  } sblock;
  union {
    buffer_t b[SIMINODEBLOCKS];
    dinode_t i[SIMNINODES];
  } inodes;
  buffer_t bmap[SIMBMAPBLOCKS];
} simfs_t;

typedef union {
  simfs_t fs;
  buffer_t block[SIMNBLOCKS];
} simpart_t;

#endif
