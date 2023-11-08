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

#define MAXFS 6           ///< maximum number of file systems
#define MAXOPENFILES 10   ///< maximum number of open files per process
#define MAXFILETAB 100    ///< maximum number of open files system wide
#define DIRNAMEENTRY 14   ///< maximum length of directory entry name
#define MAXPATH 256       ///< maximum length of path name

typedef enum omode {
  OREAD = 0x0001,       ///< open for reading
  OWRITE = 0x0002,      ///< open for writing
  ORDWR = 0x0003,       ///< open for reading and writing
  OEXEC = 0x0004,       ///< open for execution
  OTRUNC = 0x0010,      ///< truncate file
  OAPPEND = 0x0020,     ///< append to file
  OCREATE = 0x0040,     ///< create file if it does not exist
  OEXCL = 0x0080,       ///< exclusive use
  ONONBLOCK = 0x0100,   ///< non-blocking call
  OSYNC = 0x0200,       ///< synchronous write
} omode_t;

typedef struct dirent_t {
  ninode_t inum;
  char name[DIRNAMEENTRY];
} dirent_t;


typedef struct filetab_t {
  iinode_t *inode;
  word_t refs;
  fsize_t offset;
  int flags;
} filetab_t;



#endif
