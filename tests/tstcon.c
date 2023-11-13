/**
 * @file tstcon.c
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
#include "clist.h"

void tstcon_read(ldevminor_t minor, clist_t *cl);
void tstcon_write(ldevminor_t minor, clist_t *cl);
void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg);

cdev_t tstcon = {
  NULL,
  NULL,
  tstcon_read,
  tstcon_write,
  tstcon_ioctl
};


void tstcon_read(ldevminor_t minor, clist_t *cl)
{
  ASSERT(minor < 1);
  ASSERT(cl);
}


void tstcon_write(ldevminor_t minor, clist_t *cl)
{
  ASSERT(minor < 1);
  ASSERT(cl);
}



void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg)
{
  ASSERT(minor < 1);
  ASSERT(cmd);
  ASSERT(arg);
}


