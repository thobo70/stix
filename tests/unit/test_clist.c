/**
 * @file test_clist.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Basic character list unit tests for STIX
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file contains basic unit tests for the character list (clist)
 * data structure including fundamental operations.
 */

#include "../common/test_common.h"

/**
 * @brief Test basic clist operations
 * 
 * This test validates fundamental clist functionality including
 * creation, push/pop operations, and size tracking.
 */
void test_clist_pass(void) {
  byte_t i = clist_create();
  CU_ASSERT_NOT_EQUAL_FATAL(i, 0);
  CU_ASSERT_EQUAL(clist_size(i), 0);
  
  char buf[100];
  for (int j = 0; j < 100; j++) {
    buf[j] = j;
  }
  
  CU_ASSERT_EQUAL(clist_push(i, buf, 100), 0);
  CU_ASSERT_EQUAL(clist_size(i), 100);
  CU_ASSERT_EQUAL(clist_pop(i, buf, 100), 0);
  CU_ASSERT_EQUAL(clist_size(i), 0);
  CU_ASSERT_EQUAL(clist_push(i, buf, 100), 0);
  CU_ASSERT_EQUAL(clist_size(i), 100);
  CU_ASSERT_EQUAL(clist_pop(i, buf, 100), 0);
  CU_ASSERT_EQUAL(clist_size(i), 0);
  
  for (int j = 0; j < 100; j++) {
    CU_ASSERT_EQUAL(buf[j], j);
  }
  
  CU_ASSERT_EQUAL(clist_push(i, buf, 100), 0);
  CU_ASSERT_EQUAL(clist_size(i), 100);
  clist_destroy(i);
}

// Function available for external test runner
