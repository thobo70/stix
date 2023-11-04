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
#include "blocks.h"
#include "fs.h"
#include "utils.h"
#include "buf.h"
#include "pc.h"
#include "dd.h"


extern process_t *active;

char *tstdisk_getblock(ldevminor_t minor, block_t bidx);

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
  init_buffers();
  init_inodes();
  bdevopen((ldev_t){{0, 0}});
  return CUE_SUCCESS;
}
 


/* run at the end of the suite */
CU_SUITE_TEARDOWN() {
  bdevclose((ldev_t){{0, 0}});
  return CUE_SUCCESS;
}
 


/**
 * @brief Construct a new cu test setup object
 * 
 */
CU_TEST_SETUP() {
}
 


/* run at the end of each test */
CU_TEST_TEARDOWN() {
}
 


/* Test that two is bigger than one */
static void test_typesize_pass(void) {
  CU_ASSERT_EQUAL_FATAL(sizeof(byte_t), 1);
  CU_ASSERT_EQUAL_FATAL(sizeof(word_t), 2);
  CU_ASSERT_EQUAL_FATAL(sizeof(dword_t), 4);
  CU_ASSERT_EQUAL_FATAL(BLOCKSIZE % sizeof(dinode_t), 0);
}
 


/* Test that two is bigger than one */
static void test_buffer_pass(void) {
  ldev_t dev = {{0, 0}};
  const char *str = "Hello World";

  bhead_t *b = bread(dev, 60000);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b);
  CU_ASSERT_EQUAL(b->block, 60000);
  CU_ASSERT_TRUE(b->error);
  strcpy((char *)b->buf->mem, str);
  brelse(b);

  // read from cache
  // old buffer from b = bread(dev, 60000); is still in cache and should be returned first
  // but should be overwritten by new buffer
  b = bread(dev, 0);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b);
  CU_ASSERT_NOT_EQUAL(strcmp((char *)b->buf->mem, str), 0);
  brelse(b);

  b = bread(dev, 0);
  CU_ASSERT_PTR_NOT_NULL_FATAL(b);
  strcpy((char *)b->buf->mem, str);
  b->dwrite = false;    // write to disk immediately
  bwrite(b);
  brelse(b);
  CU_ASSERT_EQUAL(strcmp(tstdisk_getblock(0, 0), str), 0);
}
 


static void test_block_pass(void) {
  init_isblock(0, (ldev_t){{0, 0}});  // init superblock FS = 0 , device = 0

  bhead_t *b = balloc(0);
  CU_ASSERT_EQUAL(b->block, 6);
  bfree(0, b->block);
  brelse(b);
  b = balloc(0);
  CU_ASSERT_EQUAL(b->block, 6);
  bfree(0, b->block);
  brelse(b);
}
 


static void test_inode_pass(void) {
  iinode_t *i;
  active->u->fsroot = iget(0, 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(active->u->fsroot);
  active->u->workdir = iget(0, 1);
  CU_ASSERT_PTR_NOT_NULL_FATAL(active->u->workdir);
  CU_ASSERT_EQUAL(active->u->fsroot, active->u->workdir);
  CU_ASSERT_EQUAL(active->u->fsroot->nref, 2);
  i = namei("/.");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i);
  iput(i);
  CU_ASSERT_EQUAL(i, active->u->fsroot);
  i = namei(".");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i);
  iput(i);
  CU_ASSERT_EQUAL(i, active->u->fsroot);
  i = namei("/..");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i);
  iput(i);
  CU_ASSERT_EQUAL(i, active->u->fsroot);
  i = namei("..");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i);
  iput(i);
  CU_ASSERT_EQUAL(i, active->u->fsroot);
  i = namei("/");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i);
  iput(i);
  CU_ASSERT_EQUAL(i, active->u->fsroot);
  CU_ASSERT_EQUAL(active->u->fsroot->nref, 2);
}



static void test_file_pass(void) {
}
 


CUNIT_CI_RUN(
  "my-suite",
  CUNIT_CI_TEST(test_typesize_pass),
  CUNIT_CI_TEST(test_buffer_pass),
  CUNIT_CI_TEST(test_block_pass),
  CUNIT_CI_TEST(test_inode_pass),
  CUNIT_CI_TEST(test_file_pass)
)
