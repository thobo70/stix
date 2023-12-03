/**
 * @file main.c
 * @author Thomas Boos
 * @brief main for mockup
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#include "dd.h"
#include "inode.h"
#include "buf.h"
#include "fs.h"
#include "clist.h"

bdev_t *bdevtable[] = {
  NULL
};


cdev_t *cdevtable[] = {
  NULL
};

/**
 * @brief main of mockup
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(void)
{
  init_clist();
  init_dd();
  init_buffers();
  init_inodes();
  init_fs(); 
  
  
  return 0;
}
