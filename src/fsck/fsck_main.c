/**
 * @file fsck_standalone.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Standalone filesystem check utility
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * This is a standalone executable that demonstrates the use of the fsck module
 * to check a filesystem image file.
 */

#include "fsck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Global file handle for the filesystem image */
static FILE *g_fs_image = NULL;

/**
 * @brief Read a sector from the filesystem image file
 * 
 * @param sector_number The sector number to read
 * @param buffer Buffer to store the sector data
 * @return 0 on success, non-zero on failure
 */
int file_read_sector(block_t sector_number, byte_t *buffer) {
    if (g_fs_image == NULL) {
        return 1;  // File not open
    }
    
    // Seek to the beginning of the sector
    long offset = sector_number * FSCK_BLOCKSIZE;
    if (fseek(g_fs_image, offset, SEEK_SET) != 0) {
        return 2;  // Seek failed
    }
    
    // Read the sector
    size_t bytes_read = fread(buffer, 1, FSCK_BLOCKSIZE, g_fs_image);
    if (bytes_read != FSCK_BLOCKSIZE) {
        return 3;  // Read failed or incomplete
    }
    
    return 0;  // Success
}

/**
 * @brief Print usage information
 */
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <filesystem_image>\n", program_name);
    printf("\n");
    printf("Perform a filesystem check on the specified image file.\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  filesystem_image  Path to the filesystem image file to check\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help       Show this help message\n");
    printf("\n");
    printf("Return codes:\n");
    printf("  0  Filesystem is clean\n");
    printf("  1  Filesystem has errors\n");
    printf("  2  Usage error or file access problem\n");
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    // Handle help flags
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (argc != 2) {
        print_usage(argv[0]);
        return 2;
    }
    
    const char *image_file = argv[1];
    
    printf("STIX Filesystem Check Utility\n");
    printf("=============================\n");
    printf("Checking filesystem image: %s\n\n", image_file);
    
    // Open the filesystem image file
    g_fs_image = fopen(image_file, "rb");
    if (g_fs_image == NULL) {
        printf("Error: Cannot open file '%s'\n", image_file);
        return 2;
    }
    
    // Initialize the fsck module
    fsck_result_t result = fsck_init(file_read_sector);
    if (result != FSCK_OK) {
        printf("Error: Failed to initialize fsck module: %s\n", 
               fsck_get_error_message(result));
        fclose(g_fs_image);
        return 2;
    }
    
    // Perform the filesystem check
    fsck_stats_t stats;
    result = fsck_check_filesystem(&stats);
    
    // Print results
    printf("Filesystem Check Results:\n");
    printf("-------------------------\n");
    printf("Total blocks:     %u\n", stats.total_blocks);
    printf("Free blocks:      %u\n", stats.free_blocks);
    printf("Total inodes:     %u\n", stats.total_inodes);
    printf("Free inodes:      %u\n", stats.free_inodes);
    printf("Errors found:     %u\n", stats.errors_found);
    printf("Warnings found:   %u\n", stats.warnings_found);
    printf("\n");
    
    if (result == FSCK_OK) {
        printf("Result: Filesystem is CLEAN\n");
    } else {
        printf("Result: Filesystem has ERRORS - %s\n", fsck_get_error_message(result));
    }
    
    // Clean up
    fclose(g_fs_image);
    
    return (result == FSCK_OK) ? 0 : 1;
}
