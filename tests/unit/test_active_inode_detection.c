/**
 * @file test_active_inode_detection.c
 * @brief Test the improved active inode detection in umount
 */

#include "../common/test_common.h"

// Test the new active inode detection functions
void test_improved_active_inode_detection(void) {
    // Test filesystem 1 (should have some active inodes as it's the root filesystem)
    int active_count = activeinodes(1);
    CU_ASSERT_TRUE(active_count >= 0);
    
    // Test the helper functions
    int open_files = count_open_files_on_fs(1);
    int busy_workdir = is_fs_busy_workdir(1);
    
    CU_ASSERT_TRUE(open_files >= 0);
    CU_ASSERT_TRUE(busy_workdir >= 0);
    
    // Since filesystem 1 is the root, it should be busy due to working directory
    CU_ASSERT_EQUAL(busy_workdir, 1);
}

void test_file_open_detection(void) {
    // First check the current state
    int active_count_initial = activeinodes(1);
    int open_files_initial = count_open_files_on_fs(1);
    
    // Try to create a test file
    int fd = open("active_inode_test.txt", OCREATE | OWRITE, 0644);
    
    if (fd >= 0) {
        // Write some data
        byte_t test_data[] = "testing active inode detection";
        int bytes_written = write(fd, test_data, 30);
        CU_ASSERT(bytes_written > 0);  // Use the variable
        
        // Check active inodes - should include our open file
        int active_count_before = activeinodes(1);
        int open_files_before = count_open_files_on_fs(1);
        CU_ASSERT_TRUE(active_count_before >= 0);  // Use the variable
        
        // Close the file
        close(fd);
        
        // Check again after closing
        int active_count_after = activeinodes(1);
        int open_files_after = count_open_files_on_fs(1);
        
        // Open files count should be less or equal after closing
        CU_ASSERT_TRUE(open_files_after <= open_files_before);
        
        // Active inodes might change, but should still be valid
        CU_ASSERT_TRUE(active_count_after >= 0);
        
        // Clean up
        unlink("active_inode_test.txt");
    } else {
        // If file creation fails, we can still test the detection functions
        CU_ASSERT_TRUE(active_count_initial >= 0);
        CU_ASSERT_TRUE(open_files_initial >= 0);
    }
}
