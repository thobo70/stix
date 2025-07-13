/**
 * @file tstcon.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Character device driver for comprehensive clist testing
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file implements a character device driver specifically designed for
 * comprehensive unit testing of clist operations. Similar to how tstdisk.c
 * provides a simulated disk for filesystem testing, this provides a test
 * console with data generation, verification, and advanced testing capabilities.
 */

#include "utils.h"
#include "dd.h"
#include "clist.h"
#include <stdlib.h>

// Test configuration and state tracking
#define TSTCON_MAX_DEVICES 4        ///< Maximum number of test console devices
#define TSTCON_PATTERN_SIZE 256     ///< Size of test patterns
#define TSTCON_BUFFER_SIZE 1024     ///< Internal buffer size

/**
 * @brief Test console device state
 * 
 * Tracks the state and configuration for each test console device,
 * enabling comprehensive testing and verification of clist operations.
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

// Global device state
static tstcon_device_t devices[TSTCON_MAX_DEVICES];
static byte_t tstcon_initialized = 0;


// Function prototypes
void tstcon_open(ldevminor_t minor);
void tstcon_close(ldevminor_t minor);
void tstcon_read(ldevminor_t minor, byte_t cl);
void tstcon_write(ldevminor_t minor, byte_t cl);
void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg);

// Internal helper functions
static void tstcon_init_device(ldevminor_t minor);
static void tstcon_generate_pattern_data(tstcon_device_t *dev, char *buffer, sizem_t size);
static void tstcon_generate_sequential_data(tstcon_device_t *dev, char *buffer, sizem_t size);
static void tstcon_generate_random_data(tstcon_device_t *dev, char *buffer, sizem_t size);
static byte_t tstcon_verify_data(tstcon_device_t *dev, char *buffer, sizem_t size);

cdev_t tstcon = {
  NULL,
  tstcon_open,
  tstcon_close,
  tstcon_read,
  tstcon_write,
  tstcon_ioctl
};

/**
 * @brief Initialize a test console device
 * 
 * @param minor Device minor number
 */
static void tstcon_init_device(ldevminor_t minor) {
    if (minor >= TSTCON_MAX_DEVICES) return;
    
    tstcon_device_t *dev = &devices[minor];
    dev->active = 1;
    dev->test_mode = TSTCON_MODE_PASSTHROUGH;
    dev->bytes_written = 0;
    dev->bytes_read = 0;
    dev->pattern_offset = 0;
    dev->verify_mode = 0;
    dev->error_count = 0;
    
    // Initialize default pattern (sequential bytes)
    for (int i = 0; i < TSTCON_PATTERN_SIZE; i++) {
        dev->test_pattern[i] = (char)(i & 0xFF);
    }
}

/**
 * @brief Generate pattern data for testing
 * 
 * @param dev Device state
 * @param buffer Output buffer
 * @param size Number of bytes to generate
 */
static void tstcon_generate_pattern_data(tstcon_device_t *dev, char *buffer, sizem_t size) {
    for (sizem_t i = 0; i < size; i++) {
        buffer[i] = dev->test_pattern[dev->pattern_offset % TSTCON_PATTERN_SIZE];
        dev->pattern_offset++;
    }
}

/**
 * @brief Generate sequential data for testing
 * 
 * @param dev Device state
 * @param buffer Output buffer
 * @param size Number of bytes to generate
 */
static void tstcon_generate_sequential_data(tstcon_device_t *dev, char *buffer, sizem_t size) {
    for (sizem_t i = 0; i < size; i++) {
        buffer[i] = (char)((dev->bytes_written + i) & 0xFF);
    }
}

/**
 * @brief Generate pseudo-random data for testing
 * 
 * @param dev Device state
 * @param buffer Output buffer
 * @param size Number of bytes to generate
 */
static void tstcon_generate_random_data(tstcon_device_t *dev, char *buffer, sizem_t size) {
    // Simple LFSR-based pseudo-random generator for reproducible testing
    static word_t lfsr = 0xACE1;  // Non-zero seed
    (void)dev; // Suppress unused parameter warning
    
    for (sizem_t i = 0; i < size; i++) {
        // 16-bit LFSR with taps at 16, 14, 13, 11
        byte_t lsb = lfsr & 1;
        lfsr >>= 1;
        if (lsb) lfsr ^= 0xB400;
        
        buffer[i] = (char)(lfsr & 0xFF);
    }
}

/**
 * @brief Verify data matches expected pattern
 * 
 * @param dev Device state
 * @param buffer Data to verify
 * @param size Number of bytes to verify
 * @return 1 if data matches, 0 if mismatch detected
 */
static byte_t tstcon_verify_data(tstcon_device_t *dev, char *buffer, sizem_t size) {
    char expected[TSTCON_BUFFER_SIZE];
    sizem_t chunk_size = size > TSTCON_BUFFER_SIZE ? TSTCON_BUFFER_SIZE : size;
    
    // Generate expected data based on current mode
    switch (dev->test_mode) {
        case TSTCON_MODE_PATTERN:
            tstcon_generate_pattern_data(dev, expected, chunk_size);
            break;
        case TSTCON_MODE_SEQUENTIAL:
            tstcon_generate_sequential_data(dev, expected, chunk_size);
            break;
        case TSTCON_MODE_RANDOM:
            tstcon_generate_random_data(dev, expected, chunk_size);
            break;
        default:
            return 1; // No verification for other modes
    }
    
    // Compare data
    for (sizem_t i = 0; i < chunk_size; i++) {
        if (buffer[i] != expected[i]) {
            dev->error_count++;
            return 0;
        }
    }
    
    return 1;
}



/**
 * @brief Open test console device
 * 
 * @param minor Device minor number
 */
void tstcon_open(ldevminor_t minor) {
    ASSERT(minor < TSTCON_MAX_DEVICES);
    
    if (!tstcon_initialized) {
        mset(devices, 0, sizeof(devices));
        init_clist();
        tstcon_initialized = 1;
    }
    
    tstcon_init_device(minor);
}

/**
 * @brief Close test console device
 * 
 * @param minor Device minor number
 */
void tstcon_close(ldevminor_t minor) {
    ASSERT(minor < TSTCON_MAX_DEVICES);
    
    if (devices[minor].active) {
        devices[minor].active = 0;
    }
}

/**
 * @brief Read from test console device
 * 
 * Generates test data based on current mode and pushes to clist
 * 
 * @param minor Device minor number
 * @param cl Character list to write to
 */
void tstcon_read(ldevminor_t minor, byte_t cl) {
    ASSERT(minor < TSTCON_MAX_DEVICES);
    ASSERT(devices[minor].active);
    
    tstcon_device_t *dev = &devices[minor];
    char buffer[TSTCON_BUFFER_SIZE];
    sizem_t chunk_size = 64; // Generate data in 64-byte chunks
    
    switch (dev->test_mode) {
        case TSTCON_MODE_PATTERN:
            tstcon_generate_pattern_data(dev, buffer, chunk_size);
            break;
            
        case TSTCON_MODE_SEQUENTIAL:
            tstcon_generate_sequential_data(dev, buffer, chunk_size);
            break;
            
        case TSTCON_MODE_RANDOM:
            tstcon_generate_random_data(dev, buffer, chunk_size);
            break;
            
        case TSTCON_MODE_STRESS:
            // Stress mode: variable size chunks with different patterns
            chunk_size = (dev->bytes_written % 32) + 1;
            if (dev->bytes_written % 3 == 0) {
                tstcon_generate_pattern_data(dev, buffer, chunk_size);
            } else if (dev->bytes_written % 3 == 1) {
                tstcon_generate_sequential_data(dev, buffer, chunk_size);
            } else {
                tstcon_generate_random_data(dev, buffer, chunk_size);
            }
            break;
            
        case TSTCON_MODE_PASSTHROUGH:
        default:
            // Passthrough mode: just push a simple test character
            buffer[0] = 'T';
            chunk_size = 1;
            break;
    }
    
    // Push generated data to clist
    for (sizem_t i = 0; i < chunk_size; i++) {
        if (clist_push(cl, &buffer[i], 1) != 0) {
            break; // Stop if clist is full
        }
    }
    
    dev->bytes_written += chunk_size;
}

/**
 * @brief Write to test console device
 * 
 * Pops data from clist and optionally verifies it matches expected patterns
 * 
 * @param minor Device minor number  
 * @param cl Character list to read from
 */
void tstcon_write(ldevminor_t minor, byte_t cl) {
    ASSERT(minor < TSTCON_MAX_DEVICES);
    ASSERT(devices[minor].active);
    
    tstcon_device_t *dev = &devices[minor];
    char buffer[TSTCON_BUFFER_SIZE];
    sizem_t bytes_read = 0;
    
    // Pop data from clist
    while (bytes_read < TSTCON_BUFFER_SIZE && 
           clist_pop(cl, &buffer[bytes_read], 1) == 0) {
        bytes_read++;
    }
    
    if (bytes_read > 0) {
        // Verify data if verification mode is enabled
        if (dev->verify_mode && dev->test_mode != TSTCON_MODE_PASSTHROUGH) {
            tstcon_verify_data(dev, buffer, bytes_read);
        }
        
        dev->bytes_read += bytes_read;
    }
}

/**
 * @brief IOCTL for test console device
 * 
 * Provides control interface for configuring test modes and retrieving statistics
 * 
 * @param minor Device minor number
 * @param cmd IOCTL command
 * @param arg Command argument
 */
void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg) {
    ASSERT(minor < TSTCON_MAX_DEVICES);
    ASSERT(devices[minor].active);
    
    tstcon_device_t *dev = &devices[minor];
    
    switch (cmd) {
        case TSTCON_SET_MODE:
            ASSERT(arg);
            dev->test_mode = *(byte_t*)arg;
            dev->pattern_offset = 0; // Reset pattern offset
            break;
            
        case TSTCON_SET_PATTERN:
            ASSERT(arg);
            mcpy(dev->test_pattern, (char*)arg, TSTCON_PATTERN_SIZE);
            dev->pattern_offset = 0;
            break;
            
        case TSTCON_GET_STATS:
            ASSERT(arg);
            {
                dword_t *stats = (dword_t*)arg;
                stats[0] = dev->bytes_written;
                stats[1] = dev->bytes_read;
                stats[2] = dev->error_count;
            }
            break;
            
        case TSTCON_RESET_STATS:
            // No arg needed for reset
            dev->bytes_written = 0;
            dev->bytes_read = 0;
            dev->error_count = 0;
            dev->pattern_offset = 0;
            break;
            
        case TSTCON_ENABLE_VERIFY:
            ASSERT(arg);
            dev->verify_mode = *(byte_t*)arg;
            break;
            
        case TSTCON_GET_ERRORS:
            ASSERT(arg);
            *(byte_t*)arg = dev->error_count;
            break;
            
        default:
            // Unknown command - could assert or ignore
            break;
    }
}


