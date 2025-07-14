/**
 * @file test_fsck.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Header for filesystem check unit tests
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _TEST_FSCK_H
#define _TEST_FSCK_H

/**
 * @brief Register all fsck tests with CUnit
 * 
 * @return CUE_SUCCESS on success, CUnit error code on failure
 */
int register_fsck_tests(void);

#endif /* _TEST_FSCK_H */
