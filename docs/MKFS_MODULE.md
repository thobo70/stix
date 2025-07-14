# Make Filesystem (MKFS) Module

## Overview

The MKFS module provides filesystem creation functionality for the STIX operating system. It uses simple sector read/write interfaces that allow it to be used both in unit tests and standalone executables. The module can create complete STIX filesystems with proper superblock, inode table, bitmap, and root directory structures.

## Architecture

The module is designed around two interface functions for sector operations:

```c
typedef int (*mkfs_read_sector_fn)(block_t sector_number, byte_t *buffer);
typedef int (*mkfs_write_sector_fn)(block_t sector_number, const byte_t *buffer);
```

This abstraction allows the module to work with:
- Real hardware devices
- Mock devices for testing  
- Filesystem image files
- Any other storage backend that can read/write 512-byte sectors

## Files

### Core Module
- `src/include/mkfs.h` - Public interface and type definitions
- `src/mkfs/mkfs.c` - Implementation of filesystem creation functionality

### Standalone Executable
- `src/mkfs/mkfs_standalone.c` - Standalone utility for creating filesystem images

### Unit Tests
- `tests/unit/test_mkfs.c` - Comprehensive unit tests (8 test functions)

## Usage

### In Unit Tests

```c
#include "mkfs.h"

// Define sector read/write functions
int my_read_sector(block_t sector_number, byte_t *buffer) {
    // Implementation to read sector from mock storage
    return 0; // Return 0 on success, non-zero on error
}

int my_write_sector(block_t sector_number, const byte_t *buffer) {
    // Implementation to write sector to mock storage
    return 0; // Return 0 on success, non-zero on error
}

// Initialize the module
mkfs_result_t result = mkfs_init(my_read_sector, my_write_sector);
if (result != MKFS_OK) {
    // Handle initialization error
}

// Calculate filesystem layout
mkfs_params_t params;
result = mkfs_calculate_layout(1024, 0, &params); // 1024 sectors, auto-calculate inodes
if (result != MKFS_OK) {
    printf("Layout calculation failed: %s\n", mkfs_get_error_message(result));
}

// Create filesystem
result = mkfs_create_filesystem(&params);
if (result != MKFS_OK) {
    printf("Filesystem creation failed: %s\n", mkfs_get_error_message(result));
}
```

### Standalone Executable

```bash
# Create a 1MB filesystem (2048 sectors)
./mkfs_standalone disk.img 2048

# Create filesystem with specific number of inodes
./mkfs_standalone -i 256 disk.img 2048

# Verbose output with force overwrite
./mkfs_standalone -v -f disk.img 4096

# Get help
./mkfs_standalone --help
```

## Command Line Options

- `-i, --inodes NUM` - Specify number of inodes (default: auto-calculate)
- `-f, --force` - Force creation, overwrite existing filesystem
- `-v, --verbose` - Verbose output showing layout details
- `-h, --help` - Show help message

## Functionality

### Filesystem Layout Calculation
- Automatically calculates optimal inode count if not specified
- Determines inode table size based on inode count
- Calculates bitmap size for block allocation
- Determines data area layout

### Superblock Creation
- Creates valid STIX superblock with magic number (0x73746978 - "stix")
- Sets filesystem metadata (version, block counts, layout pointers)
- Validates structural consistency

### Inode Table Initialization
- Creates properly formatted inode table
- Initializes root directory inode (inode 0)
- Marks unused inodes as free (IFREE)
- Sets proper permissions and metadata

### Block Bitmap Creation
- Creates block allocation bitmap
- Marks system blocks as used (superblock, inode table, bitmap)
- Marks root directory block as used
- Leaves data blocks free for allocation

### Root Directory Creation
- Creates root directory with "." and ".." entries
- Both entries point to inode 0 (root is its own parent)
- Properly formatted directory entries

## Error Codes

- `MKFS_OK` - Filesystem created successfully
- `MKFS_ERR_READ_FAILED` - Sector read failed
- `MKFS_ERR_WRITE_FAILED` - Sector write failed
- `MKFS_ERR_INVALID_SIZE` - Invalid filesystem size
- `MKFS_ERR_INVALID_INODES` - Invalid number of inodes
- `MKFS_ERR_TOO_SMALL` - Filesystem too small for requested inodes
- `MKFS_ERR_NULL_POINTER` - Null pointer error

## Filesystem Parameters

```c
typedef struct mkfs_params_t {
    word_t total_sectors;      // Total number of sectors
    word_t num_inodes;         // Number of inodes requested
    word_t calculated_inodes;  // Actual number of inodes
    word_t inode_sectors;      // Number of sectors for inode table
    word_t bitmap_sectors;     // Number of sectors for bitmap
    word_t first_data_sector;  // First sector available for data
    word_t data_sectors;       // Number of data sectors available
} mkfs_params_t;
```

## Automatic Inode Calculation

When `num_inodes` is 0, the module automatically calculates a reasonable number:
- Uses 1 inode per 4 data blocks as default ratio
- Enforces minimum of 16 inodes
- Enforces maximum of 32768 inodes (due to word_t size limits)
- Ensures inodes fit within filesystem constraints

## Integration with FSCK

The mkfs module is designed to work seamlessly with the fsck module:
- Creates filesystems that pass fsck validation
- Uses compatible data structures and layout
- Proper magic numbers and metadata

Example workflow:
```bash
# Create filesystem
./mkfs_standalone -v filesystem.img 1024

# Verify filesystem
./fsck_standalone filesystem.img
```

## Building

The module is automatically built as part of the STIX project:

```bash
cd build
make

# Executables created:
# - tests_modular (includes mkfs unit tests)
# - mkfs_standalone (standalone creator)
```

## Testing

The module includes comprehensive unit tests covering:
- Module initialization and parameter validation
- Filesystem layout calculation
- Superblock creation
- Inode table initialization  
- Bitmap creation
- Root directory creation
- Complete filesystem creation with fsck verification
- Edge cases and error conditions

Run tests with:
```bash
cd build
./tests_modular  # Includes mkfs tests (8 test functions)
ctest            # Run all project tests
```

## Examples

### Create Small Filesystem
```bash
# 50KB filesystem with auto-calculated inodes
./mkfs_standalone small.img 100
```

### Create Specific Configuration
```bash
# 500KB filesystem with exactly 128 inodes
./mkfs_standalone -i 128 medium.img 1000
```

### Create with Verification
```bash
# Create and immediately verify
./mkfs_standalone -v large.img 4096 && ./fsck_standalone large.img
```

## Design Principles

1. **Separation of Concerns**: Storage access abstracted through sector interfaces
2. **Testability**: Mock storage easily implemented for comprehensive testing
3. **Modularity**: Can be used as library or standalone executable
4. **Validation**: Creates filesystems compatible with fsck verification
5. **Flexibility**: Automatic inode calculation with manual override option
6. **Safety**: Robust error handling and parameter validation
7. **Standards**: Follows STIX filesystem format specifications

## Compatibility

- Works with existing STIX filesystem drivers
- Compatible with fsck module for verification
- Supports same inode and directory entry formats as kernel
- Uses standard STIX superblock layout and magic numbers
