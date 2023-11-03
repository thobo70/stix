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


process_t *active = NULL;

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
