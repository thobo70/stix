/**
 * @file test_common.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Common utilities implementation for STIX unit tests
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 */

#include "test_common.h"

// External device declarations
extern bdev_t tstdisk;
extern cdev_t tstcon;

// Device tables required by the dd subsystem
bdev_t *bdevtable[] = {
  &tstdisk,  // major 0 for tstdisk
  NULL
};

cdev_t *cdevtable[] = {
  &tstcon,
  NULL
};

// Global test state
static byte_t test_common_initialized = 0;

/**
 * @brief Common test setup function
 */
void test_common_setup(void) {
    if (!test_common_initialized) {
        // Initialize device drivers and filesystem
        init_clist();
        test_common_initialized = 1;
    }
}

/**
 * @brief Common test teardown function
 */
void test_common_teardown(void) {
    // Cleanup any remaining resources
    // This is called after each test to ensure clean state
}

/**
 * @brief Create a temporary test file with specified content
 */
int test_create_temp_file(const char *filename, const char *content, sizem_t size) {
    int fd = open(filename, OCREATE | OWRITE, 0644);
    if (fd < 0) return -1;
    
    ssize_t written = write(fd, (byte_t*)content, size);
    close(fd);
    
    return (written == (ssize_t)size) ? 0 : -1;
}

/**
 * @brief Clean up temporary test files
 */
int test_cleanup_temp_file(const char *filename) {
    return unlink(filename);
}

/**
 * @brief Verify file content matches expected data
 */
int test_verify_file_content(const char *filename, const char *expected, sizem_t size) {
    int fd = open(filename, OREAD, 0644);
    if (fd < 0) return -1;
    
    char buffer[TEST_BUFFER_SIZE];
    ssize_t bytes_read = read(fd, (byte_t*)buffer, size);
    close(fd);
    
    if (bytes_read != (ssize_t)size) return -1;
    
    return sncmp(buffer, expected, size);
}

/**
 * @brief Register a test suite with the CUnit framework
 */
int test_register_suite(const test_suite_info_t *suite_info) {
    CU_SuiteInfo suite[] = {
        { suite_info->name, suite_info->init, suite_info->cleanup, NULL, NULL, suite_info->tests },
        CU_SUITE_INFO_NULL
    };
    
    return CU_register_suites(suite);
}

/**
 * @brief Performance measurement utilities
 */
void test_perf_start(test_perf_t *perf, const char *test_name) {
    perf->test_name = test_name;
    perf->operations = 0;
    // Note: STIX doesn't have standard timing functions, so we'll use operation counts
    perf->start_time = 0;
}

void test_perf_end(test_perf_t *perf, dword_t operations) {
    perf->operations = operations;
    perf->end_time = 1; // Placeholder since we don't have real timing
}

void test_perf_report(const test_perf_t *perf) {
    // Simple performance reporting
    // In a real implementation, this would calculate operations/second
    (void)perf; // Suppress unused parameter warning for now
}
