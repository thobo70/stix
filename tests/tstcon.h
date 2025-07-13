/**
 * @file tstcon.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Header for character device driver clist testing infrastructure
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * Provides declarations for the comprehensive clist testing infrastructure
 * implemented as a character device driver in tstcon.c.
 */

#ifndef TSTCON_H
#define TSTCON_H

#include "utils.h"
#include "dd.h"
#include "clist.h"

// Test configuration constants
#define TSTCON_MAX_DEVICES 4        ///< Maximum number of test console devices
#define TSTCON_PATTERN_SIZE 256     ///< Size of test patterns
#define TSTCON_BUFFER_SIZE 1024     ///< Internal buffer size

// Test modes for different testing scenarios
#define TSTCON_MODE_PASSTHROUGH 0   ///< Simple passthrough mode (original behavior)
#define TSTCON_MODE_PATTERN     1   ///< Generate/verify repeating patterns
#define TSTCON_MODE_SEQUENTIAL  2   ///< Generate/verify sequential data
#define TSTCON_MODE_RANDOM      3   ///< Generate/verify pseudo-random data
#define TSTCON_MODE_STRESS      4   ///< Stress testing with variable data

// IOCTL commands for test control
#define TSTCON_SET_MODE         0x100   ///< Set test mode
#define TSTCON_SET_PATTERN      0x101   ///< Set test pattern
#define TSTCON_GET_STATS        0x102   ///< Get device statistics
#define TSTCON_RESET_STATS      0x103   ///< Reset statistics
#define TSTCON_ENABLE_VERIFY    0x104   ///< Enable verification mode
#define TSTCON_GET_ERRORS       0x105   ///< Get error count

/**
 * @brief Test console device state structure
 * 
 * This structure is internal to tstcon.c but the layout is documented
 * here for understanding the device behavior.
 */
typedef struct tstcon_device_t {
    byte_t active;              ///< Device is active (0=inactive, 1=active)
    byte_t test_mode;           ///< Current test mode
    sizem_t bytes_written;      ///< Total bytes written to device
    sizem_t bytes_read;         ///< Total bytes read from device
    sizem_t pattern_offset;     ///< Current offset in test pattern
    char test_pattern[TSTCON_PATTERN_SIZE]; ///< Current test pattern
    byte_t verify_mode;         ///< Enable verification during read/write
    byte_t error_count;         ///< Number of verification errors detected
} tstcon_device_t;

// Character device structure (defined in tstcon.c)
extern cdev_t tstcon;

// Character device driver function declarations
void tstcon_open(ldevminor_t minor);
void tstcon_close(ldevminor_t minor);
void tstcon_read(ldevminor_t minor, byte_t cl);
void tstcon_write(ldevminor_t minor, byte_t cl);
void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg);

/**
 * @brief Usage Examples
 * 
 * The tstcon device driver can be used through the standard character device
 * interface with the DD (device driver) subsystem, or directly by calling
 * the device functions for unit testing.
 * 
 * @section basic_usage Basic Usage Example
 * 
 * @code
 * // Open device
 * tstcon_open(0);
 * 
 * // Create a clist for testing
 * byte_t cl = clist_create();
 * 
 * // Set test mode to pattern generation
 * byte_t mode = TSTCON_MODE_PATTERN;
 * tstcon_ioctl(0, TSTCON_SET_MODE, &mode);
 * 
 * // Generate test data by reading from device
 * tstcon_read(0, cl);
 * 
 * // Process data by writing back to device
 * tstcon_write(0, cl);
 * 
 * // Get statistics
 * dword_t stats[3];
 * tstcon_ioctl(0, TSTCON_GET_STATS, stats);
 * // stats[0] = bytes_written, stats[1] = bytes_read, stats[2] = error_count
 * 
 * // Clean up
 * clist_destroy(cl);
 * tstcon_close(0);
 * @endcode
 * 
 * @section pattern_testing Pattern Testing Example
 * 
 * @code
 * void test_clist_with_pattern(void) {
 *     tstcon_open(0);
 *     byte_t cl = clist_create();
 *     
 *     // Set up custom pattern
 *     char pattern[TSTCON_PATTERN_SIZE];
 *     for (int i = 0; i < TSTCON_PATTERN_SIZE; i++) {
 *         pattern[i] = 'A' + (i % 26); // A-Z repeating
 *     }
 *     
 *     byte_t mode = TSTCON_MODE_PATTERN;
 *     tstcon_ioctl(0, TSTCON_SET_MODE, &mode);
 *     tstcon_ioctl(0, TSTCON_SET_PATTERN, pattern);
 *     
 *     // Enable verification
 *     byte_t verify = 1;
 *     tstcon_ioctl(0, TSTCON_ENABLE_VERIFY, &verify);
 *     
 *     // Generate and verify data multiple times
 *     for (int i = 0; i < 10; i++) {
 *         tstcon_read(0, cl);   // Generate pattern data
 *         tstcon_write(0, cl);  // Verify pattern data
 *     }
 *     
 *     // Check for errors
 *     byte_t errors;
 *     tstcon_ioctl(0, TSTCON_GET_ERRORS, &errors);
 *     ASSERT(errors == 0); // Should be no verification errors
 *     
 *     clist_destroy(cl);
 *     tstcon_close(0);
 * }
 * @endcode
 * 
 * @section stress_testing Stress Testing Example
 * 
 * @code
 * void test_clist_stress(void) {
 *     tstcon_open(0);
 *     byte_t cl = clist_create();
 *     
 *     // Set stress testing mode
 *     byte_t mode = TSTCON_MODE_STRESS;
 *     tstcon_ioctl(0, TSTCON_SET_MODE, &mode);
 *     
 *     // Reset statistics
 *     tstcon_ioctl(0, TSTCON_RESET_STATS, NULL);
 *     
 *     // Perform many operations with varying data
 *     for (int i = 0; i < 1000; i++) {
 *         tstcon_read(0, cl);  // Generate variable data
 *         tstcon_write(0, cl); // Process the data
 *     }
 *     
 *     // Get final statistics
 *     dword_t stats[3];
 *     tstcon_ioctl(0, TSTCON_GET_STATS, stats);
 *     
 *     // Verify significant data was processed
 *     ASSERT(stats[0] > 1000); // bytes_written
 *     ASSERT(stats[1] > 1000); // bytes_read
 *     
 *     clist_destroy(cl);
 *     tstcon_close(0);
 * }
 * @endcode
 */

/**
 * @brief Key Benefits of the Enhanced tstcon Device Driver
 * 
 * 1. **Multiple Test Modes**: Supports pattern, sequential, random, and stress testing
 * 2. **Data Verification**: Built-in verification to detect data corruption
 * 3. **Statistics Tracking**: Comprehensive statistics for performance analysis
 * 4. **Configurable Patterns**: Custom test patterns for specific scenarios
 * 5. **Device Driver Interface**: Standard character device interface for integration
 * 6. **Multi-Device Support**: Support for up to 4 independent test devices
 * 7. **STIX Native**: Uses only STIX-native functions for kernel compatibility
 * 8. **Comprehensive Testing**: Enables thorough testing of clist operations
 */

#endif /* TSTCON_H */
