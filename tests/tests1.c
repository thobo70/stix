/**
 * @file tests1.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief CUnit test
 * @version 0.1
 * @date 2023-10-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "CUnit/CUnitCI.h"

#include "tests1.h"

#include "inode.h"
#include "dd.h"

extern bdev_t tstdisk;

bdev_t *bdevtable[] = {
  &tstdisk
};

extern cdev_t tstcon;

cdev_t *cdevtable[] = {
  &tstcon
};



/* run at the start of the suite */
CU_SUITE_SETUP() {
    ddinit();
    return CUE_SUCCESS;
}
 
/* run at the end of the suite */
CU_SUITE_TEARDOWN() {
    return CUE_SUCCESS;
}
 
/**
 * @brief Construct a new cu test setup object
 * 
 */
CU_TEST_SETUP() {
    bdevopen((ldev_t){{0, 0}});
}
 
/* run at the end of each test */
CU_TEST_TEARDOWN() {
    bdevclose((ldev_t){{0, 0}});
}
 
/* Test that two is bigger than one */
static void test_typesize_pass(void) {
    CU_ASSERT_FATAL(sizeof(byte_t) == 1);
    CU_ASSERT_FATAL(sizeof(word_t) == 2);
    CU_ASSERT_FATAL(sizeof(dword_t) == 4);
    CU_ASSERT_FATAL(BLOCKSIZE % sizeof(dinode_t) == 0);
}
 
/* Test that two is bigger than one */
static void test_buffer_pass(void) {
}
 
/* Test that two is bigger than one */
static void test_inode_pass(void) {
}

/* Test that two is bigger than one */
static void test_file_pass(void) {
}
 
CUNIT_CI_RUN("my-suite",
             CUNIT_CI_TEST(test_typesize_pass),
             CUNIT_CI_TEST(test_buffer_pass),
             CUNIT_CI_TEST(test_inode_pass),
             CUNIT_CI_TEST(test_file_pass)
             )
