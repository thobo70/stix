/**
 * @file dd.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-10-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "utils.h"
#include "dd.h"

extern bdev_t tstdisk;

bdev_t *bdevtable[] = {
  &tstdisk
};

extern cdev_t tstcon;

cdev_t *cdevtable[] = {
  &tstcon
};




void bdevopen(ldev_t ldev)
{
  ASSERT(ldev.major < NBDEVENTRIES);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->open(ldev.minor);
}

void bdevclose(ldev_t ldev)
{
  ASSERT(ldev.major < NBDEVENTRIES);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->close(ldev.minor);
}

void bdevread(ldev_t ldev, bhead_t *bh)
{
  ASSERT(ldev.major < NBDEVENTRIES);
  ASSERT(bh);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->strategy(ldev.minor, bh);
}


void bdevwrite(ldev_t ldev, bhead_t *bh)
{
  ASSERT(ldev.major < NBDEVENTRIES);
  ASSERT(bh);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->strategy(ldev.minor, bh);
}

