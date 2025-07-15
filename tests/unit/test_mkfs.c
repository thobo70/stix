/**
 * @file test_mkfs.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Unit tests for make filesystem module
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "../common/test_common.h"
#include "mkfs.h"
#include "fsck.h"  // To verify created filesystems
#include "blocks.h"
#include "inode.h"
#include <string.h>

/** Test data storage for mock sector operations */
static byte_t test_sectors[1024][MKFS_BLOCKSIZE];  // Support up to 1024 sectors for testing
static int mock_read_fail = 0;   // Flag to simulate read failures
static int mock_write_fail = 0;  // Flag to simulate write failures

/**
 * @brief Mock sector read function for mkfs testing
 */
int mkfs_mock_read_sector(block_t sector_number, byte_t *buffer) {
    if (mock_read_fail) {
        return 1;  // Simulate read failure
    }
    
    if (sector_number >= 1024) {
        return 1;  // Out of bounds
    }
    
    memcpy(buffer, test_sectors[sector_number], MKFS_BLOCKSIZE);
    return 0;  // Success
}

/**
 * @brief Mock sector write function for mkfs testing
 */
int mkfs_mock_write_sector(block_t sector_number, const byte_t *buffer) {
    if (mock_write_fail) {
        return 1;  // Simulate write failure
    }
    
    if (sector_number >= 1024) {
        return 1;  // Out of bounds
    }
    
    memcpy(test_sectors[sector_number], buffer, MKFS_BLOCKSIZE);
    return 0;  // Success
}

/**
 * @brief Setup function - called before each test
 */
void mkfs_setup(void) {
    // Clear all test sectors
    memset(test_sectors, 0, sizeof(test_sectors));
    mock_read_fail = 0;
    mock_write_fail = 0;
    
    // Initialize mkfs module with mock functions
    mkfs_init(mkfs_mock_read_sector, mkfs_mock_write_sector);
}

/**
 * @brief Test mkfs initialization and basic functionality
 */
void test_mkfs_pass(void) {
    // Test initialization
    mkfs_result_t result = mkfs_init(mkfs_mock_read_sector, mkfs_mock_write_sector);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Test initialization with null pointers
    result = mkfs_init(NULL, mkfs_mock_write_sector);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
    
    result = mkfs_init(mkfs_mock_read_sector, NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
    
    result = mkfs_init(NULL, NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
    
    // Re-initialize for subsequent tests
    mkfs_init(mkfs_mock_read_sector, mkfs_mock_write_sector);
    
    // Test error message function
    const char *msg = mkfs_get_error_message(MKFS_OK);
    CU_ASSERT_PTR_NOT_NULL(msg);
    CU_ASSERT_STRING_EQUAL(msg, "Filesystem created successfully");
    
    msg = mkfs_get_error_message(MKFS_ERR_WRITE_FAILED);
    CU_ASSERT_PTR_NOT_NULL(msg);
    CU_ASSERT_STRING_EQUAL(msg, "Sector write failed");
    
    // Test inode calculation
    word_t inodes = mkfs_calculate_inodes(100);
    CU_ASSERT_TRUE(inodes >= 16);  // Should be at least minimum
    CU_ASSERT_TRUE(inodes <= 100); // Should be reasonable for size
}

/**
 * @brief Test filesystem layout calculation
 */
void test_mkfs_layout_pass(void) {
    mkfs_setup();
    
    mkfs_params_t params;
    
    // Test normal layout calculation
    mkfs_result_t result = mkfs_calculate_layout(100, 0, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    CU_ASSERT_EQUAL(params.total_sectors, 100);
    CU_ASSERT_TRUE(params.calculated_inodes > 0);
    CU_ASSERT_TRUE(params.inode_sectors > 0);
    CU_ASSERT_TRUE(params.bitmap_sectors > 0);
    CU_ASSERT_TRUE(params.first_data_sector < 100);
    CU_ASSERT_TRUE(params.data_sectors > 0);
    
    // Test with specific inode count
    result = mkfs_calculate_layout(100, 64, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    CU_ASSERT_EQUAL(params.num_inodes, 64);
    CU_ASSERT_EQUAL(params.calculated_inodes, 64);
    
    // Test with too small filesystem
    result = mkfs_calculate_layout(2, 0, &params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_INVALID_SIZE);
    
    // Test with invalid inode count
    result = mkfs_calculate_layout(100, 1, &params);  // Too few inodes
    CU_ASSERT_EQUAL(result, MKFS_ERR_INVALID_INODES);
    
    result = mkfs_calculate_layout(100, 40000, &params);  // Too many inodes
    CU_ASSERT_EQUAL(result, MKFS_ERR_INVALID_INODES);
    
    // Test with null pointer
    result = mkfs_calculate_layout(100, 0, NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
    
    // Test filesystem too small for inodes
    result = mkfs_calculate_layout(10, 1000, &params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_TOO_SMALL);
}

/**
 * @brief Test superblock creation
 */
void test_mkfs_superblock_pass(void) {
    mkfs_setup();
    
    mkfs_params_t params;
    mkfs_result_t result = mkfs_calculate_layout(100, 64, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Test superblock creation
    result = mkfs_create_superblock(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify superblock was written correctly
    superblock_t *sb = (superblock_t *)test_sectors[1];  // Superblock is at sector 1
    // Check magic number using endian-safe comparison
    dword_t stored_magic = stix_le32toh(sb->magic);
    CU_ASSERT_EQUAL(stored_magic, STIX_MAGIC_NUMBER);  // Endian-independent comparison
    CU_ASSERT_EQUAL(sb->type, 1);
    CU_ASSERT_EQUAL(sb->version, 1);
    CU_ASSERT_EQUAL(sb->notclean, 0);
    CU_ASSERT_EQUAL(sb->ninodes, 64);
    CU_ASSERT_EQUAL(sb->nblocks, 100);
    CU_ASSERT_EQUAL(sb->inodes, 1);
    CU_ASSERT_TRUE(sb->bbitmap > 1);
    CU_ASSERT_TRUE(sb->firstblock > sb->bbitmap);
    
    // Test with write failure
    mock_write_fail = 1;
    result = mkfs_create_superblock(&params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_WRITE_FAILED);
    mock_write_fail = 0;
    
    // Test with null pointer
    result = mkfs_create_superblock(NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
}

/**
 * @brief Test inode table creation
 */
void test_mkfs_inode_table_pass(void) {
    mkfs_setup();
    
    mkfs_params_t params;
    mkfs_result_t result = mkfs_calculate_layout(100, 32, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Test inode table creation
    result = mkfs_create_inode_table(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify root inode (inode 0) was created correctly
    dinode_t *inodes = (dinode_t *)test_sectors[1];  // Inode table starts at sector 1
    CU_ASSERT_EQUAL(inodes[0].ftype, DIRECTORY);
    CU_ASSERT_EQUAL(inodes[0].fmode, 0755);
    CU_ASSERT_EQUAL(inodes[0].nlinks, 2);
    CU_ASSERT_EQUAL(inodes[0].fsize, MKFS_BLOCKSIZE);
    CU_ASSERT_TRUE(inodes[0].blockrefs[0] > 0);  // Should have a data block
    
    // Verify other inodes are free
    CU_ASSERT_EQUAL(inodes[1].ftype, IFREE);
    CU_ASSERT_EQUAL(inodes[1].nlinks, 0);
    
    // Test with write failure
    mock_write_fail = 1;
    result = mkfs_create_inode_table(&params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_WRITE_FAILED);
    mock_write_fail = 0;
    
    // Test with null pointer
    result = mkfs_create_inode_table(NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
}

/**
 * @brief Test bitmap creation
 */
void test_mkfs_bitmap_pass(void) {
    mkfs_setup();
    
    mkfs_params_t params;
    mkfs_result_t result = mkfs_calculate_layout(64, 32, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Test bitmap creation
    result = mkfs_create_bitmap(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify bitmap was created - system blocks should be marked as used
    word_t bitmap_start = 1 + params.inode_sectors;
    byte_t *bitmap = test_sectors[bitmap_start];
    
    // First few bits should be set (superblock, inode table, bitmap, root dir)
    CU_ASSERT_TRUE((bitmap[0] & 0x01) != 0);  // Superblock (block 0)
    CU_ASSERT_TRUE((bitmap[0] & 0x02) != 0);  // Inode table (block 1)
    
    // Test with write failure
    mock_write_fail = 1;
    result = mkfs_create_bitmap(&params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_WRITE_FAILED);
    mock_write_fail = 0;
    
    // Test with null pointer
    result = mkfs_create_bitmap(NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
}

/**
 * @brief Test root directory creation
 */
void test_mkfs_root_directory_pass(void) {
    mkfs_setup();
    
    mkfs_params_t params;
    mkfs_result_t result = mkfs_calculate_layout(100, 32, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Test root directory creation
    result = mkfs_create_root_directory(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify root directory entries
    dirent_t *entries = (dirent_t *)test_sectors[params.first_data_sector];
    
    // Check "." entry
    CU_ASSERT_EQUAL(entries[0].inum, 0);
    CU_ASSERT_STRING_EQUAL(entries[0].name, ".");
    
    // Check ".." entry
    CU_ASSERT_EQUAL(entries[1].inum, 0);
    CU_ASSERT_STRING_EQUAL(entries[1].name, "..");
    
    // Test with write failure
    mock_write_fail = 1;
    result = mkfs_create_root_directory(&params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_WRITE_FAILED);
    mock_write_fail = 0;
    
    // Test with null pointer
    result = mkfs_create_root_directory(NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
}

/**
 * @brief Test complete filesystem creation and verify with fsck
 */
void test_mkfs_complete_filesystem_pass(void) {
    mkfs_setup();
    
    mkfs_params_t params;
    mkfs_result_t result = mkfs_calculate_layout(128, 64, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Create complete filesystem
    result = mkfs_create_filesystem(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify the created filesystem using fsck
    fsck_result_t fsck_result = fsck_init(mkfs_mock_read_sector);
    CU_ASSERT_EQUAL(fsck_result, FSCK_OK);
    
    fsck_stats_t stats;
    fsck_result = fsck_check_filesystem(&stats);
    CU_ASSERT_EQUAL(fsck_result, FSCK_OK);
    CU_ASSERT_EQUAL(stats.total_blocks, 128);
    CU_ASSERT_EQUAL(stats.total_inodes, 64);
    CU_ASSERT_EQUAL(stats.errors_found, 0);
    
    // Test with null pointer
    result = mkfs_create_filesystem(NULL);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
    
    // Test with uninitialized module
    mkfs_init(NULL, NULL);
    result = mkfs_create_filesystem(&params);
    CU_ASSERT_EQUAL(result, MKFS_ERR_NULL_POINTER);
}

/**
 * @brief Test edge cases and error conditions
 */
void test_mkfs_edge_cases_pass(void) {
    mkfs_setup();
    
    // Test very small filesystem
    mkfs_params_t params;
    mkfs_result_t result = mkfs_calculate_layout(10, 0, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    result = mkfs_create_filesystem(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify it's still valid
    fsck_result_t fsck_result = fsck_init(mkfs_mock_read_sector);
    CU_ASSERT_EQUAL(fsck_result, FSCK_OK);
    
    fsck_stats_t stats;
    fsck_result = fsck_check_filesystem(&stats);
    CU_ASSERT_EQUAL(fsck_result, FSCK_OK);
    
    // Test larger filesystem
    mkfs_setup();
    result = mkfs_calculate_layout(512, 256, &params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    result = mkfs_create_filesystem(&params);
    CU_ASSERT_EQUAL(result, MKFS_OK);
    
    // Verify larger filesystem
    fsck_result = fsck_init(mkfs_mock_read_sector);
    CU_ASSERT_EQUAL(fsck_result, FSCK_OK);
    
    fsck_result = fsck_check_filesystem(&stats);
    CU_ASSERT_EQUAL(fsck_result, FSCK_OK);
    CU_ASSERT_EQUAL(stats.total_blocks, 512);
    CU_ASSERT_EQUAL(stats.total_inodes, 256);
    CU_ASSERT_EQUAL(stats.errors_found, 0);
}
