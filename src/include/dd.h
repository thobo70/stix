/**
 * @file dd.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-10-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef _DD_H
#define _DD_H

#include "tdefs.h"
#include "buf.h"

#define BLOCKSIZE 512

#define NBDEVENTRIES (sizeof(bdevtable) / sizeof(bdev_t*))

typedef struct bdev_t {
  void (*open)(ldevminor_t minor);
  void (*close)(ldevminor_t minor);
  void (*strategy)(ldevminor_t minor, bhead_t *bh);
} bdev_t;

#define NCDEVENTRIES (sizeof(cdevtable) / sizeof(cdev_t*))

typedef struct cdev_t {
  void (*open)(ldevminor_t minor);
  void (*close)(ldevminor_t minor);
  void (*read)(ldevminor_t minor, bhead_t *bh);
  void (*write)(ldevminor_t minor, bhead_t *bh);
  void (*ioctl)(ldevminor_t minor, bhead_t *bh);
} cdev_t;

#endif
