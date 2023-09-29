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

#include "inode.h"
#include "buf.h"

/**
 * @brief main of mockup
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
  init_buffers();
  init_inodes();
  
  return 0;
}