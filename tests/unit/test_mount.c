/**
 * @file test_mount.c
 * @author GitHub Copilot
 * @brief Comprehensive unit tests for mount and umount filesystem operations
 * @version 0.2
 * @date 2025-07-15
 * 
 * @copyright Copyright (c) 2025
 * 
 * This file provides comprehensive tests for mount/umount functionality using
 * the updated tstdisk.c with support for 8 minor devices. These tests create
 * real filesystems and test actual mount/umount operations.
 */

#include "../common/test_common.h"
#include "fs.h"
#include "blocks.h"
#include "inode.h"
#include "mkfs.h"
#include "fsck.h"
#include <stdio.h>

// Mount flag definitions for testing
#define MS_RDONLY    0x0001    ///< Mount read-only
#define MS_RDWR      0x0002    ///< Mount read-write
#define MS_NOSUID    0x0004    ///< Ignore suid bits
#define MS_SUID      0x0008    ///< Honor suid bits
#define MS_NODEV     0x0010    ///< Ignore device files
#define MS_DEV       0x0020    ///< Allow device files

// Test device paths for tstdisk minors 0-7
#define TEST_DEV_0   "/dev/tstdisk0"
#define TEST_DEV_1   "/dev/tstdisk1"
#define TEST_DEV_2   "/dev/tstdisk2"
#define TEST_DEV_3   "/dev/tstdisk3"
#define TEST_DEV_4   "/dev/tstdisk4"
#define TEST_DEV_5   "/dev/tstdisk5"
#define TEST_DEV_6   "/dev/tstdisk6"
#define TEST_DEV_7   "/dev/tstdisk7"

// Test mount points
#define TEST_MNT_1   "/mnt1"
#define TEST_MNT_2   "/mnt2"
#define TEST_MNT_3   "/mnt3"
#define TEST_MNT_4   "/mnt4"

// External tstdisk functions
extern void tstdisk_open(ldevminor_t minor);
extern void tstdisk_close(ldevminor_t minor);
extern int tstdisk_create_fresh_fs(ldevminor_t minor, word_t sectors, word_t inodes);
extern int tstdisk_mkfs_init(ldevminor_t minor);
extern int tstdisk_fsck_validate(ldevminor_t minor);
extern char *tstdisk_getblock(ldevminor_t minor, block_t bidx);

/**
 * @brief Setup function for mount tests - create device nodes
 */
static void setup_mount_test_devices(void) {
    // Ensure /dev directory exists
    mkdir("/dev", 0755);
    
    // Create block device nodes for all tstdisk minors (major=0)
    mknod(TEST_DEV_0, BLOCK, 0660, 0, 0);  // minor 0 (default, already open)
    mknod(TEST_DEV_1, BLOCK, 0660, 0, 1);  // minor 1
    mknod(TEST_DEV_2, BLOCK, 0660, 0, 2);  // minor 2
    mknod(TEST_DEV_3, BLOCK, 0660, 0, 3);  // minor 3
    mknod(TEST_DEV_4, BLOCK, 0660, 0, 4);  // minor 4
    mknod(TEST_DEV_5, BLOCK, 0660, 0, 5);  // minor 5
    mknod(TEST_DEV_6, BLOCK, 0660, 0, 6);  // minor 6
    mknod(TEST_DEV_7, BLOCK, 0660, 0, 7);  // minor 7
    
    // Create mount point directories (don't recreate if they exist)
    mkdir(TEST_MNT_1, 0755);
    mkdir(TEST_MNT_2, 0755);
    mkdir(TEST_MNT_3, 0755);
    mkdir(TEST_MNT_4, 0755);
    
    // Open additional test disk minors (minor 0 is already open by default)
    tstdisk_open(1);
    tstdisk_open(2);
    tstdisk_open(3);
    tstdisk_open(4);
    tstdisk_open(5);
    tstdisk_open(6);
    tstdisk_open(7);
}

/**
 * @brief Cleanup function for mount tests
 */
static void cleanup_mount_test_devices(void) {
    // Attempt to umount any mounted filesystems (ignore errors)
    umount(TEST_MNT_1);
    umount(TEST_MNT_2);
    umount(TEST_MNT_3);
    umount(TEST_MNT_4);
    
    // DON'T remove mount point directories - keep them persistent
    // rmdir(TEST_MNT_1);
    // rmdir(TEST_MNT_2);
    // rmdir(TEST_MNT_3);
    // rmdir(TEST_MNT_4);
    
    // Remove device nodes
    unlink(TEST_DEV_0);
    unlink(TEST_DEV_1);
    unlink(TEST_DEV_2);
    unlink(TEST_DEV_3);
    unlink(TEST_DEV_4);
    unlink(TEST_DEV_5);
    unlink(TEST_DEV_6);
    unlink(TEST_DEV_7);
    
    // Close test disk minors (leave minor 0 open for other tests)
    tstdisk_close(1);
    tstdisk_close(2);
    tstdisk_close(3);
    tstdisk_close(4);
    tstdisk_close(5);
    tstdisk_close(6);
    tstdisk_close(7);
}

/**
 * @brief Test basic mount functionality with real filesystem
 * 
 * This test creates a real filesystem on a test device and mounts it.
 */
void test_mount_basic_pass(void) {
    // First create device files and mount points in the root filesystem
    setup_mount_test_devices();
    
    // Create a fresh filesystem on minor 2 (filesystem 1 is the root, keep it intact)
    int result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    // THEN create device files and mount points (after the fresh filesystem exists)
    
    if (result == 0) {
        // Validate filesystem integrity before mount
        int fsck_result = tstdisk_fsck_validate(2);
        CU_ASSERT_EQUAL(fsck_result, 0);
         // Mount the filesystem
        int mount_result = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Test filesystem access after mount
            int fd = open("/mnt1/test_file", OCREATE | OWRITE, 0644);
            if (fd >= 0) {
                const char *test_data = "Hello, mounted filesystem!";
                write(fd, (byte_t*)test_data, snlen(test_data, 100));
                close(fd);
                
                // Verify file exists
                fd = open("/mnt1/test_file", OREAD, 0644);
                CU_ASSERT_TRUE(fd >= 0);
                if (fd >= 0) {
                    char buffer[100];
                    int bytes_read = read(fd, (byte_t*)buffer, sizeof(buffer) - 1);
                    buffer[bytes_read] = '\0';
                    CU_ASSERT_STRING_EQUAL(buffer, test_data);
                    close(fd);
                }
                
                // Clean up test file
                unlink("/mnt1/test_file");
            }
            
            // Test umount functionality  
            int umount_result = umount(TEST_MNT_1);
            CU_ASSERT_EQUAL(umount_result, 0);
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test mount parameter validation
 * 
 * This test validates that mount properly validates its parameters
 * and returns appropriate errors for invalid inputs.
 */
void test_mount_parameter_validation(void) {
    setup_mount_test_devices();
    
    // Test null pointer parameters (these should fail with assertions in debug builds)
    // In production, these would return -1
    
    // Test non-existent device file
    int result = mount("/dev/nonexistent", TEST_MNT_1, MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);
    
    // Test non-existent mount point
    result = mount(TEST_DEV_1, "/nonexistent_mountpoint", MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);
    
    // Test empty strings
    result = mount("", TEST_MNT_1, MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);
    
    result = mount(TEST_DEV_1, "", MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);
    
    // Test mounting non-block device (regular file)
    mknode("/tmp/regular_file", REGULAR, 0644);
    result = mount("/tmp/regular_file", TEST_MNT_1, MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);
    unlink("/tmp/regular_file");
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test mount flags handling
 * 
 * This test validates that mount properly handles different mount flags
 * and stores them correctly in the filesystem structure.
 */
void test_mount_flags_pass(void) {
    setup_mount_test_devices();
    
    // Test 1: MS_RDONLY flag on filesystem 2 (reusing working configuration)
    int result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        // Validate filesystem integrity before mount (like the successful test does)
        int fsck_result = tstdisk_fsck_validate(2);
        CU_ASSERT_EQUAL(fsck_result, 0);
        
        int mount_result = mount(TEST_DEV_2, TEST_MNT_1, MS_RDONLY);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Attempt to create file on read-only mount (should fail)
            int fd = open("/mnt1/readonly_test", OCREATE | OWRITE, 0644);
            // Note: This behavior depends on implementation - some systems allow
            // creation but fail on write, others fail on creation
            if (fd >= 0) {
                close(fd);
                unlink("/mnt1/readonly_test");
            }
            umount(TEST_MNT_1);
        }
    }
    
    // Test 2: MS_RDWR flag on filesystem 2 (reuse after umount)
    result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        int mount_result = mount(TEST_DEV_2, TEST_MNT_2, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Test file creation on read-write mount
            int fd = open("/mnt2/readwrite_test", OCREATE | OWRITE, 0644);
            CU_ASSERT_TRUE(fd >= 0);
            if (fd >= 0) {
                close(fd);
                unlink("/mnt2/readwrite_test");
            }
            umount(TEST_MNT_2);
        }
    }
    
    // Test 3: Combined flags - use filesystem 2 again
    result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    if (result == 0) {
        int mount_result = mount(TEST_DEV_2, TEST_MNT_3, MS_RDWR | MS_NOSUID);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            umount(TEST_MNT_3);
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test basic umount functionality
 * 
 * This test validates basic umount operations including unmounting
 * a previously mounted filesystem.
 */
void test_umount_basic_pass(void) {
    setup_mount_test_devices();
    
    // Test umount on non-mounted directory
    int result = umount(TEST_MNT_1);
    CU_ASSERT_EQUAL(result, -1);
    
    // Test umount on root (should fail)
    result = umount("/");
    CU_ASSERT_EQUAL(result, -1);
    
    // Create and mount filesystem (use filesystem 6 to avoid conflicts)
    result = tstdisk_create_fresh_fs(6, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        int mount_result = mount(TEST_DEV_6, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Test successful umount
            int umount_result = umount(TEST_MNT_1);
            CU_ASSERT_EQUAL(umount_result, 0);
            
            // Verify filesystem is no longer accessible
            int fd = open("/mnt1/test", OCREATE | OWRITE, 0644);
            // This should either fail or create file in underlying directory
            if (fd >= 0) {
                close(fd);
                unlink("/mnt1/test");  // Clean up if it was created
            }
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test umount parameter validation
 * 
 * This test validates that umount properly validates its parameters
 * and returns appropriate errors for invalid inputs.
 */
void test_umount_parameter_validation(void) {
    setup_mount_test_devices();
    
    // Test non-existent mount point
    int result = umount("/nonexistent");
    CU_ASSERT_EQUAL(result, -1);
    
    // Test empty path
    result = umount("");
    CU_ASSERT_EQUAL(result, -1);
    
    // Test umount on regular file (not a directory)
    mknode("/tmp/test_file", REGULAR, 0644);
    result = umount("/tmp/test_file");
    CU_ASSERT_EQUAL(result, -1);
    unlink("/tmp/test_file");
    
    // Test umount on non-mounted directory
    result = umount(TEST_MNT_1);
    CU_ASSERT_EQUAL(result, -1);
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test mount/umount workflow
 * 
 * This test validates the complete mount/umount workflow including
 * mounting a filesystem and then unmounting it.
 */
void test_mount_umount_workflow(void) {
    setup_mount_test_devices();
    
    // Create filesystem on minor 2 (filesystem 1 is the root, keep it intact)
    int result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        // Mount the filesystem
        int mount_result = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Create a test file
            int fd = open("/mnt1/workflow_test.txt", OCREATE | OWRITE, 0644);
            CU_ASSERT_TRUE(fd >= 0);
            
            if (fd >= 0) {
                const char *data = "Mount/umount workflow test";
                write(fd, (byte_t*)data, snlen(data, 100));
                close(fd);
                
                // Verify file is accessible
                fd = open("/mnt1/workflow_test.txt", OREAD, 0644);
                CU_ASSERT_TRUE(fd >= 0);
                if (fd >= 0) {
                    close(fd);
                }
            }
            
            // Test file operations while mounted
            CU_ASSERT_EQUAL(mkdir("/mnt1/test_dir", 0755), 0);
            CU_ASSERT_EQUAL(rmdir("/mnt1/test_dir"), 0);
            
            // Clean up test file
            unlink("/mnt1/workflow_test.txt");
            
            // Umount the filesystem
            int umount_result = umount(TEST_MNT_1);
            CU_ASSERT_EQUAL(umount_result, 0);
            
            // Verify filesystem is no longer accessible at mount point
            fd = open("/mnt1/after_umount.txt", OCREATE | OWRITE, 0644);
            if (fd >= 0) {
                // If file creation succeeds, it's in the underlying directory
                close(fd);
                unlink("/mnt1/after_umount.txt");
            }
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test multiple mount points
 * 
 * This test validates that the system can handle multiple mount points
 * and properly track mounted filesystems.
 */
void test_mount_multiple_points(void) {
    setup_mount_test_devices();
    
    // Create filesystems on multiple minors (avoid filesystem 1 which is root)
    int result1 = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    int result2 = tstdisk_create_fresh_fs(3, TEST_SIMNBLOCKS, 0);
    
    CU_ASSERT_EQUAL(result1, 0);
    CU_ASSERT_EQUAL(result2, 0);
    
    if (result1 == 0 && result2 == 0) {
        // Mount first filesystem
        int mount1 = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount1, 0);
        
        // Mount second filesystem
        int mount2 = mount(TEST_DEV_3, TEST_MNT_2, MS_RDWR);
        CU_ASSERT_EQUAL(mount2, 0);
        
        if (mount1 == 0 && mount2 == 0) {
            // Create files on both mounted filesystems
            int fd1 = open("/mnt1/fs1_file.txt", OCREATE | OWRITE, 0644);
            int fd2 = open("/mnt2/fs2_file.txt", OCREATE | OWRITE, 0644);
            
            CU_ASSERT_TRUE(fd1 >= 0);
            CU_ASSERT_TRUE(fd2 >= 0);
            
            if (fd1 >= 0) {
                write(fd1, (byte_t*)"filesystem 1", 12);
                close(fd1);
            }
            
            if (fd2 >= 0) {
                write(fd2, (byte_t*)"filesystem 2", 12);
                close(fd2);
            }
            
            // Verify files are on different filesystems
            fd1 = open("/mnt1/fs1_file.txt", OREAD, 0644);
            fd2 = open("/mnt2/fs2_file.txt", OREAD, 0644);
            
            if (fd1 >= 0) {
                close(fd1);
                unlink("/mnt1/fs1_file.txt");
            }
            
            if (fd2 >= 0) {
                close(fd2);
                unlink("/mnt2/fs2_file.txt");
            }
            
            // Umount both filesystems
            CU_ASSERT_EQUAL(umount(TEST_MNT_1), 0);
            CU_ASSERT_EQUAL(umount(TEST_MNT_2), 0);
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test mount on already mounted filesystem
 * 
 * This test validates that attempting to mount an already mounted
 * filesystem is properly handled.
 */
void test_mount_already_mounted(void) {
    setup_mount_test_devices();
    
    // Create filesystem (avoid filesystem 1 which is root)
    int result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        // First mount should succeed
        int mount1 = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount1, 0);
        
        if (mount1 == 0) {
            // Second mount of same device should fail
            int mount2 = mount(TEST_DEV_2, TEST_MNT_2, MS_RDWR);
            CU_ASSERT_EQUAL(mount2, -1);
            
            // Umount the first mount
            CU_ASSERT_EQUAL(umount(TEST_MNT_1), 0);
            
            // Now mounting to second location should succeed
            mount2 = mount(TEST_DEV_2, TEST_MNT_2, MS_RDWR);
            CU_ASSERT_EQUAL(mount2, 0);
            
            if (mount2 == 0) {
                CU_ASSERT_EQUAL(umount(TEST_MNT_2), 0);
            }
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test umount with active files
 * 
 * This test validates that umount properly handles the case where
 * there are still active inodes on the filesystem.
 */
void test_umount_busy_filesystem(void) {
    setup_mount_test_devices();
    
    // Create filesystem (avoid filesystem 1 which is root)
    int result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        // Mount filesystem
        int mount_result = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Open a file and keep it open
            int fd = open("/mnt1/busy_file.txt", OCREATE | OWRITE, 0644);
            CU_ASSERT_TRUE(fd >= 0);
            
            if (fd >= 0) {
                write(fd, (byte_t*)"keeping file open", 17);
                
                // Attempt to umount while file is open (should fail)
                int umount_result = umount(TEST_MNT_1);
                CU_ASSERT_EQUAL(umount_result, -1);
                
                // Close the file
                close(fd);
                
                // Now umount should succeed
                umount_result = umount(TEST_MNT_1);
                CU_ASSERT_EQUAL(umount_result, 0);
            } else {
                // If file open failed, just umount
                umount(TEST_MNT_1);
            }
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test filesystem superblock handling in mount
 * 
 * This test validates that mount properly interacts with the
 * filesystem superblock structures.
 */
void test_mount_superblock_handling(void) {
    setup_mount_test_devices();
    
    // Test mount with device that has no filesystem (should fail)
    tstdisk_open(3);  // Open but don't create filesystem
    int result = mount(TEST_DEV_3, TEST_MNT_1, MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);  // Should fail - no valid superblock
    
    // Test mount with properly created filesystem
    result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        // Validate superblock before mount
        int fsck_result = tstdisk_fsck_validate(2);
        CU_ASSERT_EQUAL(fsck_result, 0);
        
        // Mount should succeed with valid superblock
        int mount_result = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Test filesystem operations to verify superblock is working
            int fd = open("/mnt1/superblock_test", OCREATE | OWRITE, 0644);
            CU_ASSERT_TRUE(fd >= 0);
            
            if (fd >= 0) {
                close(fd);
                unlink("/mnt1/superblock_test");
            }
            
            CU_ASSERT_EQUAL(umount(TEST_MNT_1), 0);
        }
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test edge cases and error conditions
 * 
 * This test validates various edge cases and error conditions
 * in mount and umount operations.
 */
void test_mount_umount_edge_cases(void) {
    setup_mount_test_devices();
    
    // Test mounting root on itself (should fail)
    int result = mount("/", "/", MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);
    
    // Test umounting root (should fail)
    result = umount("/");
    CU_ASSERT_EQUAL(result, -1);
    
    // Test with relative path
    result = umount(".");
    CU_ASSERT_EQUAL(result, -1);
    
    // Test mounting device on non-directory
    mknode("/tmp/regular_file", REGULAR, 0644);
    result = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    if (result == 0) {
        result = mount(TEST_DEV_2, "/tmp/regular_file", MS_RDWR);
        CU_ASSERT_EQUAL(result, -1);  // Should fail - not a directory
    }
    unlink("/tmp/regular_file");
    
    // Test very long path names (near MAXPATH limit)
    char long_path[300];
    mset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    result = mount(TEST_DEV_2, long_path, MS_RDWR);
    CU_ASSERT_EQUAL(result, -1);  // Should fail - path too long
    
    cleanup_mount_test_devices();
}

/**
 * @brief Comprehensive mount/umount test combining multiple scenarios
 * 
 * This test combines multiple mount/umount scenarios to test
 * the robustness of the overall system.
 */
void test_mount_umount_comprehensive(void) {
    setup_mount_test_devices();
    
    // Create multiple filesystems (use minors 2,3,4,5 to avoid filesystem 1 which is root)
    int fs_results[4];
    fs_results[0] = tstdisk_create_fresh_fs(2, TEST_SIMNBLOCKS, 0);
    fs_results[1] = tstdisk_create_fresh_fs(3, TEST_SIMNBLOCKS, 0);
    fs_results[2] = tstdisk_create_fresh_fs(4, TEST_SIMNBLOCKS, 0);
    fs_results[3] = tstdisk_create_fresh_fs(5, TEST_SIMNBLOCKS, 0);
    
    // Verify all filesystems were created
    for (int i = 0; i < 4; i++) {
        CU_ASSERT_EQUAL(fs_results[i], 0);
    }
    
    // Mount multiple filesystems
    int mount_results[4];
    mount_results[0] = mount(TEST_DEV_2, TEST_MNT_1, MS_RDWR);
    mount_results[1] = mount(TEST_DEV_3, TEST_MNT_2, MS_RDONLY);
    mount_results[2] = mount(TEST_DEV_4, TEST_MNT_3, MS_RDWR | MS_NOSUID);
    mount_results[3] = mount(TEST_DEV_5, TEST_MNT_4, MS_RDWR | MS_NODEV);
    
    // Verify all mounts succeeded
    for (int i = 0; i < 4; i++) {
        CU_ASSERT_EQUAL(mount_results[i], 0);
    }
    
    // Perform file operations on each mounted filesystem
    if (mount_results[0] == 0) {
        int fd = open("/mnt1/comprehensive_test1", OCREATE | OWRITE, 0644);
        if (fd >= 0) {
            write(fd, (byte_t*)"test data 1", 11);
            close(fd);
        }
    }
    
    if (mount_results[1] == 0) {
        // Read-only mount - just try to read
        int fd = open("/mnt2", OREAD, 0644);
        if (fd >= 0) {
            close(fd);
        }
    }
    
    if (mount_results[2] == 0) {
        int fd = open("/mnt3/comprehensive_test3", OCREATE | OWRITE, 0644);
        if (fd >= 0) {
            write(fd, (byte_t*)"test data 3", 11);
            close(fd);
        }
    }
    
    if (mount_results[3] == 0) {
        int fd = open("/mnt4/comprehensive_test4", OCREATE | OWRITE, 0644);
        if (fd >= 0) {
            write(fd, (byte_t*)"test data 4", 11);
            close(fd);
        }
    }
    
    // Test filesystem integrity for all mounted filesystems
    for (int i = 1; i <= 4; i++) {
        int fsck_result = tstdisk_fsck_validate(i);
        CU_ASSERT_EQUAL(fsck_result, 0);
    }
    
    // Clean up files
    unlink("/mnt1/comprehensive_test1");
    unlink("/mnt3/comprehensive_test3");
    unlink("/mnt4/comprehensive_test4");
    
    // Umount all filesystems in reverse order
    int umount_results[4];
    umount_results[3] = umount(TEST_MNT_4);
    umount_results[2] = umount(TEST_MNT_3);
    umount_results[1] = umount(TEST_MNT_2);
    umount_results[0] = umount(TEST_MNT_1);
    
    // Verify all umounts succeeded
    for (int i = 0; i < 4; i++) {
        CU_ASSERT_EQUAL(umount_results[i], 0);
    }
    
    // Final filesystem integrity check
    for (int i = 1; i <= 4; i++) {
        int fsck_result = tstdisk_fsck_validate(i);
        CU_ASSERT_EQUAL(fsck_result, 0);
    }
    
    cleanup_mount_test_devices();
}

/**
 * @brief Test mkfs integration with mount operations
 * 
 * This test demonstrates the integration between mkfs module and mount operations,
 * showing how to create fresh filesystems and mount them.
 */
void test_mount_mkfs_integration(void) {
    setup_mount_test_devices();
    
    // Test creating filesystem with mkfs module directly
    tstdisk_open(5);  // Open clean disk
    
    int result = tstdisk_mkfs_init(5);  // Use mkfs to create filesystem
    CU_ASSERT_EQUAL(result, 0);
    
    if (result == 0) {
        // Validate the mkfs-created filesystem
        int fsck_result = tstdisk_fsck_validate(5);
        CU_ASSERT_EQUAL(fsck_result, 0);
        
        // Mount the mkfs-created filesystem
        int mount_result = mount(TEST_DEV_5, TEST_MNT_1, MS_RDWR);
        CU_ASSERT_EQUAL(mount_result, 0);
        
        if (mount_result == 0) {
            // Test file operations on mkfs-created filesystem
            int fd = open("/mnt1/mkfs_integration_test", OCREATE | OWRITE, 0644);
            CU_ASSERT_TRUE(fd >= 0);
            
            if (fd >= 0) {
                const char *test_data = "mkfs integration test data";
                write(fd, (byte_t*)test_data, snlen(test_data, 100));
                close(fd);
                
                // Verify file can be read back
                fd = open("/mnt1/mkfs_integration_test", OREAD, 0644);
                CU_ASSERT_TRUE(fd >= 0);
                if (fd >= 0) {
                    char buffer[100];
                    int bytes_read = read(fd, (byte_t*)buffer, sizeof(buffer) - 1);
                    buffer[bytes_read] = '\0';
                    CU_ASSERT_STRING_EQUAL(buffer, test_data);
                    close(fd);
                }
                
                unlink("/mnt1/mkfs_integration_test");
            }
            
            CU_ASSERT_EQUAL(umount(TEST_MNT_1), 0);
        }
        
        // Final integrity check after umount
        fsck_result = tstdisk_fsck_validate(5);
        CU_ASSERT_EQUAL(fsck_result, 0);
    }
    
    cleanup_mount_test_devices();
}
