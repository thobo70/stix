#include "CUnit/CUnitCI.h"
 
char *buf = NULL;
size_t bufsize = 32;
 
/* run at the start of the suite */
CU_SUITE_SETUP() {
    buf = (char*) malloc(bufsize);
    CU_ASSERT_FATAL(buf != NULL);
    return CUE_SUCCESS;
}
 
/* run at the end of the suite */
CU_SUITE_TEARDOWN() {
    if (buf) free(buf);
    return CUE_SUCCESS;
}
 
/* run at the start of each test */
CU_TEST_SETUP() {
    memset(buf, 1, bufsize);
}
 
/* run at the end of each test */
CU_TEST_TEARDOWN() {
    memset(buf, 0, bufsize);
}
 
/* Test that one equals one */
static void test_simple_pass1(void) {
    CU_ASSERT_FATAL(1 == 1);
}
 
/* Test that two is bigger than one */
static void test_simple_pass2(void) {
    CU_ASSERT_FATAL(2 > 1);
}
 
CUNIT_CI_RUN("my-suite",
             CUNIT_CI_TEST(test_simple_pass1),
             CUNIT_CI_TEST(test_simple_pass2)
             );
