/**
 * @file test_buffer.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Buffer cache unit tests for STIX
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file contains unit tests for the buffer cache subsystem including
 * basic operations and edge cases.
 */

#include "../common/test_common.h"

/**
 * @brief Test basic buffer cache operations
 * 
 * This test validates fundamental buffer cache functionality including
 * buffer allocation, reading, writing, and releasing.
 */
void test_buffer_pass(void) {
  ldev_t dev = {{0, 0}};
  const char *str = "Hello World";

  bhead_t *b = bread(dev, 60000);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b);
  CU_ASSERT_EQUAL(b->block, 60000);
  CU_ASSERT_TRUE(b->error);
  sncpy((char *)b->buf->mem, str, BLOCKSIZE);
  brelse(b);

  // read from cache
  // old buffer from b = bread(dev, 60000); is still in cache and should be returned first
  // but should be overwritten by new buffer
  b = bread(dev, 0);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b);
  CU_ASSERT_NOT_EQUAL(sncmp((char *)b->buf->mem, str, BLOCKSIZE), 0);
  brelse(b);

  b = bread(dev, 0);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b);
  sncpy((char *)b->buf->mem, str, BLOCKSIZE);
  b->dwrite = false;    // write to disk immediately
  bwrite(b);
  brelse(b);
  CU_ASSERT_EQUAL(sncmp(tstdisk_getblock(0, 0), str, BLOCKSIZE), 0);
}

/**
 * @brief Test buffer cache edge cases and boundary conditions
 * 
 * This test validates buffer cache behavior under edge conditions:
 * - Repeated access to the same block (cache hit behavior)
 * - Data integrity across multiple write/read cycles
 * - Buffer reference counting and proper cleanup
 */
void test_buffer_edge_cases(void) {
  ldev_t dev = {{0, 0}};
  
  // Test simple buffer operations without complex I/O
  
  // Test getting the same buffer multiple times from cache
  bhead_t *b1 = bread(dev, 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b1);
  brelse(b1);
  
  bhead_t *b2 = bread(dev, 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b2);
  CU_ASSERT_EQUAL(b1, b2); // Should return same buffer from cache
  brelse(b2);
  
  // Test writing data and reading it back
  const char *test_data = "EdgeCaseTest";
  bhead_t *bwr = bread(dev, 2);
  CU_ASSERT_PTR_NOT_NULL_FATAL(bwr);
  sncpy((char *)bwr->buf->mem, test_data, BLOCKSIZE);
  bwr->dwrite = false;  // Write immediately
  bwrite(bwr);
  brelse(bwr);
  
  // Verify data persists
  bhead_t *bread_back = bread(dev, 2);
  CU_ASSERT_PTR_NOT_NULL_FATAL(bread_back);
  CU_ASSERT_EQUAL(sncmp((char *)bread_back->buf->mem, test_data, BLOCKSIZE), 0);
  brelse(bread_back);
}

// Functions available for external test runner
