/**
 * @file mkfs.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Make filesystem module implementation
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>
#include "../include/mkfs.h"
#include "../include/blocks.h"  // For STIX_MAGIC_NUMBER and other constants
#include "../include/buf.h"
#include "../include/inode.h"
#include "../include/fs.h"

/** Default ratio: one inode per 4 data blocks */
#define MKFS_DEFAULT_INODE_RATIO 4
#define MKFS_DEFAULT_INODE_RATIO 4

/** Minimum number of inodes */
#define MKFS_MIN_INODES 16

/** Maximum number of inodes */
#define MKFS_MAX_INODES 32768

/** Global read/write function pointers */
static mkfs_read_sector_fn g_read_sector = NULL;
static mkfs_write_sector_fn g_write_sector = NULL;

/** Internal buffer for sector operations */
static byte_t g_sector_buffer[MKFS_BLOCKSIZE];

/**
 * @brief Initialize the make filesystem module
 */
mkfs_result_t mkfs_init(mkfs_read_sector_fn read_fn, mkfs_write_sector_fn write_fn) {
    if (read_fn == NULL || write_fn == NULL) {
        g_read_sector = NULL;
        g_write_sector = NULL;
        return MKFS_ERR_NULL_POINTER;
    }
    
    g_read_sector = read_fn;
    g_write_sector = write_fn;
    return MKFS_OK;
}

/**
 * @brief Get error message string for a result code
 */
const char *mkfs_get_error_message(mkfs_result_t result) {
    switch (result) {
        case MKFS_OK:
            return "Filesystem created successfully";
        case MKFS_ERR_READ_FAILED:
            return "Sector read failed";
        case MKFS_ERR_WRITE_FAILED:
            return "Sector write failed";
        case MKFS_ERR_INVALID_SIZE:
            return "Invalid filesystem size";
        case MKFS_ERR_INVALID_INODES:
            return "Invalid number of inodes";
        case MKFS_ERR_TOO_SMALL:
            return "Filesystem too small for requested inodes";
        case MKFS_ERR_NULL_POINTER:
            return "Null pointer error";
        default:
            return "Unknown error";
    }
}

/**
 * @brief Calculate reasonable number of inodes for given sectors
 */
word_t mkfs_calculate_inodes(word_t total_sectors) {
    // Start with a ratio-based calculation
    word_t calculated = total_sectors / MKFS_DEFAULT_INODE_RATIO;
    
    // Apply minimum and maximum bounds
    if (calculated < MKFS_MIN_INODES) {
        calculated = MKFS_MIN_INODES;
    }
    if (calculated > MKFS_MAX_INODES) {
        calculated = MKFS_MAX_INODES;
    }
    
    // Make sure we don't exceed what can fit in the filesystem
    word_t inodes_per_sector = MKFS_BLOCKSIZE / sizeof(dinode_t);
    word_t max_inodes_for_size = (total_sectors - 3) * inodes_per_sector / 2; // rough estimate
    
    if (calculated > max_inodes_for_size) {
        calculated = max_inodes_for_size;
    }
    
    return calculated;
}

/**
 * @brief Calculate filesystem layout parameters
 */
mkfs_result_t mkfs_calculate_layout(word_t total_sectors, word_t num_inodes, mkfs_params_t *params) {
    if (params == NULL) {
        return MKFS_ERR_NULL_POINTER;
    }
    
    if (total_sectors < 4) {  // Need at least superblock + inode + bitmap + data
        return MKFS_ERR_INVALID_SIZE;
    }
    
    // Initialize parameters
    memset(params, 0, sizeof(mkfs_params_t));
    params->total_sectors = total_sectors;
    
    // Calculate number of inodes
    if (num_inodes == 0) {
        params->num_inodes = mkfs_calculate_inodes(total_sectors);
        params->calculated_inodes = params->num_inodes;
    } else {
        if (num_inodes < MKFS_MIN_INODES || num_inodes > MKFS_MAX_INODES) {
            return MKFS_ERR_INVALID_INODES;
        }
        params->num_inodes = num_inodes;
        params->calculated_inodes = num_inodes;
    }
    
    // Calculate inode table size
    word_t inodes_per_sector = MKFS_BLOCKSIZE / sizeof(dinode_t);
    params->inode_sectors = (params->num_inodes + inodes_per_sector - 1) / inodes_per_sector;
    
    // Calculate bitmap size (one bit per block)
    word_t bits_per_sector = MKFS_BLOCKSIZE * 8;
    params->bitmap_sectors = (total_sectors + bits_per_sector - 1) / bits_per_sector;
    
    // Calculate layout
    // Sector 0: unused/reserved
    // Sector 1: superblock
    // Sector 1+: inode table
    // Sector 1+inode_sectors: bitmap
    // Sector 1+inode_sectors+bitmap_sectors: first data block
    
    params->first_data_sector = 1 + params->inode_sectors + params->bitmap_sectors;
    
    if (params->first_data_sector >= total_sectors) {
        return MKFS_ERR_TOO_SMALL;
    }
    
    params->data_sectors = total_sectors - params->first_data_sector;
    
    return MKFS_OK;
}

/**
 * @brief Create and write the superblock
 */
mkfs_result_t mkfs_create_superblock(const mkfs_params_t *params) {
    if (g_write_sector == NULL || params == NULL) {
        return MKFS_ERR_NULL_POINTER;
    }
    
    // Clear the buffer
    memset(g_sector_buffer, 0, MKFS_BLOCKSIZE);
    
    // Create superblock
    superblock_t *sb = (superblock_t *)g_sector_buffer;
    sb->magic = STIX_MAGIC_LE;  // Store magic number in little-endian format
    sb->type = STIX_TYPE;
    sb->version = STIX_VERSION;
    sb->notclean = 0;
    sb->inodes = 2;  // Inode table starts at sector 2 (after superblock at sector 1)
    sb->bbitmap = 2 + params->inode_sectors;  // Bitmap after inode table
    sb->firstblock = params->first_data_sector;
    sb->ninodes = params->num_inodes;
    sb->nblocks = params->total_sectors;
    
    // Write superblock to sector 1 (STIX filesystem layout)
    int result = g_write_sector(1, g_sector_buffer);
    if (result != 0) {
        return MKFS_ERR_WRITE_FAILED;
    }
    
    return MKFS_OK;
}

/**
 * @brief Initialize the inode table
 */
mkfs_result_t mkfs_create_inode_table(const mkfs_params_t *params) {
    if (g_write_sector == NULL || params == NULL) {
        return MKFS_ERR_NULL_POINTER;
    }
    
    word_t inodes_per_sector = MKFS_BLOCKSIZE / sizeof(dinode_t);
    
    for (word_t sector = 0; sector < params->inode_sectors; sector++) {
        // Clear the buffer
        memset(g_sector_buffer, 0, MKFS_BLOCKSIZE);
        
        dinode_t *inodes = (dinode_t *)g_sector_buffer;
        word_t sector_start_inode = sector * inodes_per_sector;
        
        // Initialize inodes in this sector
        for (word_t i = 0; i < inodes_per_sector && (sector_start_inode + i) < params->num_inodes; i++) {
            dinode_t *inode = &inodes[i];
            
            // Most inodes are free (IFREE)
            inode->ftype = IFREE;
            inode->uid = 0;
            inode->gid = 0;
            inode->fmode = 0;
            inode->tmod = 0;
            inode->tinode = 0;
            inode->nlinks = 0;
            inode->fsize = 0;
            memset(inode->blockrefs, 0, sizeof(inode->blockrefs));
            
            // Root directory is inode 0
            if (sector_start_inode + i == 0) {
                inode->ftype = DIRECTORY;
                inode->fmode = 0755;  // rwxr-xr-x
                inode->nlinks = 2;    // . and ..
                inode->fsize = MKFS_BLOCKSIZE;
                inode->blockrefs[0] = params->first_data_sector;  // Root dir gets first data block
            }
        }
        
        // Write this sector of the inode table
        int result = g_write_sector(1 + sector, g_sector_buffer);
        if (result != 0) {
            return MKFS_ERR_WRITE_FAILED;
        }
    }
    
    return MKFS_OK;
}

/**
 * @brief Initialize the block bitmap
 */
mkfs_result_t mkfs_create_bitmap(const mkfs_params_t *params) {
    if (g_write_sector == NULL || params == NULL) {
        return MKFS_ERR_NULL_POINTER;
    }
    
    word_t bits_per_sector = MKFS_BLOCKSIZE * 8;
    word_t bitmap_start = 1 + params->inode_sectors;
    
    for (word_t sector = 0; sector < params->bitmap_sectors; sector++) {
        // Clear the buffer
        memset(g_sector_buffer, 0, MKFS_BLOCKSIZE);
        
        word_t sector_start_bit = sector * bits_per_sector;
        
        // Mark blocks as used or free
        for (word_t bit = 0; bit < bits_per_sector && (sector_start_bit + bit) < params->total_sectors; bit++) {
            word_t block_num = sector_start_bit + bit;
            
            // Mark system blocks as used
            if (block_num < params->first_data_sector || block_num == params->first_data_sector) {
                // Used: superblock, inode table, bitmap, root directory
                word_t byte_index = bit / 8;
                word_t bit_index = bit % 8;
                g_sector_buffer[byte_index] |= (1 << bit_index);
            }
            // All other blocks are free (already zeroed)
        }
        
        // Write this sector of the bitmap
        int result = g_write_sector(bitmap_start + sector, g_sector_buffer);
        if (result != 0) {
            return MKFS_ERR_WRITE_FAILED;
        }
    }
    
    return MKFS_OK;
}

/**
 * @brief Create the root directory
 */
mkfs_result_t mkfs_create_root_directory(const mkfs_params_t *params) {
    if (g_write_sector == NULL || params == NULL) {
        return MKFS_ERR_NULL_POINTER;
    }
    
    // Clear the buffer
    memset(g_sector_buffer, 0, MKFS_BLOCKSIZE);
    
    // Create directory entries
    dirent_t *entries = (dirent_t *)g_sector_buffer;
    
    // "." entry (current directory)
    entries[0].inum = 0;  // Root inode is inode 0
    memcpy(entries[0].name, ".", 2);
    
    // ".." entry (parent directory - same as current for root)
    entries[1].inum = 0;  // Root parent is itself
    memcpy(entries[1].name, "..", 3);
    
    // Write root directory to first data sector
    int result = g_write_sector(params->first_data_sector, g_sector_buffer);
    if (result != 0) {
        return MKFS_ERR_WRITE_FAILED;
    }
    
    return MKFS_OK;
}

/**
 * @brief Create a new filesystem
 */
mkfs_result_t mkfs_create_filesystem(const mkfs_params_t *params) {
    if (g_read_sector == NULL || g_write_sector == NULL || params == NULL) {
        return MKFS_ERR_NULL_POINTER;
    }
    
    mkfs_result_t result;
    
    // Create superblock
    result = mkfs_create_superblock(params);
    if (result != MKFS_OK) {
        return result;
    }
    
    // Create inode table
    result = mkfs_create_inode_table(params);
    if (result != MKFS_OK) {
        return result;
    }
    
    // Create bitmap
    result = mkfs_create_bitmap(params);
    if (result != MKFS_OK) {
        return result;
    }
    
    // Create root directory
    result = mkfs_create_root_directory(params);
    if (result != MKFS_OK) {
        return result;
    }
    
    return MKFS_OK;
}
