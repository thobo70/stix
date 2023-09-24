#ifndef _FS_H
#define _FS_H

#include "inode.h"

/// @brief file
typedef struct fsfile_t {
  iinode_t *inode;
} fsfile_t;

/// @brief user file descriptor
typedef struct fsufdescr_t {
  fsfile_t *file;
} fsufdescr_t;


#endif
