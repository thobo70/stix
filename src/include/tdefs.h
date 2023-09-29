/**
 * @file tdefs.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief global type definitions
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _TDEFS_H
#define _TDEFS_H

typedef unsigned char byte_t;
typedef unsigned short word_t;
typedef unsigned long dword_t;

#define true (1==1)
#define false (!true)

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef byte_t ldev_t;
typedef dword_t block_t;
typedef word_t ninode_t;
typedef byte_t nref_t;

typedef byte_t owner_t;
typedef byte_t group_t;
typedef word_t fmode_t;
typedef dword_t time_t;
typedef word_t nlinks_t;
typedef dword_t fsize_t;

#endif
