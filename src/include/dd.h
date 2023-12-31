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
#include "clist.h"

#define BLOCKSIZE 512

typedef struct bdev_t {
  void (*init)(void);  
  void (*open)(ldevminor_t minor);
  void (*close)(ldevminor_t minor);
  void (*strategy)(ldevminor_t minor, bhead_t *bh);
} bdev_t;

typedef struct cdev_t {
  void (*init)(void);  
  void (*open)(ldevminor_t minor);
  void (*close)(ldevminor_t minor);
  void (*read)(ldevminor_t minor, byte_t cl);
  void (*write)(ldevminor_t minor, byte_t cl);
  void (*ioctl)(ldevminor_t minor, int cmd, void *arg);
} cdev_t;

void init_dd(void);

void bdevopen(ldev_t ldev);
void bdevclose(ldev_t ldev);
void bdevstrategy(ldev_t ldev, bhead_t *bh);

void cdevopen(ldev_t ldev);
void cdevclose(ldev_t ldev);
void cdevread(ldev_t ldev, byte_t cl);
void cdevwrite(ldev_t ldev, byte_t cl);
void cdevioctl(ldev_t ldev, int cmd, void *arg);

#endif
