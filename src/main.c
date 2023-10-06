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

#include <stdio.h>

#include "inode.h"
#include "buf.h"

/**
 * @brief main of mockup
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(void)
{
  init_buffers();
  init_inodes();
  
  printf(" %ld %ld %ld %ld\n", sizeof(char), sizeof(short), sizeof(int), sizeof(ftype_t));
  printf("dinode: %ld\n", sizeof(dinode_t));
  
  return 0;
}
