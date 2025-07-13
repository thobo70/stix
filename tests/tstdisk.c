/**
 * @file tstdisk.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#include "buf.h"
#include "inode.h"
#include "blocks.h"
#include "fs.h"
#include "utils.h"
#include "dd.h"
#include "pc.h"
#include <stdlib.h>
#include <string.h>

#define SIMNINODES  (NINODESBLOCK * 2)
#define SIMINODEBLOCKS (SIMNINODES / NINODESBLOCK)

#define SIMNBLOCKS  128
#define SIMBMAPBITS (BLOCKSIZE * 8)
#define SIMBMAPBLOCKS ((SIMNBLOCKS / SIMBMAPBITS) + (((SIMNBLOCKS % SIMBMAPBITS) == 0) ? 0 : 1))

#define SIMNMINOR 2

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

simpart_t *part[SIMNMINOR] =  { NULL, NULL };


void tstdisk_open(ldevminor_t minor);
void tstdisk_close(ldevminor_t minor);
void tstdisk_strategy(ldevminor_t minor, bhead_t *bh);

bdev_t tstdisk = {
  NULL,
  tstdisk_open,
  tstdisk_close,
  tstdisk_strategy
};


char *tstdisk_getblock(ldevminor_t minor, block_t bidx)
{
  ASSERT(minor < SIMNMINOR);
  ASSERT(bidx < SIMNBLOCKS);
  ASSERT(part[minor]);
  return (char*) part[minor]->block[bidx].mem;
}

void testdiskwrite(byte_t *buf, ldevminor_t minor, block_t bidx)
{
  ASSERT(minor < SIMNMINOR);
  ASSERT(bidx < SIMNBLOCKS);
  ASSERT(part[minor]);
  memcpy(part[minor]->block[bidx].mem, buf, BLOCKSIZE);
}

void testdiskread(byte_t *buf, ldevminor_t minor, block_t bidx)
{
  ASSERT(minor < SIMNMINOR);
  ASSERT(bidx < SIMNBLOCKS);
  ASSERT(part[minor]);
  memcpy(buf, part[minor]->block[bidx].mem, BLOCKSIZE);
}


/**
 * @brief open the test disk
 * 
 * @param minor 
 * 
 * @startuml
 * salt
 * {#
 * Block | used for
 * 0 | reserved
 * 1 | superblock
 * 2 | inode 1
 * 3 | inode 2
 * 4 | bitmap
 * 5 | root dir
 * }
 * @enduml
 */
void tstdisk_open(ldevminor_t minor)
{
  ASSERT(minor < 1);
  // Free existing allocation to prevent memory leak
  if (part[minor]) {
    free(part[minor]);
    part[minor] = NULL;
  }
  part[minor] = malloc(sizeof(simpart_t));
  ASSERT(part[minor]);

  memset(part[minor], 0, sizeof(simpart_t));
  part[minor]->fs.sblock.super.version = 1;
  part[minor]->fs.sblock.super.type = 0;
  part[minor]->fs.sblock.super.inodes = 2; // start block of inodes
  part[minor]->fs.sblock.super.bbitmap = part[minor]->fs.sblock.super.inodes + SIMINODEBLOCKS;
  part[minor]->fs.sblock.super.firstblock = part[minor]->fs.sblock.super.bbitmap + SIMBMAPBLOCKS;
  part[minor]->fs.sblock.super.ninodes = SIMNINODES;
  part[minor]->fs.sblock.super.nblocks = SIMNBLOCKS;

  part[minor]->fs.inodes.i[0].ftype = DIRECTORY;
  part[minor]->fs.inodes.i[0].nlinks = 2;
  part[minor]->fs.inodes.i[0].fsize = 2 * sizeof(dirent_t);
  part[minor]->fs.inodes.i[0].blockrefs[0] = part[minor]->fs.sblock.super.firstblock;

  dirent_t *rootdir = (dirent_t*) part[minor]->block[part[minor]->fs.inodes.i[0].blockrefs[0]].mem;
  rootdir[0].inum = 1; // first inode starts with 1 not 0
  strncpy(rootdir[0].name, ".", DIRNAMEENTRY);
  rootdir[1].inum = 1; // first inode starts with 1 not 0
  strncpy(rootdir[1].name, "..", DIRNAMEENTRY);

  byte_t *bmap = part[minor]->block[part[minor]->fs.sblock.super.bbitmap].mem;
  bmap[0] = 0x3F;  // first 6 bits of bitmap are set, 6 blocks are used
}

void tstdisk_close(ldevminor_t minor)
{
  ASSERT(minor < 1);
  if (part[minor]) {
    free(part[minor]);
    part[minor] = NULL;  // Prevent double-free
  }
}

void tstdisk_strategy(ldevminor_t minor, bhead_t *bh)
{
  ASSERT(minor < 1);
  ASSERT(bh);

  int wr = bh->valid;

  if (bh->block >= SIMNBLOCKS) {
    bh->valid = false;
    bh->error = true;
    buffer_synced(bh, 1);
    wakeall(wr ? BLOCKWRITE : BLOCKREAD);
    return;
  }

  if (wr) {
    testdiskwrite(bh->buf->mem, minor, bh->block);
    bh->written = true;
  } else {
    testdiskread(bh->buf->mem, minor, bh->block);
    bh->valid = true;
  }
  
  buffer_synced(bh, 0);

  wakeall(wr ? BLOCKWRITE : BLOCKREAD);
}


