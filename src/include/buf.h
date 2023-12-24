/**
 * @file buf.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief buffer handling
 * @version 0.1
 * @date 2023-09-29
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _BUF_H
#define _BUF_H

#include "tdefs.h"

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
  byte_t written : 1;
  byte_t infreelist : 1;
  byte_t error : 1;
  ldev_t dev;
  block_t block;
} _STRUCTATTR_ bhead_t;


/**
 * @brief initialize file system buffers
 * 
 */
void init_buffers(void);
void check_bfreelist(void);



/**
 * @brief add block to free list
 * 
 * @param b 
 * @param asFirst 
 */
void add_buf_to_freelist(bhead_t *b, int asFirst);



/**
 * @brief get block from free list
 * 
 * @return bhead_t*   pointer to block header
 */
bhead_t *getblk(ldev_t dev, block_t block);



/**
 * @brief sync all buffers to disk 
 * 
 * @param async   if true, sync buffers asynchronously
 */
void syncall_buffers(int async);



/**
 * @brief read block from device dev
 * 
 * @param dev       device to read from
 * @param block     block to read
 * @return bhead_t* pointer to block header read from disk
 */
bhead_t *bread(ldev_t dev, block_t block);

/**
 * @brief reads bl1 and pre loads bl2
 * 
 * @param dev 
 * @param bl1 
 * @param bl2 
 * @return bhead_t*   buffer for bl1
 */
bhead_t *breada(ldev_t dev, block_t bl1, block_t bl2);

/**
 * @brief writes buffer to disk
 * 
 * @param b 
 */
void bwrite(bhead_t *b);

/**
 * @brief release buffer 
 * 
 * @param b buffer to release
 */
void brelse(bhead_t *b);

/**
 * @brief is called when buffer is synced
 * 
 * @param b 
 * @param error 
 */
void buffer_synced(bhead_t *b, int error);

#endif
