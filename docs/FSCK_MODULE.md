# Filesystem Check (FSCK) Module

## Overview

The FSCK module provides filesystem checking functionality for the STIX operating system. It uses a simple sector read interface that allows it to be used both in unit tests and standalone executables.

## Architecture

The module is designed around a single interface function that reads sectors:

```c
typedef int (*fsck_read_sector_fn)(block_t sector_number, byte_t *buffer);
```

This abstraction allows the module to work with:
- Real hardware devices
- Mock devices for testing
- Filesystem image files
- Any other storage backend that can read 512-byte sectors

## Files

### Core Module
- `src/include/fsck.h` - Public interface and type definitions
- `src/fsck/fsck.c` - Implementation of filesystem checking functionality

### Standalone Executable
- `src/fsck/fsck_standalone.c` - Standalone utility for checking filesystem images

### Unit Tests
- `tests/unit/test_fsck.c` - Comprehensive unit tests
- `tests/unit/test_fsck.h` - Test function declarations

## Usage

### In Unit Tests

```c
#include "fsck.h"

// Define a sector read function
int my_read_sector(block_t sector_number, byte_t *buffer) {
    // Implementation to read sector from mock storage
    return 0; // Return 0 on success, non-zero on error
}

// Initialize the module
fsck_result_t result = fsck_init(my_read_sector);
if (result != FSCK_OK) {
    // Handle initialization error
}

// Perform filesystem check
fsck_stats_t stats;
result = fsck_check_filesystem(&stats);
if (result != FSCK_OK) {
    printf("Filesystem has errors: %s\n", fsck_get_error_message(result));
}
```

### Standalone Executable

```bash
# Check a filesystem image
./fsck_standalone filesystem.img

# Return codes:
# 0 - Filesystem is clean
# 1 - Filesystem has errors
# 2 - Usage error or file access problem
```

## Functionality

The module performs the following checks:

### Superblock Validation
- Checks magic number (0x73746978 - "stix")
- Validates structural consistency
- Verifies block and inode counts

### Inode Table Validation
- Checks each used inode for valid file type
- Validates file size consistency
- Ensures proper structure

### Bitmap Validation
- Verifies bitmap structure
- Checks unused bits are properly zeroed

### Overall Consistency
- Cross-checks between superblock, inodes, and bitmap
- Reports comprehensive statistics

## Error Codes

- `FSCK_OK` - No errors found
- `FSCK_ERR_READ_FAILED` - Sector read failed
- `FSCK_ERR_INVALID_MAGIC` - Invalid superblock magic number
- `FSCK_ERR_INVALID_SUPERBLOCK` - Invalid superblock structure
- `FSCK_ERR_INVALID_INODE` - Invalid inode structure
- `FSCK_ERR_INVALID_BITMAP` - Invalid bitmap structure
- `FSCK_ERR_INCONSISTENT` - Filesystem inconsistency detected
- `FSCK_ERR_NULL_POINTER` - Null pointer error

## Statistics

The module provides detailed statistics:

```c
typedef struct fsck_stats_t {
    word_t total_blocks;      // Total number of blocks
    word_t free_blocks;       // Number of free blocks
    word_t total_inodes;      // Total number of inodes
    word_t free_inodes;       // Number of free inodes
    word_t errors_found;      // Number of errors found
    word_t warnings_found;    // Number of warnings found
} fsck_stats_t;
```

## Building

The module is automatically built as part of the STIX project:

```bash
cd build
make

# Executables created:
# - tests_modular (includes fsck unit tests)
# - fsck_standalone (standalone checker)
```

## Testing

The module includes comprehensive unit tests that cover:
- Module initialization
- Error handling
- Superblock validation
- Inode checking
- Bitmap validation
- Complete filesystem checking
- Edge cases and error conditions

Run tests with:
```bash
cd build
./tests_modular  # Includes fsck tests
ctest            # Run all project tests
```

## Design Principles

1. **Separation of Concerns**: Storage access is abstracted through the sector read interface
2. **Testability**: Mock storage can be easily implemented for testing
3. **Modularity**: Can be used as a library or standalone executable
4. **Error Reporting**: Comprehensive error codes and human-readable messages
5. **Statistics**: Detailed reporting of filesystem state
6. **Safety**: Robust error handling and input validation
