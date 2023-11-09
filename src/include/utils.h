/**
 * @file utils.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief utility functions
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _UTILS_H
#define _UTILS_H

#include "tdefs.h"

#ifndef NDEBUG
#define ASSERT(a) if (!(a)) panic("\n" #a " is 0/false in %s[Ln %d]\n", __FILE__, __LINE__)
#else
#define ASSERT(s)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b)) ///< minimum of a and b
#define MAX(a, b) ((a) > (b) ? (a) : (b)) ///< maximum of a and b 

void mcpy(void *dst, void *src, sizem_t n);
void mset(void *s, int c, sizem_t n);

int sncmp(const char *s1, const char *s2, sizem_t n);
void sncpy(char *dst, const char *src, sizem_t n);
void snapnd(char *dst, const char *src, sizem_t n);

sizem_t snlen(const char *s, sizem_t mlen);

void kprintf(const char *t, ...);
void panic(const char *t, ...);

#endif
