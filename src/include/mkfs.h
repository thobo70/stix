/**
 * @file mkfs.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Make filesystem module
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * This module provides filesystem creation functionality using simple
 * sector read/write interfaces. It can be used both in unit tests and 
 * standalone executables.
 */

#ifndef _MKFS_H
#define _MKFS_H

#include "tdefs.h"

/** Block size in bytes (same as BLOCKSIZE from buf.h) */
#define MKFS_BLOCKSIZE 512

/** Return codes for mkfs operations */
typedef enum mkfs_result_t {
    MKFS_OK = 0,                  ///< Operation successful
    MKFS_ERR_READ_FAILED = 1,     ///< Sector read failed
    MKFS_ERR_WRITE_FAILED = 2,    ///< Sector write failed
    MKFS_ERR_INVALID_SIZE = 3,    ///< Invalid filesystem size
    MKFS_ERR_INVALID_INODES = 4,  ///< Invalid number of inodes
    MKFS_ERR_TOO_SMALL = 5,       ///< Filesystem too small for requested inodes
    MKFS_ERR_NULL_POINTER = 6     ///< Null pointer passed to function
} mkfs_result_t;

/** Filesystem creation parameters */
typedef struct mkfs_params_t {
    word_t total_sectors;         ///< Total number of sectors
    word_t num_inodes;           ///< Number of inodes (0 = auto-calculate)
    word_t calculated_inodes;    ///< Actual number of inodes (after calculation)
    word_t inode_sectors;        ///< Number of sectors for inode table
    word_t bitmap_sectors;       ///< Number of sectors for block bitmap
    word_t first_data_sector;    ///< First sector available for data
    word_t data_sectors;         ///< Number of data sectors available
} mkfs_params_t;

/**
 * @brief Sector read function pointer type
 * 
 * This function should read a sector from the storage device.
 * 
 * @param sector_number The sector number to read
 * @param buffer Buffer to store the sector data (must be at least MKFS_BLOCKSIZE bytes)
 * @return 0 on success, non-zero error code on failure (like errno)
 */
typedef int (*mkfs_read_sector_fn)(block_t sector_number, byte_t *buffer);

/**
 * @brief Sector write function pointer type
 * 
 * This function should write a sector to the storage device.
 * 
 * @param sector_number The sector number to write
 * @param buffer Buffer containing the sector data (must be at least MKFS_BLOCKSIZE bytes)
 * @return 0 on success, non-zero error code on failure (like errno)
 */
typedef int (*mkfs_write_sector_fn)(block_t sector_number, const byte_t *buffer);

/**
 * @brief Initialize the make filesystem module
 * 
 * @param read_fn Function pointer to sector read function
 * @param write_fn Function pointer to sector write function
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_init(mkfs_read_sector_fn read_fn, mkfs_write_sector_fn write_fn);

/**
 * @brief Calculate filesystem layout parameters
 * 
 * This function calculates the layout of the filesystem including
 * inode table size, bitmap size, and data area.
 * 
 * @param total_sectors Total number of sectors available
 * @param num_inodes Number of inodes requested (0 = auto-calculate)
 * @param params Pointer to parameters structure to fill
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_calculate_layout(word_t total_sectors, word_t num_inodes, mkfs_params_t *params);

/**
 * @brief Create a new filesystem
 * 
 * This function creates a complete filesystem with superblock,
 * inode table, bitmap, and root directory.
 * 
 * @param params Filesystem parameters (should be calculated first)
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_create_filesystem(const mkfs_params_t *params);

/**
 * @brief Create and write the superblock
 * 
 * @param params Filesystem parameters
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_create_superblock(const mkfs_params_t *params);

/**
 * @brief Initialize the inode table
 * 
 * @param params Filesystem parameters
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_create_inode_table(const mkfs_params_t *params);

/**
 * @brief Initialize the block bitmap
 * 
 * @param params Filesystem parameters
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_create_bitmap(const mkfs_params_t *params);

/**
 * @brief Create the root directory
 * 
 * @param params Filesystem parameters
 * @return MKFS_OK on success, error code on failure
 */
mkfs_result_t mkfs_create_root_directory(const mkfs_params_t *params);

/**
 * @brief Get error message string for a result code
 * 
 * @param result Result code
 * @return Human-readable error message
 */
const char *mkfs_get_error_message(mkfs_result_t result);

/**
 * @brief Calculate reasonable number of inodes for given sectors
 * 
 * This function calculates a reasonable number of inodes based on
 * filesystem size, using common filesystem heuristics.
 * 
 * @param total_sectors Total number of sectors in filesystem
 * @return Recommended number of inodes
 */
word_t mkfs_calculate_inodes(word_t total_sectors);

#endif /* _MKFS_H */
