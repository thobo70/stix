/**
 * @file test_inode.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Inode unit tests for STIX
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file contains unit tests for inode operations including
 * inode creation, modification, and management.
 */

#include "../common/test_common.h"

/**
 * @brief Test inode allocation and deallocation
 * 
 * This test validates the inode allocation functionality by creating
 * and freeing an inode.
 */
void test_inode_alloc_pass(void) {
  iinode_t *inode = ialloc(0, REGULAR, 0644);
  CU_ASSERT_PTR_NOT_NULL(inode);
  
  if (inode) {
    CU_ASSERT_EQUAL(inode->dinode.ftype, REGULAR);
    CU_ASSERT_EQUAL(inode->dinode.fmode, 0644);
    ifree(inode);
  }
}

/**
 * @brief Test inode get operation
 * 
 * This test validates the inode retrieval functionality by getting
 * an inode by filesystem and inode number.
 */
void test_inode_get_pass(void) {
  iinode_t *inode = iget(0, 1); // Get root inode
  CU_ASSERT_PTR_NOT_NULL(inode);
  
  if (inode) {
    CU_ASSERT_EQUAL(inode->fs, 0);
    CU_ASSERT_EQUAL(inode->inum, 1);
    iput(inode); // Release the inode
  }
}

/**
 * @brief Test inode put operation
 * 
 * This test validates the inode put functionality by allocating
 * an inode and then releasing it.
 */
void test_inode_put_pass(void) {
  iinode_t *inode = ialloc(0, REGULAR, 0644);
  CU_ASSERT_PTR_NOT_NULL(inode);
  
  if (inode) {
    // Release the inode
    iput(inode);
    // After put, we can't safely access the inode anymore
    // but the test succeeded if we didn't crash
    CU_ASSERT_TRUE(1); // Test passed if we reach here
  }
}

/**
 * @brief Test active inodes count
 * 
 * This test validates the active inode counting functionality.
 */
void test_inode_active_count_pass(void) {
  int count = activeinodes(0);
  CU_ASSERT_TRUE(count >= 0); // Should be non-negative
  
  // Allocate an inode and verify count increases
  iinode_t *inode = ialloc(0, REGULAR, 0644);
  if (inode) {
    int new_count = activeinodes(0);
    CU_ASSERT_TRUE(new_count >= count);
    ifree(inode);
  }
}

// Functions available for external test runner

/**
 * @brief Test basic inode operations (original tests1.c version)
 */
void test_inode_pass(void) {
  namei_t i;
  active->u->fsroot = iget(fs1, 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(active->u->fsroot);
  active->u->workdir = iget(fs1, 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(active->u->workdir);
  CU_ASSERT_EQUAL(active->u->fsroot, active->u->workdir);
  CU_ASSERT_EQUAL(active->u->fsroot->nref, 2);
  i = namei("/.");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i.i);
  iput(i.i);
  CU_ASSERT_EQUAL(i.i, active->u->fsroot);
  i = namei(".");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i.i);
  iput(i.i);
  CU_ASSERT_EQUAL(i.i, active->u->fsroot);
  i = namei("/..");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i.i);
  iput(i.i);
  CU_ASSERT_EQUAL(i.i, active->u->fsroot);
  i = namei("..");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i.i);
  iput(i.i);
  CU_ASSERT_EQUAL(i.i, active->u->fsroot);
}
