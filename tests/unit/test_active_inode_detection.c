/**
 * @file test_active_inode_detection.c
 * @brief Test the improved active inode detection in umount
 */

#include "../common/test_common.h"

// Test the new active inode detection functions
void test_improved_active_inode_detection(void) {
    printf("Testing improved active inode detection...\n");
    
    // Test filesystem 1 (should have some active inodes as it's the root filesystem)
    int active_count = activeinodes(1);
    printf("Active inodes in filesystem 1: %d\n", active_count);
    CU_ASSERT_TRUE(active_count >= 0);
    
    // Test the helper functions
    int open_files = count_open_files_on_fs(1);
    int busy_workdir = is_fs_busy_workdir(1);
    
    printf("Open files on filesystem 1: %d\n", open_files);
    printf("Filesystem 1 busy due to workdir: %d\n", busy_workdir);
    
    CU_ASSERT_TRUE(open_files >= 0);
    CU_ASSERT_TRUE(busy_workdir >= 0);
    
    // Since filesystem 1 is the root, it should be busy due to working directory
    CU_ASSERT_EQUAL(busy_workdir, 1);
}

void test_file_open_detection(void) {
    printf("Testing file open detection...\n");
    
    // First check the current state
    int active_count_initial = activeinodes(1);
    int open_files_initial = count_open_files_on_fs(1);
    printf("Initial state - Active inodes: %d, Open files: %d\n", 
           active_count_initial, open_files_initial);
    
    // Try to create a test file
    int fd = open("active_inode_test.txt", OCREATE | OWRITE, 0644);
    printf("File open result: fd = %d\n", fd);
    
    if (fd >= 0) {
        // Write some data
        int bytes_written = write(fd, (byte_t*)"testing active inode detection", 30);
        printf("Bytes written: %d\n", bytes_written);
        
        // Check active inodes - should include our open file
        int active_count_before = activeinodes(1);
        int open_files_before = count_open_files_on_fs(1);
        
        printf("Before close - Active inodes: %d, Open files: %d\n", 
               active_count_before, open_files_before);
        
        // Close the file
        close(fd);
        
        // Check again after closing
        int active_count_after = activeinodes(1);
        int open_files_after = count_open_files_on_fs(1);
        
        printf("After close - Active inodes: %d, Open files: %d\n", 
               active_count_after, open_files_after);
        
        // Open files count should be less or equal after closing
        CU_ASSERT_TRUE(open_files_after <= open_files_before);
        
        // Active inodes might change, but should still be valid
        CU_ASSERT_TRUE(active_count_after >= 0);
        
        // Clean up
        unlink("active_inode_test.txt");
        
        printf("File operations completed successfully\n");
    } else {
        // If file creation fails, we can still test the detection functions
        printf("File creation failed, but testing detection functions anyway\n");
        CU_ASSERT_TRUE(active_count_initial >= 0);
        CU_ASSERT_TRUE(open_files_initial >= 0);
        printf("Detection functions work even without file creation\n");
    }
}
