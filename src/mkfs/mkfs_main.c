/**
 * @file mkfs_standalone.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Standalone make filesystem utility
 * @version 0.1
 * @date 2025-07-14
 * 
 * @copyright Copyright (c) 2025
 * 
 * This is a standalone executable that demonstrates the use of the mkfs module
 * to create a filesystem on an image file.
 */

#include "mkfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

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
    long offset = sector_number * MKFS_BLOCKSIZE;
    if (fseek(g_fs_image, offset, SEEK_SET) != 0) {
        return 2;  // Seek failed
    }
    
    // Read the sector
    size_t bytes_read = fread(buffer, 1, MKFS_BLOCKSIZE, g_fs_image);
    if (bytes_read != MKFS_BLOCKSIZE) {
        return 3;  // Read failed or incomplete
    }
    
    return 0;  // Success
}

/**
 * @brief Write a sector to the filesystem image file
 * 
 * @param sector_number The sector number to write
 * @param buffer Buffer containing the sector data
 * @return 0 on success, non-zero on failure
 */
int file_write_sector(block_t sector_number, const byte_t *buffer) {
    if (g_fs_image == NULL) {
        return 1;  // File not open
    }
    
    // Seek to the beginning of the sector
    long offset = sector_number * MKFS_BLOCKSIZE;
    if (fseek(g_fs_image, offset, SEEK_SET) != 0) {
        return 2;  // Seek failed
    }
    
    // Write the sector
    size_t bytes_written = fwrite(buffer, 1, MKFS_BLOCKSIZE, g_fs_image);
    if (bytes_written != MKFS_BLOCKSIZE) {
        return 3;  // Write failed or incomplete
    }
    
    return 0;  // Success
}

/**
 * @brief Print usage information
 */
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <device_or_image> <size_in_sectors>\n", program_name);
    printf("\n");
    printf("Create a STIX filesystem on the specified device or image file.\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  device_or_image     Path to the device or image file\n");
    printf("  size_in_sectors     Size of the filesystem in 512-byte sectors\n");
    printf("\n");
    printf("Options:\n");
    printf("  -i, --inodes NUM    Number of inodes (default: auto-calculate)\n");
    printf("  -f, --force         Force creation, overwrite existing filesystem\n");
    printf("  -v, --verbose       Verbose output\n");
    printf("  -h, --help          Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s disk.img 1024                    # Create 512KB filesystem\n", program_name);
    printf("  %s -i 256 disk.img 2048             # Create 1MB filesystem with 256 inodes\n", program_name);
    printf("  %s -v -f /dev/loop0 4096            # Create 2MB filesystem with verbose output\n", program_name);
    printf("\n");
    printf("Return codes:\n");
    printf("  0  Filesystem created successfully\n");
    printf("  1  Creation failed\n");
    printf("  2  Usage error or file access problem\n");
}

/**
 * @brief Parse command line arguments
 */
int parse_args(int argc, char *argv[], char **device, word_t *size, word_t *inodes, int *force, int *verbose) {
    int opt;
    int option_index = 0;
    
    static struct option long_options[] = {
        {"inodes",  required_argument, 0, 'i'},
        {"force",   no_argument,       0, 'f'},
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    *inodes = 0;  // Auto-calculate by default
    *force = 0;
    *verbose = 0;
    
    while ((opt = getopt_long(argc, argv, "i:fvh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                *inodes = (word_t)atoi(optarg);
                if (*inodes == 0) {
                    printf("Error: Invalid number of inodes: %s\n", optarg);
                    return -1;
                }
                break;
            case 'f':
                *force = 1;
                break;
            case 'v':
                *verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            case '?':
                return -1;
            default:
                return -1;
        }
    }
    
    // Check for required positional arguments
    if (optind + 2 != argc) {
        printf("Error: Missing required arguments\n");
        return -1;
    }
    
    *device = argv[optind];
    *size = (word_t)atoi(argv[optind + 1]);
    
    if (*size == 0) {
        printf("Error: Invalid size: %s\n", argv[optind + 1]);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Create the image file with specified size
 */
int create_image_file(const char *filename, word_t size_sectors, int verbose) {
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error: Cannot create file '%s'\n", filename);
        return 1;
    }
    
    if (verbose) {
        printf("Creating image file: %s (%u sectors, %u bytes)\n", 
               filename, size_sectors, size_sectors * MKFS_BLOCKSIZE);
    }
    
    // Write zeros to create the file
    byte_t zero_buffer[MKFS_BLOCKSIZE];
    memset(zero_buffer, 0, MKFS_BLOCKSIZE);
    
    for (word_t i = 0; i < size_sectors; i++) {
        if (fwrite(zero_buffer, 1, MKFS_BLOCKSIZE, file) != MKFS_BLOCKSIZE) {
            printf("Error: Failed to write to file\n");
            fclose(file);
            return 1;
        }
    }
    
    fclose(file);
    return 0;
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    char *device;
    word_t size_sectors, num_inodes;
    int force, verbose;
    
    // Parse command line arguments
    if (parse_args(argc, argv, &device, &size_sectors, &num_inodes, &force, &verbose) != 0) {
        print_usage(argv[0]);
        return 2;
    }
    
    if (verbose) {
        printf("STIX Make Filesystem Utility\n");
        printf("============================\n");
        printf("Device/Image: %s\n", device);
        printf("Size: %u sectors (%u KB)\n", size_sectors, (size_sectors * MKFS_BLOCKSIZE) / 1024);
        if (num_inodes > 0) {
            printf("Inodes: %u (specified)\n", num_inodes);
        } else {
            printf("Inodes: auto-calculate\n");
        }
        printf("\n");
    }
    
    // Create image file if it doesn't exist
    FILE *test_file = fopen(device, "rb");
    if (test_file == NULL) {
        // File doesn't exist, create it
        if (create_image_file(device, size_sectors, verbose) != 0) {
            return 2;
        }
    } else {
        fclose(test_file);
        if (!force) {
            printf("Error: File '%s' already exists. Use -f to force overwrite.\n", device);
            return 2;
        }
    }
    
    // Open the filesystem image file
    g_fs_image = fopen(device, "r+b");
    if (g_fs_image == NULL) {
        printf("Error: Cannot open file '%s' for writing\n", device);
        return 2;
    }
    
    // Initialize the mkfs module
    mkfs_result_t result = mkfs_init(file_read_sector, file_write_sector);
    if (result != MKFS_OK) {
        printf("Error: Failed to initialize mkfs module: %s\n", 
               mkfs_get_error_message(result));
        fclose(g_fs_image);
        return 2;
    }
    
    // Calculate filesystem layout
    mkfs_params_t params;
    result = mkfs_calculate_layout(size_sectors, num_inodes, &params);
    if (result != MKFS_OK) {
        printf("Error: Failed to calculate filesystem layout: %s\n", 
               mkfs_get_error_message(result));
        fclose(g_fs_image);
        return 1;
    }
    
    if (verbose) {
        printf("Filesystem Layout:\n");
        printf("-----------------\n");
        printf("Total sectors:     %u\n", params.total_sectors);
        printf("Inodes:            %u\n", params.calculated_inodes);
        printf("Inode sectors:     %u\n", params.inode_sectors);
        printf("Bitmap sectors:    %u\n", params.bitmap_sectors);
        printf("First data sector: %u\n", params.first_data_sector);
        printf("Data sectors:      %u\n", params.data_sectors);
        printf("\n");
    }
    
    // Create the filesystem
    result = mkfs_create_filesystem(&params);
    if (result != MKFS_OK) {
        printf("Error: Failed to create filesystem: %s\n", 
               mkfs_get_error_message(result));
        fclose(g_fs_image);
        return 1;
    }
    
    // Sync and close
    fflush(g_fs_image);
    fclose(g_fs_image);
    
    if (verbose) {
        printf("Filesystem created successfully!\n");
    } else {
        printf("Filesystem created on %s\n", device);
    }
    
    return 0;
}
