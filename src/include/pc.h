/**
 * @file pc.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief process control
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef _PC_H
#define _PC_H

#include "tdefs.h"
#include "inode.h"
#include "fs.h"

enum waitfor {
  RUNHIGH,
  RUNMID,
  RUNLOW,
  SBLOCKBUSY,
  BLOCKBUSY,
  NOFREEBLOCKS,
  BLOCKREAD,
  BLOCKWRITE,
  INODELOCKED,
  SWAPIN,
  SWAPOUT,
  NQUEUES
};

typedef enum waitfor waitfor_t;

/*
typedef enum errno_t {
  EPERM = -39,
  ENOENT,
  ESRCH,
  EINTR,
  EIO,
  ENXIO,
  E2BIG,
  ENOEXEC,
  EBADF,
  ECHILD,
  EAGAIN,
  ENOMEM,
  EACCES,
  EFAULT,
  ENOTBLK,
  EBUSY,
  EEXIST,
  EXDEV,
  ENODEV,
  ENOTDIR,
  EISDIR,
  EINVAL,
  ENFILE,
  EMFILE,
  ENOTTY,
  ETXTBSY,
  EFBIG,
  ENOSPC,
  ESPIPE,
  EROFS,
  EMLINK,
  EPIPE,
  EDOM,
  ERANGE,
  EDEADLK,
  ENAMETOOLONG,
  ENOLCK,
  ENOSYS,
  ENOTEMPTY,
  NOERROR = 0,
} errno_t;
*/
typedef int errno_t;

typedef struct fdesctab_t {
  filetab_t *ftabent;
  omode_t omode;
} fdesctab_t;

typedef struct userspace_t {
  iinode_t *fsroot;
  iinode_t *workdir;
  fdesctab_t fdesc[MAXOPENFILES];
  errno_t err;
} u_t;


typedef struct process_t {
  int pid;
  byte_t isswapped : 1;
  waitfor_t iswaitingfor;
  struct process_t *queue;
  u_t *u;
} process_t;

extern process_t *active;

/**
 * @brief waitfor puts process to sleep till reason is no longer valid
 * 
 * @param w   reason to wait
 */
void waitfor(waitfor_t w);

/**
 * @brief wakes all processes waiting till reason is no longer valid
 * 
 * @param w   reason waited for
 */
void wakeall(waitfor_t w);

#endif
