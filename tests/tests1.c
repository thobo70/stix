#include "CUnit/CUnitCI.h"
 
#include "tdefs.h"
#include "inode.h"

/* run at the start of the suite */
CU_SUITE_SETUP() {
    return CUE_SUCCESS;
}
 
/* run at the end of the suite */
CU_SUITE_TEARDOWN() {
    return CUE_SUCCESS;
}
 
/* run at the start of each test */
CU_TEST_SETUP() {
}
 
/* run at the end of each test */
CU_TEST_TEARDOWN() {
}
 
/* Test that two is bigger than one */
static void test_bytesize_pass(void) {
    CU_ASSERT_FATAL(sizeof(byte_t) == 1);
}
 
/* Test that two is bigger than one */
static void test_wordsize_pass(void) {
    CU_ASSERT_FATAL(sizeof(word_t) == 2);
}
 
/* Test that two is bigger than one */
static void test_dwordsize_pass(void) {
    CU_ASSERT_FATAL(sizeof(dword_t) == 4);
}

/* Test that two is bigger than one */
static void test_dinodesize_pass(void) {
    CU_ASSERT_FATAL(sizeof(dinode_t) == 64);
}
 
CUNIT_CI_RUN("my-suite",
             CUNIT_CI_TEST(test_bytesize_pass),
             CUNIT_CI_TEST(test_wordsize_pass),
             CUNIT_CI_TEST(test_dwordsize_pass),
             CUNIT_CI_TEST(test_dinodesize_pass)
             );
