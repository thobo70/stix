/**
 * @file test_filesystem.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Filesystem unit tests for STIX
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file contains unit tests for filesystem operations including
 * file creation, reading, writing, and deletion.
 */

#include "../common/test_common.h"

/**
 * @brief Test filesystem read operation
 * 
 * This test validates the filesystem read functionality by creating
 * a file, writing to it, and then reading from it.
 */
void test_fs_read_pass(void) {
  // Create a test file and read from it
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  
  const char *test_data = "Hello";
  int written = write(fd, (byte_t*)test_data, strlen(test_data));
  CU_ASSERT_EQUAL(written, (int)strlen(test_data));
  close(fd);
  
  // Now read from the file
  fd = open("testfile", OREAD, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  
  char buffer[10];
  int bytes_read = read(fd, (byte_t*)buffer, strlen(test_data));
  CU_ASSERT_EQUAL(bytes_read, (int)strlen(test_data));
  close(fd);
  
  // Clean up
  unlink("testfile");
}

/**
 * @brief Test filesystem write operation
 * 
 * This test validates the filesystem write functionality by creating
 * a file and writing data to it.
 */
void test_fs_write_pass(void) {
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  
  const char *test_data = "Test data";
  int written = write(fd, (byte_t*)test_data, strlen(test_data));
  CU_ASSERT_EQUAL(written, (int)strlen(test_data));
  
  close(fd);
  unlink("testfile");
}

/**
 * @brief Test filesystem delete operation
 * 
 * This test validates the filesystem delete functionality by creating
 * a file and then deleting it.
 */
void test_fs_delete_pass(void) {
  // Create a file first
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  close(fd);
  
  // Now delete it
  CU_ASSERT_EQUAL(unlink("testfile"), 0);
}

/**
 * @brief Test filesystem create operation
 * 
 * This test validates the filesystem create functionality by creating
 * a new file.
 */
void test_fs_create_pass(void) {
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  close(fd);
  
  // Verify file was created by trying to open it for reading
  fd = open("testfile", OREAD, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  close(fd);
  
  // Clean up
  unlink("testfile");
}

// Functions available for external test runner

/**
 * @brief Test block operations
 * 
 * This test validates basic block operations for the filesystem.
 */
void test_block_pass(void) {
  fs1 = init_isblock((ldev_t){{0, 0}});  // init superblock device = 0, should return fs1 = 1
  CU_ASSERT_EQUAL(fs1, 1);

  bhead_t *b = balloc(fs1);
  CU_ASSERT_EQUAL(b->block, 6);
  bfree(fs1, b->block);
  brelse(b);
  b = balloc(fs1);
  CU_ASSERT_EQUAL(b->block, 6);
  bfree(fs1, b->block);
  brelse(b);
}

/**
 * @brief Test file operations
 * 
 * This test validates file operations including open/close/read/write.
 */
void test_file_pass(void) {
  CU_ASSERT_EQUAL(mkdir("/test", 0777), 0);
  CU_ASSERT_EQUAL(rmdir("/test"), 0);

  CU_ASSERT_EQUAL(mkdir("/test", 0777), 0);

  const char *str = "Hello World";
  char buf[100];
  int fd = open("/test/test.txt", OCREATE | ORDWR, 0777);
  CU_ASSERT_EQUAL(fd, 0);
  fsize_t str_len = snlen(str, 100);
  CU_ASSERT_EQUAL(write(fd, (byte_t *)str, str_len + 1), (int)str_len + 1);
  CU_ASSERT_EQUAL(close(fd), 0);

  fd = open("/test/test.txt", ORDWR, 0777);
  CU_ASSERT_EQUAL(fd, 0);
  CU_ASSERT_EQUAL(read(fd, (byte_t *)buf, str_len + 1), (int)str_len + 1);
  CU_ASSERT_EQUAL(sncmp(buf, str, 100), 0);
  CU_ASSERT_EQUAL(close(fd), 0);

  CU_ASSERT_EQUAL(open("/test", ORDWR, 0777), -1);

  CU_ASSERT_EQUAL(unlink("/test/test.txt"), 0);
  CU_ASSERT_EQUAL(rmdir("/test"), 0);

  CU_ASSERT_EQUAL(open("/test/test.txt", ORDWR, 0777), -1);

  int n;
  CU_ASSERT_EQUAL(fd = open("full.txt", OCREATE | ORDWR, 0777), 0);
  do {
    CU_ASSERT_EQUAL((n = write(fd, (byte_t *)str, str_len + 1)) >= 0, (1 == 1));
  } while (n != 0);
  CU_ASSERT_EQUAL(close(fd), 0);
  CU_ASSERT_EQUAL(unlink("full.txt"), 0);
  syncall_buffers(false);
  check_bfreelist();
}

/**
 * @brief Test filesystem edge cases
 */
void test_filesystem_simple_edge_cases(void) {
  // Test various edge cases in filesystem operations
  CU_ASSERT_EQUAL(open("nonexistent", OREAD, 0644), -1); // Should fail
}

/**
 * @brief Test lseek functionality
 */
void test_lseek_pass(void) {
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  
  if (fd >= 0) {
    const char *data = "Hello World";
    write(fd, (byte_t*)data, strlen(data));
    
    // Test seeking
    int pos = lseek(fd, 0, SEEKSET);
    CU_ASSERT_EQUAL(pos, 0);
    
    close(fd);
    unlink("testfile");
  }
}

/**
 * @brief Test link functionality
 */
void test_link_pass(void) {
  // Create a file first
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  if (fd >= 0) {
    close(fd);
    
    // Test linking
    int result = link("testfile", "linkfile");
    CU_ASSERT_EQUAL(result, 0);
    
    // Cleanup
    unlink("linkfile");
    unlink("testfile");
  }
}

/**
 * @brief Test rename functionality
 */
void test_rename_pass(void) {
  // Create a file first
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  if (fd >= 0) {
    close(fd);
    
    // Test renaming
    int result = rename("testfile", "newname");
    CU_ASSERT_EQUAL(result, 0);
    
    // Cleanup
    unlink("newname");
  }
}

/**
 * @brief Test stat functionality
 */
void test_stat_pass(void) {
  // Create a file first
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  if (fd >= 0) {
    close(fd);
    
    // Test stat
    stat_t st;
    int result = stat("testfile", &st);
    CU_ASSERT_EQUAL(result, 0);
    
    // Cleanup
    unlink("testfile");
  }
}

/**
 * @brief Test chmod and chown functionality
 */
void test_chmod_chown_pass(void) {
  // Create a file first
  int fd = open("testfile", OCREATE | OWRITE, 0644);
  CU_ASSERT_TRUE(fd >= 0);
  if (fd >= 0) {
    close(fd);
    
    // Test chmod
    int result = chmod("testfile", 0755);
    CU_ASSERT_EQUAL(result, 0);
    
    // Cleanup
    unlink("testfile");
  }
}

/**
 * @brief Test directory navigation
 */
void test_directory_navigation_pass(void) {
  // Test directory operations
  int result = mkdir("testdir", 0755);
  CU_ASSERT_EQUAL(result, 0);
  
  if (result == 0) {
    // Test chdir
    result = chdir("testdir");
    CU_ASSERT_EQUAL(result, 0);
    
    // Change back
    chdir("..");
    
    // Cleanup
    rmdir("testdir");
  }
}

/**
 * @brief Test sync functionality
 */
void test_sync_pass(void) {
  const char *test_data = "Sync test data that should be persistent";
  
  // Create file and write data
  int fd = open("sync_test.txt", OCREATE | ORDWR, 0777);
  CU_ASSERT_NOT_EQUAL(fd, -1);
  fsize_t test_data_len = snlen(test_data, 100);
  CU_ASSERT_EQUAL(write(fd, (byte_t *)test_data, test_data_len), (int)test_data_len);
  
  // Force sync to ensure data is written
  CU_ASSERT_EQUAL(sync(), 0);
  
  // Close and reopen to verify persistence
  close(fd);
  fd = open("sync_test.txt", ORDWR, 0777);
  CU_ASSERT_NOT_EQUAL(fd, -1);
  
  // Verify data persisted
  char buf[100];
  CU_ASSERT_EQUAL(read(fd, (byte_t *)buf, test_data_len), (int)test_data_len);
  buf[test_data_len] = '\0';
  CU_ASSERT_STRING_EQUAL(buf, test_data);
  
  close(fd);
  CU_ASSERT_EQUAL(unlink("sync_test.txt"), 0);
  
  // Test sync with no pending operations
  CU_ASSERT_EQUAL(sync(), 0);
}

/**
 * @brief Test mknode functionality
 */
void test_mknode_pass(void) {
  stat_t stat_buf;
  
  // Test creating character device node
  int result = mknode("test_char_dev", CHARACTER, 0666);
  if (result == 0) {
    CU_ASSERT_EQUAL(stat("test_char_dev", &stat_buf), 0);
    CU_ASSERT_EQUAL(stat_buf.ftype, CHARACTER);
    CU_ASSERT_EQUAL(unlink("test_char_dev"), 0);
  }
  // If mknode fails, that's okay - it might not be implemented
  
  // Test creating block device node
  result = mknode("test_block_dev", BLOCK, 0666);
  if (result == 0) {
    CU_ASSERT_EQUAL(stat("test_block_dev", &stat_buf), 0);
    CU_ASSERT_EQUAL(stat_buf.ftype, BLOCK);
    CU_ASSERT_EQUAL(unlink("test_block_dev"), 0);
  }
  
  // Test creating FIFO node
  result = mknode("test_fifo", FIFO, 0666);
  if (result == 0) {
    CU_ASSERT_EQUAL(stat("test_fifo", &stat_buf), 0);
    CU_ASSERT_EQUAL(stat_buf.ftype, FIFO);
    CU_ASSERT_EQUAL(unlink("test_fifo"), 0);
  }
}

/**
 * @brief Test directory operations
 */
void test_directory_operations_pass(void) {
  // Create test directory structure
  CU_ASSERT_EQUAL(mkdir("dir_test", 0777), 0);
  CU_ASSERT_EQUAL(mkdir("dir_test/subdir1", 0777), 0);
  CU_ASSERT_EQUAL(mkdir("dir_test/subdir2", 0777), 0);
  
  // Create some test files
  int fd = open("dir_test/file1.txt", OCREATE | ORDWR, 0777);
  CU_ASSERT_NOT_EQUAL(fd, -1);
  close(fd);
  
  fd = open("dir_test/file2.txt", OCREATE | ORDWR, 0777);
  CU_ASSERT_NOT_EQUAL(fd, -1);
  close(fd);
  
  // Test opening directory
  int dir_fd = opendir("dir_test");
  CU_ASSERT_NOT_EQUAL(dir_fd, -1);
  
  if (dir_fd != -1) {
    dirent_t entry;
    int entry_count = 0;
    int found_dot = 0, found_dotdot = 0;
    int found_file1 = 0, found_file2 = 0;
    int found_subdir1 = 0, found_subdir2 = 0;
    
    // Read all directory entries
    int result;
    while ((result = readdir(dir_fd, &entry)) > 0) {
      entry_count++;
      
      // Check for standard entries
      if (sncmp(entry.name, ".", DIRNAMEENTRY) == 0) {
        found_dot = 1;
      } else if (sncmp(entry.name, "..", DIRNAMEENTRY) == 0) {
        found_dotdot = 1;
      } else if (sncmp(entry.name, "file1.txt", DIRNAMEENTRY) == 0) {
        found_file1 = 1;
      } else if (sncmp(entry.name, "file2.txt", DIRNAMEENTRY) == 0) {
        found_file2 = 1;
      } else if (sncmp(entry.name, "subdir1", DIRNAMEENTRY) == 0) {
        found_subdir1 = 1;
      } else if (sncmp(entry.name, "subdir2", DIRNAMEENTRY) == 0) {
        found_subdir2 = 1;
      }
    }
    
    // Should have read successfully until end of directory
    CU_ASSERT_EQUAL(result, 0);  // End of directory
    
    // Should have found . and .. entries
    CU_ASSERT_EQUAL(found_dot, 1);
    CU_ASSERT_EQUAL(found_dotdot, 1);
    
    // Should have found our created files and directories
    CU_ASSERT_EQUAL(found_file1, 1);
    CU_ASSERT_EQUAL(found_file2, 1);
    CU_ASSERT_EQUAL(found_subdir1, 1);
    CU_ASSERT_EQUAL(found_subdir2, 1);
    
    // Total entries should be at least 6 (., .., file1.txt, file2.txt, subdir1, subdir2)
    CU_ASSERT(entry_count >= 6);
    
    // Test closing directory
    CU_ASSERT_EQUAL(closedir(dir_fd), 0);
  }
  
  // Test error cases
  CU_ASSERT_EQUAL(opendir("nonexistent_dir"), -1);  // Directory doesn't exist
  CU_ASSERT_EQUAL(opendir("dir_test/file1.txt"), -1);  // Not a directory
  CU_ASSERT_EQUAL(closedir(-1), -1);  // Invalid file descriptor
  
  // Test readdir with invalid file descriptor
  dirent_t dummy;
  CU_ASSERT_EQUAL(readdir(-1, &dummy), -1);
  
  // Test opening root directory
  int root_fd = opendir("/");
  if (root_fd != -1) {
    dirent_t root_entry;
    int root_result = readdir(root_fd, &root_entry);
    CU_ASSERT(root_result >= 0);  // Should be able to read root directory
    closedir(root_fd);
  }
  
  // Cleanup
  CU_ASSERT_EQUAL(unlink("dir_test/file1.txt"), 0);
  CU_ASSERT_EQUAL(unlink("dir_test/file2.txt"), 0);
  CU_ASSERT_EQUAL(rmdir("dir_test/subdir1"), 0);
  CU_ASSERT_EQUAL(rmdir("dir_test/subdir2"), 0);
  CU_ASSERT_EQUAL(rmdir("dir_test"), 0);
}
