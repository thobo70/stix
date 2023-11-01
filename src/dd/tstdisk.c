/**
 * @file tstdisk.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */



#include "utils.h"
#include "dd.h"


extern void tstdisk_strategy(ldevminor_t minor, bhead_t *bh);

bdev_t tstdisk = {
  NULL,
  NULL,
  tstdisk_strategy
};


void tstdisk_strategy(ldevminor_t minor, bhead_t *bh)
{
  ASSERT(minor < 1);
  ASSERT(bh);
}


