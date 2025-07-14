/**
 * @file test_common.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Common utilities and definitions for STIX unit tests
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header provides common utilities, macros, and shared functionality
 * for all STIX unit tests. It centralizes common setup/teardown operations
 * and provides consistent testing infrastructure.
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#include "CUnit/Automated.h"
#include "tests1.h"
#include "inode.h"
#include "blocks.h"
#include "fs.h"
#include "utils.h"
#include "buf.h"
#include "pc.h"
#include "dd.h"
#include "clist.h"
#include "tstcon.h"
#include <stdio.h>

// ANSI color codes for output formatting
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Common test configuration
#define TEST_TIMEOUT_MS 1000        ///< Maximum test execution time
#define TEST_BUFFER_SIZE 1024       ///< Standard test buffer size
#define TEST_ITERATIONS_DEFAULT 10  ///< Default number of test iterations

// Test disk simulation constants (from tstdisk.c)
#define TEST_SIMNBLOCKS 128         ///< Number of blocks in simulated disk

// External references
extern process_t *active;
extern bdev_t tstdisk;
extern cdev_t tstcon;
extern fsnum_t fs1;

/**
 * @brief Common test setup function
 * 
 * Initializes common test infrastructure including device drivers,
 * filesystem, and mock process control.
 */
void test_common_setup(void);

/**
 * @brief Common test teardown function
 * 
 * Cleans up test resources and resets system state.
 */
void test_common_teardown(void);

/**
 * @brief Create a temporary test file with specified content
 * 
 * @param filename Name of file to create
 * @param content Content to write to file
 * @param size Size of content
 * @return 0 on success, negative on error
 */
int test_create_temp_file(const char *filename, const char *content, sizem_t size);

/**
 * @brief Clean up temporary test files
 * 
 * @param filename Name of file to remove
 * @return 0 on success, negative on error
 */
int test_cleanup_temp_file(const char *filename);

/**
 * @brief Verify file content matches expected data
 * 
 * @param filename Name of file to verify
 * @param expected Expected content
 * @param size Size of expected content
 * @return 0 if content matches, negative if mismatch
 */
int test_verify_file_content(const char *filename, const char *expected, sizem_t size);

/**
 * @brief Enhanced assertion macros for better error reporting
 */
#define TEST_ASSERT_EQUAL_MSG(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            CU_FAIL(msg); \
        } else { \
            CU_ASSERT_EQUAL(actual, expected); \
        } \
    } while(0)

#define TEST_ASSERT_NOT_NULL_MSG(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            CU_FAIL(msg); \
        } else { \
            CU_ASSERT_PTR_NOT_NULL(ptr); \
        } \
    } while(0)

#define TEST_ASSERT_FILE_EXISTS(filename) \
    do { \
        int fd = open(filename, O_RDONLY); \
        if (fd < 0) { \
            CU_FAIL("File does not exist: " filename); \
        } else { \
            close(fd); \
            CU_PASS("File exists: " filename); \
        } \
    } while(0)

/**
 * @brief Test suite registration helpers
 */
typedef struct test_suite_info_t {
    const char *name;
    CU_TestInfo *tests;
    CU_InitializeFunc init;
    CU_CleanupFunc cleanup;
} test_suite_info_t;

/**
 * @brief Register a test suite with the CUnit framework
 * 
 * @param suite_info Test suite information
 * @return 0 on success, negative on error
 */
int test_register_suite(const test_suite_info_t *suite_info);

/**
 * @brief Performance measurement utilities
 */
typedef struct test_perf_t {
    dword_t start_time;
    dword_t end_time;
    dword_t operations;
    const char *test_name;
} test_perf_t;

void test_perf_start(test_perf_t *perf, const char *test_name);
void test_perf_end(test_perf_t *perf, dword_t operations);
void test_perf_report(const test_perf_t *perf);

// Function prototypes for tstdisk enhancements
extern int tstdisk_fsck_validate(ldevminor_t minor);
extern int tstdisk_mkfs_init(ldevminor_t minor);
extern int tstdisk_create_fresh_fs(ldevminor_t minor, word_t sectors, word_t inodes);
extern char *tstdisk_getblock(ldevminor_t minor, block_t bidx);

#endif /* TEST_COMMON_H */
