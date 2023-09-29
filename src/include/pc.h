/**
 * @file pc.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief process control
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef _PC_H
#define _PC_H

#include "tdefs.h"

enum waitfor {
  RUNHIGH,
  RUNMID,
  RUNLOW,
  BLOCKBUSY,
  NOFREEBLOCKS,
  BLOCKREAD,
  INODELOCKED,
  SWAPIN,
  SWAPOUT,
  NQUEUES
};

typedef enum waitfor waitfor_t;

typedef struct process_t {
  int pid;
  byte_t isswapped : 1;
  waitfor_t iswaitingfor;
  struct process_t *queue;
} process_t;

/**
 * @brief waitfor puts process to sleep till reason is no longer valid
 * 
 * @param w   reason to wait
 */
void waitfor(waitfor_t w);

/**
 * @brief wakes all processes waiting till reason is no longer valid
 * 
 * @param w   reason waited for
 */
void wakeall(waitfor_t w);

#endif