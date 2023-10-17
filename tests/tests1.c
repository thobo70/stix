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

#include <stdlib.h>
#include <string.h>

simpart_t *part = NULL;

/* run at the start of the suite */
CU_SUITE_SETUP() {
    part = malloc(sizeof(simpart_t));
    if (!part)
        return CUE_NOMEMORY;
    return CUE_SUCCESS;
}
 
/* run at the end of the suite */
CU_SUITE_TEARDOWN() {
    if (part)
        free(part);
    return CUE_SUCCESS;
}
 
/**
 * @brief Construct a new cu test setup object
 * 
 * @startuml
 * salt
 * {#
 * Block | used for
 * 0 | reserved
 * 1 | superblock
 * 2 | inode 1
 * 3 | inode 2
 * 4 | bitmap
 * 5 | root dir
 * }
 * @enduml
 */
CU_TEST_SETUP() {
    memset(part, 0, sizeof(simpart_t));
    part->fs.sblock.super.version = 1;
    part->fs.sblock.super.type = 0;
    part->fs.sblock.super.inodes = 2; // start block of inodes
    part->fs.sblock.super.bbitmap = part->fs.sblock.super.inodes + SIMINODEBLOCKS;
    part->fs.sblock.super.firstblock = part->fs.sblock.super.bbitmap + SIMBMAPBLOCKS;
    part->fs.sblock.super.inodes = SIMNINODES;
    part->fs.sblock.super.nblocks = SIMNBLOCKS;

    part->fs.inodes.i[0].ftype = DIRECTORY;
    part->fs.inodes.i[0].nlinks = 2;
    part->fs.inodes.i[0].fsize = 0;
    part->fs.inodes.i[0].blockrefs[0] = part->fs.sblock.super.firstblock;

    dirent_t *rootdir = (dirent_t*) part->block[part->fs.inodes.i[0].blockrefs[0]].mem;
    rootdir[0].inum = 1; // first inode starts with 1 not 0
    strncpy(rootdir[0].name, ".", DIRNAMEENTRY);
    rootdir[1].inum = 1; // first inode starts with 1 not 0
    strncpy(rootdir[1].name, "..", DIRNAMEENTRY);

    byte_t *bmap = part->block[part->fs.sblock.super.bbitmap].mem;
    bmap[0] = 0x3F;  // first 6 bits of bitmap are set, 6 blocks are used
}
 
/* run at the end of each test */
CU_TEST_TEARDOWN() {
}
 
/* Test that two is bigger than one */
static void test_typesize_pass(void) {
    CU_ASSERT_FATAL(sizeof(byte_t) == 1);
    CU_ASSERT_FATAL(sizeof(word_t) == 2);
    CU_ASSERT_FATAL(sizeof(dword_t) == 4);
    CU_ASSERT_FATAL(BLOCKSIZE % sizeof(dinode_t) == 0);
}
 
/* Test that two is bigger than one */
static void test_wordsize_pass(void) {
}
 
/* Test that two is bigger than one */
static void test_dwordsize_pass(void) {
}

/* Test that two is bigger than one */
static void test_dinodesize_pass(void) {
}
 
CUNIT_CI_RUN("my-suite",
             CUNIT_CI_TEST(test_typesize_pass),
             CUNIT_CI_TEST(test_wordsize_pass),
             CUNIT_CI_TEST(test_dwordsize_pass),
             CUNIT_CI_TEST(test_dinodesize_pass)
             )
