/**
 * @file test_main.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Simple test runner using CUNIT_CI_RUN for modular STIX tests
 * @version   CUNIT_CI_TEST(test_mount_umount_comprehensive),
  CUNIT_CI_TEST(test_clist_tstcon_pattern_mode),
  CUNIT_CI_TEST(test_clist_tstcon_sequential_mode),
  CUNIT_CI_TEST(test_clist_tstcon_random_mode),
  CUNIT_CI_TEST(test_clist_tstcon_stress_mode),
  CUNIT_CI_TEST(test_clist_tstcon_multiple_devices),
  CUNIT_CI_TEST(test_clist_tstcon_statistics_tracking),
  CUNIT_CI_TEST(test_clist_tstcon_comprehensive)
)
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 */

#include "common/test_common.h"
#include "CUnit/CUnitCI.h"

// Global variables needed by tests
fsnum_t fs1;

// Declare test functions from individual modules
extern void test_typesize_pass(void);
extern void test_buffer_pass(void);
extern void test_block_pass(void);
extern void test_inode_pass(void);
extern void test_file_pass(void);
extern void test_clist_pass(void);
extern void test_buffer_edge_cases(void);
extern void test_filesystem_simple_edge_cases(void);
extern void test_lseek_pass(void);
extern void test_link_pass(void);
extern void test_rename_pass(void);
extern void test_stat_pass(void);
extern void test_chmod_chown_pass(void);
extern void test_directory_navigation_pass(void);
extern void test_sync_pass(void);
extern void test_mknode_pass(void);
extern void test_directory_operations_pass(void);
extern void test_clist_tstcon_pattern_mode(void);
extern void test_clist_tstcon_sequential_mode(void);
extern void test_clist_tstcon_random_mode(void);
extern void test_clist_tstcon_stress_mode(void);
extern void test_clist_tstcon_multiple_devices(void);
extern void test_clist_tstcon_statistics_tracking(void);
extern void test_clist_tstcon_comprehensive(void);
extern void test_fsck_pass(void);
extern void test_fsck_superblock_pass(void);
extern void test_fsck_inodes_pass(void);
extern void test_fsck_bitmap_pass(void);
extern void test_fsck_filesystem_pass(void);
extern void test_mkfs_pass(void);
extern void test_mkfs_layout_pass(void);
extern void test_mkfs_superblock_pass(void);
extern void test_mkfs_inode_table_pass(void);
extern void test_mkfs_bitmap_pass(void);
extern void test_mkfs_root_directory_pass(void);
extern void test_mkfs_complete_filesystem_pass(void);
extern void test_mkfs_edge_cases_pass(void);
extern void test_filesystem_integrity_validation(void);
extern void test_filesystem_stress_with_validation(void);
extern void test_filesystem_mkfs_fresh_creation(void);
extern void test_mount_basic_pass(void);
extern void test_mount_parameter_validation(void);
extern void test_mount_flags_pass(void);
extern void test_umount_basic_pass(void);
extern void test_umount_parameter_validation(void);
extern void test_mount_umount_workflow(void);
extern void test_mount_multiple_points(void);
extern void test_mount_already_mounted(void);
extern void test_umount_busy_filesystem(void);
extern void test_mount_superblock_handling(void);
extern void test_mount_umount_edge_cases(void);
extern void test_mount_umount_comprehensive(void);

// Suite setup function - called once before all tests
CU_SUITE_SETUP() {
  init_dd();
  init_buffers();
  init_inodes();
  init_fs(); 
  init_clist();
  
  // Open test disk (bdevopen returns void, so we can't check return value)
  bdevopen((ldev_t){{0, 0}});
  
  return CUE_SUCCESS;
}

// Suite teardown function - called once after all tests
CU_SUITE_TEARDOWN() {
  // Sync all buffers before closing
  syncall_buffers(false);
  
  // Close test disk
  bdevclose((ldev_t){{0, 0}});
  
  return CUE_SUCCESS;
}

// Test setup function - called before each test
CU_TEST_SETUP() {
}

// Test teardown function - called after each test
CU_TEST_TEARDOWN() {
}

// Use CUNIT_CI_RUN for automatic test registration and execution
CUNIT_CI_RUN(
  "STIX-modular-tests",
  CUNIT_CI_TEST(test_fsck_pass),
  CUNIT_CI_TEST(test_fsck_superblock_pass),
  CUNIT_CI_TEST(test_fsck_inodes_pass),
  CUNIT_CI_TEST(test_fsck_bitmap_pass),
  CUNIT_CI_TEST(test_fsck_filesystem_pass),
  CUNIT_CI_TEST(test_mkfs_pass),
  CUNIT_CI_TEST(test_mkfs_layout_pass),
  CUNIT_CI_TEST(test_mkfs_superblock_pass),
  CUNIT_CI_TEST(test_mkfs_inode_table_pass),
  CUNIT_CI_TEST(test_mkfs_bitmap_pass),
  CUNIT_CI_TEST(test_mkfs_root_directory_pass),
  CUNIT_CI_TEST(test_mkfs_complete_filesystem_pass),
  CUNIT_CI_TEST(test_mkfs_edge_cases_pass),
  CUNIT_CI_TEST(test_filesystem_mkfs_fresh_creation),
  CUNIT_CI_TEST(test_typesize_pass),
  CUNIT_CI_TEST(test_buffer_pass),
  CUNIT_CI_TEST(test_block_pass),
  CUNIT_CI_TEST(test_inode_pass),
  CUNIT_CI_TEST(test_file_pass),
  CUNIT_CI_TEST(test_clist_pass),
  CUNIT_CI_TEST(test_buffer_edge_cases),
  CUNIT_CI_TEST(test_filesystem_simple_edge_cases),
  CUNIT_CI_TEST(test_filesystem_integrity_validation),
  CUNIT_CI_TEST(test_filesystem_stress_with_validation),
  CUNIT_CI_TEST(test_lseek_pass),
  CUNIT_CI_TEST(test_link_pass),
  CUNIT_CI_TEST(test_rename_pass),
  CUNIT_CI_TEST(test_stat_pass),
  CUNIT_CI_TEST(test_chmod_chown_pass),
  CUNIT_CI_TEST(test_directory_navigation_pass),
  CUNIT_CI_TEST(test_sync_pass),
  CUNIT_CI_TEST(test_mknode_pass),
  CUNIT_CI_TEST(test_directory_operations_pass),
  CUNIT_CI_TEST(test_mount_basic_pass),
  CUNIT_CI_TEST(test_mount_parameter_validation),
  CUNIT_CI_TEST(test_mount_flags_pass),
  CUNIT_CI_TEST(test_umount_basic_pass),
  CUNIT_CI_TEST(test_umount_parameter_validation),
  CUNIT_CI_TEST(test_mount_umount_workflow),
  CUNIT_CI_TEST(test_mount_multiple_points),
  CUNIT_CI_TEST(test_mount_already_mounted),
  CUNIT_CI_TEST(test_umount_busy_filesystem),
  CUNIT_CI_TEST(test_mount_superblock_handling),
  CUNIT_CI_TEST(test_mount_umount_edge_cases),
  CUNIT_CI_TEST(test_mount_umount_comprehensive),
  CUNIT_CI_TEST(test_clist_tstcon_pattern_mode),
  CUNIT_CI_TEST(test_clist_tstcon_sequential_mode),
  CUNIT_CI_TEST(test_clist_tstcon_random_mode),
  CUNIT_CI_TEST(test_clist_tstcon_stress_mode),
  CUNIT_CI_TEST(test_clist_tstcon_multiple_devices),
  CUNIT_CI_TEST(test_clist_tstcon_statistics_tracking),
  CUNIT_CI_TEST(test_clist_tstcon_comprehensive)
)
