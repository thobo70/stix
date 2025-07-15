/**
 * @file test_fsck.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Unit tests for filesystem check module
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "../common/test_common.h"
#include "fsck.h"
#include "blocks.h"
#include "inode.h"
#include <string.h>

/** Test data storage for mock sector reads */
static byte_t test_sectors[16][FSCK_BLOCKSIZE];
static int mock_read_fail = 0;  // Flag to simulate read failures

/**
 * @brief Mock sector read function for testing
 */
int mock_read_sector(block_t sector_number, byte_t *buffer) {
    if (mock_read_fail) {
        return 1;  // Simulate read failure
    }
    
    if (sector_number >= 16) {
        return 1;  // Out of bounds
    }
    
    memcpy(buffer, test_sectors[sector_number], FSCK_BLOCKSIZE);
    return 0;  // Success
}

/**
 * @brief Setup function - called before each test
 */
void fsck_setup(void) {
    // Clear all test sectors
    memset(test_sectors, 0, sizeof(test_sectors));
    mock_read_fail = 0;
    
    // Initialize fsck module with mock read function
    fsck_init(mock_read_sector);
}

/**
 * @brief Create a valid superblock for testing
 */
void create_valid_superblock(block_t sector) {
    superblock_t *sb = (superblock_t *)test_sectors[sector];
    
    sb->magic = STIX_MAGIC_LE;  // Store magic in little-endian format
    sb->type = 1;
    sb->version = 1;
    sb->notclean = 0;
    sb->inodes = 1;     // Inode table starts at sector 1
    sb->bbitmap = 2;    // Bitmap starts at sector 2
    sb->firstblock = 3; // First data block at sector 3
    sb->ninodes = 64;   // 64 inodes
    sb->nblocks = 16;   // 16 total blocks
}

/**
 * @brief Create valid inodes for testing
 */
void create_valid_inodes(block_t sector) {
    dinode_t *inodes = (dinode_t *)test_sectors[sector];
    
    // Create a few valid inodes
    inodes[0].ftype = DIRECTORY;     // Root directory
    inodes[0].fmode = 0755;
    inodes[0].nlinks = 2;
    inodes[0].fsize = FSCK_BLOCKSIZE;
    
    inodes[1].ftype = REGULAR;     // Regular file
    inodes[1].fmode = 0644;
    inodes[1].nlinks = 1;
    inodes[1].fsize = 100;
    
    // Rest are unused (nlinks = 0)
}

/**
 * @brief Create a valid bitmap for testing
 */
void create_valid_bitmap(block_t sector) {
    byte_t *bitmap = test_sectors[sector];
    
    // Mark first few blocks as used (superblock, inode table, bitmap, root dir)
    bitmap[0] = 0x0F;  // First 4 blocks used
    // Rest are free (already zeroed)
}

/**
 * @brief Test comprehensive fsck functionality
 * This combines multiple tests into one for the CUNIT_CI_RUN pattern
 */
void test_fsck_pass(void) {
    // Test 1: fsck initialization
    fsck_result_t result = fsck_init(mock_read_sector);
    CU_ASSERT_EQUAL(result, FSCK_OK);
    
    // Test 2: fsck initialization with null pointer
    result = fsck_init(NULL);
    CU_ASSERT_EQUAL(result, FSCK_ERR_NULL_POINTER);
    
    // Re-initialize for subsequent tests
    fsck_init(mock_read_sector);
    
    // Test 3: error message function
    const char *msg = fsck_get_error_message(FSCK_OK);
    CU_ASSERT_PTR_NOT_NULL(msg);
    CU_ASSERT_STRING_EQUAL(msg, "No errors found");
    
    msg = fsck_get_error_message(FSCK_ERR_READ_FAILED);
    CU_ASSERT_PTR_NOT_NULL(msg);
    CU_ASSERT_STRING_EQUAL(msg, "Sector read failed");
    
    // Test 4: statistics reset function
    fsck_stats_t stats;
    stats.total_blocks = 100;
    stats.errors_found = 5;
    
    fsck_reset_stats(&stats);
    CU_ASSERT_EQUAL(stats.total_blocks, 0);
    CU_ASSERT_EQUAL(stats.errors_found, 0);
    
    // Test 5: statistics reset with null pointer (should not crash)
    fsck_reset_stats(NULL);
    CU_PASS("Function handled NULL pointer correctly");
}

/**
 * @brief Test superblock checking functionality
 */
void test_fsck_superblock_pass(void) {
    fsck_setup();
    create_valid_superblock(0);
    
    // Test valid superblock
    fsck_result_t result = fsck_check_superblock(0);
    CU_ASSERT_EQUAL(result, FSCK_OK);
    
    // Test invalid magic
    superblock_t *sb = (superblock_t *)test_sectors[0];
    sb->magic = 0xDEADBEEF;
    result = fsck_check_superblock(0);
    CU_ASSERT_EQUAL(result, FSCK_ERR_INVALID_MAGIC);
    
    // Test read failure
    mock_read_fail = 1;
    result = fsck_check_superblock(0);
    CU_ASSERT_EQUAL(result, FSCK_ERR_READ_FAILED);
    mock_read_fail = 0;
    
    // Test invalid structure
    create_valid_superblock(0);
    sb = (superblock_t *)test_sectors[0];
    sb->ninodes = 0;  // Invalid
    result = fsck_check_superblock(0);
    CU_ASSERT_EQUAL(result, FSCK_ERR_INVALID_SUPERBLOCK);
}

/**
 * @brief Test inode checking functionality
 */
void test_fsck_inodes_pass(void) {
    fsck_setup();
    create_valid_inodes(1);
    
    // Test valid inodes
    fsck_result_t result = fsck_check_inodes(1, 64);
    CU_ASSERT_EQUAL(result, FSCK_OK);
    
    // Test read failure
    mock_read_fail = 1;
    result = fsck_check_inodes(1, 64);
    CU_ASSERT_EQUAL(result, FSCK_ERR_READ_FAILED);
    mock_read_fail = 0;
    
    // Test invalid inode
    create_valid_inodes(1);
    dinode_t *inodes = (dinode_t *)test_sectors[1];
    inodes[2].ftype = 99;  // Invalid file type
    inodes[2].nlinks = 1;  // Make it appear used
    result = fsck_check_inodes(1, 64);
    CU_ASSERT_EQUAL(result, FSCK_ERR_INVALID_INODE);
}

/**
 * @brief Test bitmap checking functionality
 */
void test_fsck_bitmap_pass(void) {
    fsck_setup();
    create_valid_bitmap(2);
    
    // Test valid bitmap
    fsck_result_t result = fsck_check_bitmap(2, 16);
    CU_ASSERT_EQUAL(result, FSCK_OK);
    
    // Test read failure
    mock_read_fail = 1;
    result = fsck_check_bitmap(2, 16);
    CU_ASSERT_EQUAL(result, FSCK_ERR_READ_FAILED);
    mock_read_fail = 0;
}

/**
 * @brief Test complete filesystem checking functionality
 */
void test_fsck_filesystem_pass(void) {
    fsck_setup();
    create_valid_superblock(0);
    create_valid_inodes(1);
    create_valid_bitmap(2);
    
    // Test valid filesystem
    fsck_stats_t stats;
    fsck_result_t result = fsck_check_filesystem(&stats);
    CU_ASSERT_EQUAL(result, FSCK_OK);
    CU_ASSERT_EQUAL(stats.total_blocks, 16);
    CU_ASSERT_EQUAL(stats.total_inodes, 64);
    CU_ASSERT_EQUAL(stats.errors_found, 0);
    
    // Test with corrupted superblock
    superblock_t *sb = (superblock_t *)test_sectors[0];
    sb->magic = 0xDEADBEEF;
    result = fsck_check_filesystem(&stats);
    CU_ASSERT_EQUAL(result, FSCK_ERR_INVALID_MAGIC);
    CU_ASSERT_EQUAL(stats.errors_found, 1);
    
    // Test without statistics
    create_valid_superblock(0);
    result = fsck_check_filesystem(NULL);
    CU_ASSERT_EQUAL(result, FSCK_OK);
    
    // Test check without initialization
    fsck_init(NULL);  // This will set g_read_sector to NULL
    result = fsck_check_filesystem(NULL);
    CU_ASSERT_EQUAL(result, FSCK_ERR_NULL_POINTER);
}


