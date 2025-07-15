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

#include "buf.h"
#include "inode.h"
#include "tdefs.h"

// Filesystem constants
#define STIX_MAGIC_NUMBER 0x73746978  ///< "stix" in hex - filesystem magic number
#define STIX_VERSION      1           ///< Current filesystem version
#define STIX_TYPE         1           ///< Default filesystem type

// Endian-independent byte order conversion functions
static inline dword_t stix_htole32(dword_t host_32bits) {
    // Convert host byte order to little-endian
    // STIX filesystem always stores multi-byte values in little-endian format
    static const union { 
        unsigned char bytes[4]; 
        dword_t value; 
    } host_order = {{0, 1, 2, 3}};
    
    if (host_order.value == 0x00010203) {
        // Big-endian host - need to swap bytes
        return ((host_32bits & 0xFF000000) >> 24) |
               ((host_32bits & 0x00FF0000) >> 8)  |
               ((host_32bits & 0x0000FF00) << 8)  |
               ((host_32bits & 0x000000FF) << 24);
    } else {
        // Little-endian host - no conversion needed
        return host_32bits;
    }
}

static inline dword_t stix_le32toh(dword_t little_endian_32bits) {
    // Convert little-endian to host byte order
    // Same conversion as htole32 since it's symmetric
    return stix_htole32(little_endian_32bits);
}

// Endian-safe magic number accessors
#define STIX_MAGIC_LE stix_htole32(STIX_MAGIC_NUMBER)  ///< Magic number in little-endian format

#define NFREEINODES 50
#define NFREEBLOCKS 50

#define LDEVFROMFS(fs)  (getisblock(fs)->dev)        ///< ldev of fs from super block
#define LDEVFROMINODE(i)  (getisblock(i->fs)->dev)   ///< ldev of fs from inode


typedef struct superblock {
    dword_t magic;      ///< Filesystem magic number (stored in little-endian format)
    word_t type;
    word_t version;
    word_t notclean : 1;
    block_t inodes;
    block_t bbitmap;
    block_t firstblock;
    ninode_t ninodes;
    block_t nblocks;
} _STRUCTATTR_ superblock_t;

typedef struct isuperblock {
    superblock_t dsblock;
    word_t locked : 1;
    word_t modified : 1;
    word_t inuse : 1;
    int mflags;
    fsnum_t fs;
    ldev_t dev;
    iinode_t *mounted;
    fsnum_t pfs;        // parent fs of mounted fs
    ninode_t pino;      // parent inode of mounting inode (mounted)
    word_t nfinodes;
    ninode_t finode[NFREEINODES];
    ninode_t lastfinode;
    word_t nfblocks;
    block_t fblocks[NFREEBLOCKS];
    block_t lastfblock;
} _STRUCTATTR_ isuperblock_t;

fsnum_t init_isblock(ldev_t dev);

isuperblock_t *getisblock(fsnum_t fs);

/**
 * @brief Validate superblock structure and contents
 * 
 * @param sb pointer to superblock to validate
 * @return int 0 if valid, negative error code if invalid
 */
int validate_superblock(const superblock_t *sb);

bhead_t *balloc(fsnum_t fs);

void bfree(fsnum_t fs, block_t  bl);

#endif
