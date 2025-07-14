/**
 * @file fsck.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Filesystem check module
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * This module provides filesystem checking functionality using a simple
 * sector read interface. It can be used both in unit tests and standalone
 * executables.
 */

#ifndef _FSCK_H
#define _FSCK_H

#include "tdefs.h"

/** Block size in bytes (same as BLOCKSIZE from buf.h) */
#define FSCK_BLOCKSIZE 512

/** Return codes for fsck operations */
typedef enum fsck_result_t {
    FSCK_OK = 0,                  ///< Operation successful
    FSCK_ERR_READ_FAILED = 1,     ///< Sector read failed
    FSCK_ERR_INVALID_MAGIC = 2,   ///< Invalid superblock magic number
    FSCK_ERR_INVALID_SUPERBLOCK = 3,  ///< Invalid superblock structure
    FSCK_ERR_INVALID_INODE = 4,   ///< Invalid inode structure
    FSCK_ERR_INVALID_BITMAP = 5,  ///< Invalid bitmap structure
    FSCK_ERR_INCONSISTENT = 6,    ///< Filesystem inconsistency detected
    FSCK_ERR_NULL_POINTER = 7     ///< Null pointer passed to function
} fsck_result_t;

/** Filesystem check statistics */
typedef struct fsck_stats_t {
    word_t total_blocks;          ///< Total number of blocks
    word_t free_blocks;           ///< Number of free blocks
    word_t total_inodes;          ///< Total number of inodes
    word_t free_inodes;           ///< Number of free inodes
    word_t errors_found;          ///< Number of errors found
    word_t warnings_found;        ///< Number of warnings found
} fsck_stats_t;

/**
 * @brief Sector read function pointer type
 * 
 * This function should read a sector from the storage device.
 * 
 * @param sector_number The sector number to read
 * @param buffer Buffer to store the sector data (must be at least FSCK_BLOCKSIZE bytes)
 * @return 0 on success, non-zero error code on failure (like errno)
 */
typedef int (*fsck_read_sector_fn)(block_t sector_number, byte_t *buffer);

/**
 * @brief Initialize the filesystem check module
 * 
 * @param read_fn Function pointer to sector read function
 * @return FSCK_OK on success, error code on failure
 */
fsck_result_t fsck_init(fsck_read_sector_fn read_fn);

/**
 * @brief Perform a basic filesystem check
 * 
 * This function performs a basic check of the filesystem structure,
 * including superblock validation, inode checks, and bitmap consistency.
 * 
 * @param stats Pointer to statistics structure to fill (can be NULL)
 * @return FSCK_OK if filesystem is clean, error code if problems found
 */
fsck_result_t fsck_check_filesystem(fsck_stats_t *stats);

/**
 * @brief Check the superblock
 * 
 * @param sector_number Sector number where superblock is located
 * @return FSCK_OK if superblock is valid, error code otherwise
 */
fsck_result_t fsck_check_superblock(block_t sector_number);

/**
 * @brief Check inode table
 * 
 * @param inode_start_sector First sector of inode table
 * @param num_inodes Number of inodes to check
 * @return FSCK_OK if all inodes are valid, error code otherwise
 */
fsck_result_t fsck_check_inodes(block_t inode_start_sector, word_t num_inodes);

/**
 * @brief Check bitmap consistency
 * 
 * @param bitmap_sector Sector number where bitmap is located
 * @param num_blocks Number of blocks covered by bitmap
 * @return FSCK_OK if bitmap is consistent, error code otherwise
 */
fsck_result_t fsck_check_bitmap(block_t bitmap_sector, word_t num_blocks);

/**
 * @brief Get error message string for a result code
 * 
 * @param result Result code
 * @return Human-readable error message
 */
const char *fsck_get_error_message(fsck_result_t result);

/**
 * @brief Reset filesystem check statistics
 * 
 * @param stats Pointer to statistics structure to reset
 */
void fsck_reset_stats(fsck_stats_t *stats);

#endif /* _FSCK_H */
