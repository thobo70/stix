/**
 * @file fs.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _FS_H
#define _FS_H

#include "tdefs.h"
#include "inode.h"

#define MAXFS 6
#define DIRNAMEENTRY 14

typedef struct dirent_t {
  ninode_t inum;
  char name[DIRNAMEENTRY];
} dirent_t;

/// @brief file
typedef struct fsfile_t {
  iinode_t *inode;
} fsfile_t;

/// @brief user file descriptor
typedef struct fsufdescr_t {
  fsfile_t *file;
} fsufdescr_t;


#endif
