#ifndef _BUF_H
#define _BUF_H

#include "tdefs.h"

#include "dd.h"

#define BLOCKSIZE 512


#define NBUFFER 32

typedef struct buffer_t {
  byte_t mem[BLOCKSIZE];
} buffer_t;

typedef struct bhead_t {
  struct bhead_t *hprev;
  struct bhead_t *hnext;
  struct bhead_t *fprev;
  struct bhead_t *fnext;
  buffer_t *buf;
  byte_t busy : 1;
  byte_t dwrite : 1;
  byte_t valid : 1;
  byte_t infreelist : 1;
  ldev_t dev;
  block_t block;
} bhead_t;


/**
 * @brief initialize file system buffers
 * 
 */
void init_buffers(void);


/**
 * @brief add block to free at top
 * 
 * @param b 
 */
void add_free_first(bhead_t *b);

/**
 * @brief add block to free list at bottom
 * 
 * @param b 
 */
void add_free_last(bhead_t *b);


#endif
