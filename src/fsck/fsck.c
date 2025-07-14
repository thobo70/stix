/**
 * @file fsck.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Filesystem check module implementation
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "fsck.h"
#include "blocks.h"
#include "inode.h"
#include <string.h>

/** Magic number for filesystem superblock */
#define FSCK_MAGIC_NUMBER 0x73746978  // "stix" in hex

/** Global read function pointer */
static fsck_read_sector_fn g_read_sector = NULL;

/** Internal buffer for sector reads */
static byte_t g_sector_buffer[FSCK_BLOCKSIZE];

/**
 * @brief Initialize the filesystem check module
 */
fsck_result_t fsck_init(fsck_read_sector_fn read_fn) {
    if (read_fn == NULL) {
        g_read_sector = NULL;  // Allow setting to NULL for testing
        return FSCK_ERR_NULL_POINTER;
    }
    
    g_read_sector = read_fn;
    return FSCK_OK;
}

/**
 * @brief Get error message string for a result code
 */
const char *fsck_get_error_message(fsck_result_t result) {
    switch (result) {
        case FSCK_OK:
            return "No errors found";
        case FSCK_ERR_READ_FAILED:
            return "Sector read failed";
        case FSCK_ERR_INVALID_MAGIC:
            return "Invalid superblock magic number";
        case FSCK_ERR_INVALID_SUPERBLOCK:
            return "Invalid superblock structure";
        case FSCK_ERR_INVALID_INODE:
            return "Invalid inode structure";
        case FSCK_ERR_INVALID_BITMAP:
            return "Invalid bitmap structure";
        case FSCK_ERR_INCONSISTENT:
            return "Filesystem inconsistency detected";
        case FSCK_ERR_NULL_POINTER:
            return "Null pointer error";
        default:
            return "Unknown error";
    }
}

/**
 * @brief Reset filesystem check statistics
 */
void fsck_reset_stats(fsck_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    
    stats->total_blocks = 0;
    stats->free_blocks = 0;
    stats->total_inodes = 0;
    stats->free_inodes = 0;
    stats->errors_found = 0;
    stats->warnings_found = 0;
}

/**
 * @brief Check the superblock
 */
fsck_result_t fsck_check_superblock(block_t sector_number) {
    if (g_read_sector == NULL) {
        return FSCK_ERR_NULL_POINTER;
    }
    
    // Read the superblock sector
    int read_result = g_read_sector(sector_number, g_sector_buffer);
    if (read_result != 0) {
        return FSCK_ERR_READ_FAILED;
    }
    
    // Cast buffer to superblock structure
    superblock_t *sb = (superblock_t *)g_sector_buffer;
    
    // Check magic number
    if (sb->magic != FSCK_MAGIC_NUMBER) {
        return FSCK_ERR_INVALID_MAGIC;
    }
    
    // Basic superblock validation
    if (sb->ninodes == 0 || sb->nblocks == 0) {
        return FSCK_ERR_INVALID_SUPERBLOCK;
    }
    
    // Check that bitmap block is reasonable
    if (sb->bbitmap >= sb->nblocks) {
        return FSCK_ERR_INVALID_SUPERBLOCK;
    }
    
    // Check that first data block is reasonable
    if (sb->firstblock >= sb->nblocks) {
        return FSCK_ERR_INVALID_SUPERBLOCK;
    }
    
    // Check that inode block is reasonable
    if (sb->inodes >= sb->nblocks) {
        return FSCK_ERR_INVALID_SUPERBLOCK;
    }
    
    return FSCK_OK;
}

/**
 * @brief Check inode table
 */
fsck_result_t fsck_check_inodes(block_t inode_start_sector, word_t num_inodes) {
    if (g_read_sector == NULL) {
        return FSCK_ERR_NULL_POINTER;
    }
    
    word_t inodes_per_sector = FSCK_BLOCKSIZE / sizeof(dinode_t);
    word_t sectors_needed = (num_inodes + inodes_per_sector - 1) / inodes_per_sector;
    
    for (word_t sector = 0; sector < sectors_needed; sector++) {
        int read_result = g_read_sector(inode_start_sector + sector, g_sector_buffer);
        if (read_result != 0) {
            return FSCK_ERR_READ_FAILED;
        }
        
        dinode_t *inodes = (dinode_t *)g_sector_buffer;
        word_t inodes_in_this_sector = inodes_per_sector;
        
        // Adjust for the last sector which might be partially filled
        if (sector == sectors_needed - 1) {
            word_t remaining_inodes = num_inodes - (sector * inodes_per_sector);
            if (remaining_inodes < inodes_per_sector) {
                inodes_in_this_sector = remaining_inodes;
            }
        }
        
        // Check each inode in this sector
        for (word_t i = 0; i < inodes_in_this_sector; i++) {
            dinode_t *inode = &inodes[i];
            
            // Skip unused inodes (identified by zero nlinks)
            if (inode->nlinks == 0) {
                continue;
            }
            
            // Basic inode validation
            if (inode->ftype > IUNSPEC) {  // Assuming IUNSPEC is the highest valid type
                return FSCK_ERR_INVALID_INODE;
            }
            
            // Check that file size is reasonable for the number of blocks
            if (inode->ftype == REGULAR && inode->fsize > 0) {
                word_t blocks_needed = (inode->fsize + FSCK_BLOCKSIZE - 1) / FSCK_BLOCKSIZE;
                // For simplicity, just check that we don't exceed direct blocks + single indirect
                if (blocks_needed > (NBLOCKREFS + FSCK_BLOCKSIZE / sizeof(block_t))) {
                    return FSCK_ERR_INVALID_INODE;
                }
            }
        }
    }
    
    return FSCK_OK;
}

/**
 * @brief Check bitmap consistency
 */
fsck_result_t fsck_check_bitmap(block_t bitmap_sector, word_t num_blocks) {
    if (g_read_sector == NULL) {
        return FSCK_ERR_NULL_POINTER;
    }
    
    // Calculate how many sectors are needed for the bitmap
    word_t bits_per_sector = FSCK_BLOCKSIZE * 8;
    word_t sectors_needed = (num_blocks + bits_per_sector - 1) / bits_per_sector;
    
    for (word_t sector = 0; sector < sectors_needed; sector++) {
        int read_result = g_read_sector(bitmap_sector + sector, g_sector_buffer);
        if (read_result != 0) {
            return FSCK_ERR_READ_FAILED;
        }
        
        // Basic bitmap validation - check that bits beyond num_blocks are zero
        if (sector == sectors_needed - 1) {
            word_t bits_in_last_sector = num_blocks - (sector * bits_per_sector);
            word_t bytes_used = (bits_in_last_sector + 7) / 8;
            
            // Check that unused bits in the last used byte are zero
            if (bits_in_last_sector % 8 != 0) {
                byte_t last_byte = g_sector_buffer[bytes_used - 1];
                byte_t mask = 0xFF << (bits_in_last_sector % 8);
                if ((last_byte & mask) != 0) {
                    return FSCK_ERR_INVALID_BITMAP;
                }
            }
            
            // Check that completely unused bytes are zero
            for (word_t i = bytes_used; i < FSCK_BLOCKSIZE; i++) {
                if (g_sector_buffer[i] != 0) {
                    return FSCK_ERR_INVALID_BITMAP;
                }
            }
        }
    }
    
    return FSCK_OK;
}

/**
 * @brief Perform a basic filesystem check
 */
fsck_result_t fsck_check_filesystem(fsck_stats_t *stats) {
    if (g_read_sector == NULL) {
        return FSCK_ERR_NULL_POINTER;
    }
    
    // Reset statistics if provided
    if (stats != NULL) {
        fsck_reset_stats(stats);
    }
    
    // Check superblock (assume it's at sector 0)
    fsck_result_t result = fsck_check_superblock(0);
    if (result != FSCK_OK) {
        if (stats != NULL) {
            stats->errors_found++;
        }
        return result;
    }
    
    // Read superblock again to get filesystem parameters
    int read_result = g_read_sector(0, g_sector_buffer);
    if (read_result != 0) {
        return FSCK_ERR_READ_FAILED;
    }
    
    superblock_t *sb = (superblock_t *)g_sector_buffer;
    
    // Update statistics if provided
    if (stats != NULL) {
        stats->total_blocks = sb->nblocks;
        stats->total_inodes = sb->ninodes;
    }
    
    // Check inode table
    result = fsck_check_inodes(sb->inodes, sb->ninodes);
    if (result != FSCK_OK) {
        if (stats != NULL) {
            stats->errors_found++;
        }
        return result;
    }
    
    // Check bitmap
    result = fsck_check_bitmap(sb->bbitmap, sb->nblocks);
    if (result != FSCK_OK) {
        if (stats != NULL) {
            stats->errors_found++;
        }
        return result;
    }
    
    // If we get here, basic checks passed
    return FSCK_OK;
}
