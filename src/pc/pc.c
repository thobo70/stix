/**
 * @file pc.c
 * @author Thomas Boos
 * @brief process control
 * @version 0.1
 * @date 2023-09-23
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "pc.h"
#include "utils.h"



process_t *waitforq[NQUEUES];

process_t *active = NULL;

void waitfor(waitfor_t w)
{
  ASSERT(w < NQUEUES);
}


void wakeall(waitfor_t w)
{
  ASSERT(w < NQUEUES);
}
