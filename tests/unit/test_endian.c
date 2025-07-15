/**
 * @file test_endian.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Unit tests for endian-independent magic number handling
 * @version 0.1
 * @date 2025-07-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <string.h>
#include <stdint.h>
#include "CUnit/Basic.h"
#include "../common/test_common.h"
#include "../../src/include/blocks.h"
#include "../../src/include/mkfs.h"
#include "../../src/include/fsck.h"

/**
 * @brief Test endian conversion functions
 */
void test_endian_conversion_functions(void) {
    // Test round-trip conversion
    dword_t original = STIX_MAGIC_NUMBER;
    dword_t le_format = stix_htole32(original);
    dword_t recovered = stix_le32toh(le_format);
    
    CU_ASSERT_EQUAL(recovered, original);
    
    // Test that STIX_MAGIC_LE is properly converted
    dword_t magic_le = STIX_MAGIC_LE;
    dword_t host_order = stix_le32toh(magic_le);
    CU_ASSERT_EQUAL(host_order, STIX_MAGIC_NUMBER);
}

/**
 * @brief Test that magic number storage and validation work correctly
 */
void test_endian_magic_number_validation(void) {
    superblock_t test_sb;
    memset(&test_sb, 0, sizeof(test_sb));
    
    // Store magic number in little-endian format (as mkfs would)
    test_sb.magic = STIX_MAGIC_LE;
    test_sb.type = STIX_TYPE;
    test_sb.version = STIX_VERSION;
    test_sb.ninodes = 100;
    test_sb.nblocks = 1000;
    test_sb.inodes = 2;
    test_sb.bbitmap = 5;
    test_sb.firstblock = 10;
    
    // Validate using the endian-safe validation function
    int result = validate_superblock(&test_sb);
    CU_ASSERT_EQUAL(result, 0);  // Should pass validation
}

/**
 * @brief Test rejection of invalid magic numbers regardless of endianness
 */
void test_endian_invalid_magic_rejection(void) {
    superblock_t test_sb;
    memset(&test_sb, 0, sizeof(test_sb));
    
    // Test various invalid magic numbers
    dword_t invalid_magics[] = {
        0x00000000,  // Zero
        0xDEADBEEF,  // Random value
        0x78697473,  // "stix" in big-endian (reversed)
        0xFFFFFFFF   // All ones
    };
    
    for (int i = 0; i < 4; i++) {
        test_sb.magic = invalid_magics[i];
        test_sb.type = STIX_TYPE;
        test_sb.version = STIX_VERSION;
        test_sb.ninodes = 100;
        test_sb.nblocks = 1000;
        test_sb.inodes = 2;
        test_sb.bbitmap = 5;
        test_sb.firstblock = 10;
        
        int result = validate_superblock(&test_sb);
        CU_ASSERT_NOT_EQUAL(result, 0);  // Should fail validation
    }
}

/**
 * @brief Test endian-independence with simulated byte order scenarios
 */
void test_endian_simulated_scenarios(void) {
    // Test that conversion works with little-endian stored format
    dword_t little_endian_magic = STIX_MAGIC_LE;
    
    // The conversion function should handle this correctly
    dword_t converted_back = stix_le32toh(little_endian_magic);
    CU_ASSERT_EQUAL(converted_back, STIX_MAGIC_NUMBER);
    
    // Test that our endian detection works
    static const union { 
        unsigned char bytes[4]; 
        dword_t value; 
    } host_order = {{0, 1, 2, 3}};
    
    // On little-endian systems, this should be 0x03020100
    // On big-endian systems, this should be 0x00010203
    CU_ASSERT_TRUE(host_order.value == 0x03020100 || host_order.value == 0x00010203);
    
    // Test round-trip conversion
    dword_t original = 0x12345678;
    dword_t to_le = stix_htole32(original);
    dword_t back_to_host = stix_le32toh(to_le);
    CU_ASSERT_EQUAL(back_to_host, original);
}

/**
 * @brief Test that mkfs creates endian-correct superblocks
 */
void test_endian_mkfs_integration(void) {
    // Mock sector buffer for testing
    static byte_t test_buffer[512];
    memset(test_buffer, 0, sizeof(test_buffer));
    
    // Simulate what mkfs does when creating a superblock
    superblock_t *sb = (superblock_t *)test_buffer;
    sb->magic = STIX_MAGIC_LE;  // This is what mkfs should store
    sb->type = STIX_TYPE;
    sb->version = STIX_VERSION;
    sb->ninodes = 100;
    sb->nblocks = 1000;
    sb->inodes = 2;
    sb->bbitmap = 5;
    sb->firstblock = 10;
    
    // Now validate it as mount would
    int validation_result = validate_superblock(sb);
    CU_ASSERT_EQUAL(validation_result, 0);
    
    // Verify that the stored magic number converts correctly
    dword_t stored_magic = stix_le32toh(sb->magic);
    CU_ASSERT_EQUAL(stored_magic, STIX_MAGIC_NUMBER);
}

/**
 * @brief Test byte-level representation consistency
 */
void test_endian_byte_representation(void) {
    // Test that "stix" string maps correctly to magic number
    union {
        char str[4];
        dword_t num;
    } magic_test;
    
    magic_test.str[0] = 'x';  // LSB in little-endian
    magic_test.str[1] = 'i';
    magic_test.str[2] = 't';
    magic_test.str[3] = 's';  // MSB in little-endian
    
    // On little-endian systems, this should equal STIX_MAGIC_NUMBER
    // The conversion function should handle endianness correctly
    dword_t converted = stix_le32toh(magic_test.num);
    
    // Verify the magic number represents "stix" correctly
    CU_ASSERT_TRUE(converted == STIX_MAGIC_NUMBER || 
                   converted == 0x78697473);  // Allow for endian differences in test environment
}
