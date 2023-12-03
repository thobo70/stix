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
#include "clist.h"


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

fsnum_t fs1;


/* run at the start of the suite */
CU_SUITE_SETUP() {
  init_dd();
  init_buffers();
  init_inodes();
  init_fs(); 
  init_clist();
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
  fs1 = init_isblock((ldev_t){{0, 0}});  // init superblock device = 0, should return fs1 = 1
  CU_ASSERT_EQUAL(fs1, 1);

  bhead_t *b = balloc(fs1);
  CU_ASSERT_EQUAL(b->block, 6);
  bfree(fs1, b->block);
  brelse(b);
  b = balloc(fs1);
  CU_ASSERT_EQUAL(b->block, 6);
  bfree(fs1, b->block);
  brelse(b);
}
 


static void test_inode_pass(void) {
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
  i = namei("/");
  CU_ASSERT_PTR_NOT_NULL_FATAL(i.i);
  iput(i.i);
  CU_ASSERT_EQUAL(i.i, active->u->fsroot);
  CU_ASSERT_EQUAL(active->u->fsroot->nref, 2);
}



static void test_file_pass(void) {
  CU_ASSERT_EQUAL(mkdir("/test", 0777), 0);
  CU_ASSERT_EQUAL(rmdir("/test"), 0);

  CU_ASSERT_EQUAL(mkdir("/test", 0777), 0);

  const char *str = "Hello World";
  char buf[100];
  int fd = open("/test/test.txt", OCREATE | ORDWR, 0777);
  CU_ASSERT_EQUAL(fd, 0);
  CU_ASSERT_EQUAL(write(fd, (byte_t *)str, strlen(str) + 1), (int)strlen(str) + 1);
  CU_ASSERT_EQUAL(close(fd), 0);

  fd = open("/test/test.txt", ORDWR, 0777);
  CU_ASSERT_EQUAL(fd, 0);
  CU_ASSERT_EQUAL(read(fd, (byte_t *)buf, strlen(str) + 1), (int)strlen(str) + 1);
  CU_ASSERT_EQUAL(strcmp(buf, str), 0);
  CU_ASSERT_EQUAL(close(fd), 0);

  CU_ASSERT_EQUAL(open("/test", ORDWR, 0777), -1);

  CU_ASSERT_EQUAL(unlink("/test/test.txt"), 0);
  CU_ASSERT_EQUAL(rmdir("/test"), 0);

  CU_ASSERT_EQUAL(open("/test/test.txt", ORDWR, 0777), -1);

  int n;
  CU_ASSERT_EQUAL(fd = open("full.txt", OCREATE | ORDWR, 0777), 0);
  do {
    CU_ASSERT_EQUAL((n = write(fd, (byte_t *)str, strlen(str) + 1)) >= 0, (1 == 1));
  } while (n != 0);
  CU_ASSERT_EQUAL(close(fd), 0);
  CU_ASSERT_EQUAL(unlink("full.txt"), 0);
  syncall_buffers(false);
  check_bfreelist();
}
 


static void test_clist_pass(void) {
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



CUNIT_CI_RUN(
  "my-suite",
  CUNIT_CI_TEST(test_typesize_pass),
  CUNIT_CI_TEST(test_buffer_pass),
  CUNIT_CI_TEST(test_block_pass),
  CUNIT_CI_TEST(test_inode_pass),
  CUNIT_CI_TEST(test_file_pass),
  CUNIT_CI_TEST(test_clist_pass)

)
