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

#include "dd.h"

bdev_t *bdevtable[] = {
  NULL,
  NULL
};

extern cdev_t tstcon;

cdev_t *cdevtable[] = {
  &tstcon
};




