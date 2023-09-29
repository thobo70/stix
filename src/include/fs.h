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

#include "inode.h"

#define MAXFS 6

/// @brief file
typedef struct fsfile_t {
  iinode_t *inode;
} fsfile_t;

/// @brief user file descriptor
typedef struct fsufdescr_t {
  fsfile_t *file;
} fsufdescr_t;


#endif
