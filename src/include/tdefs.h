/**
 * @file tdefs.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief global type definitions
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _TDEFS_H
#define _TDEFS_H

#define _STRUCTATTR_ __attribute__((packed))

typedef unsigned char byte_t;
typedef unsigned short word_t;
typedef unsigned int dword_t;

typedef dword_t sizem_t;

#define true (1==1)
#define false (!true)

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef word_t block_t;     ///< block number on disk (sector)
typedef word_t ninode_t;    ///< index of inode in filesystem
typedef byte_t nref_t;      ///< number of (file) references to the inode

typedef word_t user_t;      ///< user id
typedef word_t group_t;     ///< group id
typedef word_t fmode_t;     ///< file mode (e.g. rwx)
typedef dword_t utime_t;    ///< system time
typedef word_t nlinks_t;    ///< number of links (directory entries) to the inode
typedef dword_t fsize_t;    ///< size of file
typedef byte_t fsnum_t;     ///< index of filesystem

typedef byte_t ldevmajor_t;      ///< logical major device
typedef byte_t ldevminor_t;      ///< logical minor device

typedef union ldev_t {
  struct {
    ldevmajor_t major;
    ldevminor_t minor;
  } _STRUCTATTR_;
  word_t ldev;
} ldev_t;


#endif
