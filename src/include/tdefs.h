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

typedef unsigned char byte_t;
typedef unsigned short word_t;
typedef unsigned long dword_t;

#define true (1==1)
#define false (!true)

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef byte_t ldev_t;      ///< logical device
typedef dword_t block_t;    ///< block number on disk (sector)
typedef word_t ninode_t;    ///< index of inode in filesystem
typedef byte_t nref_t;      ///< number of (file) references to the inode

typedef byte_t owner_t;
typedef byte_t group_t;
typedef word_t fmode_t;     ///< file mode (e.g. rwx)
typedef dword_t time_t;     ///< system time
typedef word_t nlinks_t;    ///< number of links (directory entries) to the inode
typedef dword_t fsize_t;    ///< size of file
typedef byte_t fsnum_t;     ///< index of filesystem

#endif
