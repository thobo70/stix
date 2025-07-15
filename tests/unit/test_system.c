/**
 * @file test_system.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief System-level unit tests for STIX
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file contains system-level tests including type size validation
 * and other fundamental system checks.
 */

#include "../common/test_common.h"

/**
 * @brief Test fundamental data type sizes and alignment
 * 
 * This test validates that the fundamental data types used throughout
 * STIX have the expected sizes and alignment. This is critical for
 * ensuring consistent behavior across different architectures.
 */
void test_typesize_pass(void) {
  CU_ASSERT_EQUAL_FATAL(sizeof(byte_t), 1);
  CU_ASSERT_EQUAL_FATAL(sizeof(word_t), 2);
  CU_ASSERT_EQUAL_FATAL(sizeof(dword_t), 4);
  CU_ASSERT_EQUAL_FATAL(BLOCKSIZE % sizeof(dinode_t), 0);
}

// Test functions for system-level tests
// No test arrays needed - functions called directly
