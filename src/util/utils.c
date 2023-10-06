
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

void panic(const char *t, ...)
{
  va_list args;

  va_start(args, t);
  vprintf(t, args);
  va_end(args);

  abort();
}