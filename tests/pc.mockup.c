/**
 * @file pc.mockup.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "pc.h"
#include "utils.h"

u_t u1 = {
  .fsroot = NULL,
  .workdir = NULL
};

process_t p1 = {
  .pid = 1,
  .isswapped = false,
  .iswaitingfor = RUNHIGH,
  .queue = NULL,
  .u = &u1
};

process_t *active = &p1;

waitfor_t wokenup = 0;

void waitfor(waitfor_t w)
{
  ASSERT(w < NQUEUES);
  ASSERT(w != wokenup);
}


void wakeall(waitfor_t w)
{
  ASSERT(w < NQUEUES);
  wokenup = w;
}
