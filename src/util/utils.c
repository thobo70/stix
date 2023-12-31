/**
 * @file utils.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-10-30
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

void mcpy(void *dst, void *src, sizem_t n)
{
  memcpy(dst, src, n);
}

void mset(void *s, int c, sizem_t n)
{
  memset(s, c, n);
}

int sncmp(const char *s1, const char *s2, sizem_t n)
{
  return strncmp(s1, s2, n);
}

void sncpy(char *dst, const char *src, sizem_t n)
{
  strncpy(dst, src, n);
}

void snapnd(char *dst, const char *src, sizem_t n)
{
  sizem_t i = 0;
  while (i < n && dst[i])
    ++i;
  sncpy(dst + i, src, n - i);
}

/**
 * @brief returns length of string s but maximum of mlen
 * 
 * @param s 
 * @param mlen 
 * @return sizem_t 
 */
sizem_t snlen(const char *s, sizem_t mlen)
{
  sizem_t i = 0;

  for ( ; i < mlen ; ++i )
    if (!s[i])
      break;

  return i;
}


void kprintf(const char *t, ...)
{
  va_list args;

  va_start(args, t);
  vprintf(t, args);
  va_end(args);
}



void panic(const char *t, ...)
{
  va_list args;

  va_start(args, t);
  vprintf(t, args);
  va_end(args);

  abort();
}
