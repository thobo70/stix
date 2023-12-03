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



extern bdev_t *bdevtable[];
extern cdev_t *cdevtable[];
int nbdeventries = 0;
int ncdeventries = 0;



/**
 * @brief initialize device drivers
 * 
 */
void init_dd(void)
{
  int i;

  for (i = 0; bdevtable[i] ; i++)
    if (bdevtable[i]->init)
      bdevtable[i]->init();
  nbdeventries = i;

  for (i = 0; cdevtable[i] ; i++)
    if (cdevtable[i]->init)
      cdevtable[i]->init();
  ncdeventries = i;
}


void bdevopen(ldev_t ldev)
{
  ASSERT(ldev.major < nbdeventries);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->open(ldev.minor);
}


void bdevclose(ldev_t ldev)
{
  ASSERT(ldev.major < nbdeventries);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->close(ldev.minor);
}


void bdevstrategy(ldev_t ldev, bhead_t *bh)
{
  ASSERT(ldev.major < nbdeventries);
  ASSERT(bh);
  ASSERT(bdevtable[ldev.major]);

  bdevtable[ldev.major]->strategy(ldev.minor, bh);
}




void cdevopen(ldev_t ldev)
{
  ASSERT(ldev.major < ncdeventries);
  if (!cdevtable[ldev.major])
    return;

  cdevtable[ldev.major]->open(ldev.minor);
}


void cdevclose(ldev_t ldev)
{
  ASSERT(ldev.major < ncdeventries);
  if (!cdevtable[ldev.major])
    return;

  cdevtable[ldev.major]->close(ldev.minor);
}


void cdevread(ldev_t ldev, byte_t cl)
{
  ASSERT(ldev.major < ncdeventries);
  ASSERT(cdevtable[ldev.major]);

  cdevtable[ldev.major]->read(ldev.minor, cl);
}


void cdevwrite(ldev_t ldev, byte_t cl)
{
  ASSERT(ldev.major < ncdeventries);
  ASSERT(cdevtable[ldev.major]);

  cdevtable[ldev.major]->write(ldev.minor, cl);
}


void cdevioctl(ldev_t ldev, int cmd, void *arg)
{
  ASSERT(ldev.major < ncdeventries);
  ASSERT(cdevtable[ldev.major]);
  ASSERT(arg);

  cdevtable[ldev.major]->ioctl(ldev.minor, cmd, arg);
}
