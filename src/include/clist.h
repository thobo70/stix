/**
 * @file clist.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief character list header
 * @version 0.1
 * @date 2023-11-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */


#ifndef CLIST_H
#define CLIST_H

#include <tdefs.h>

#define MAXNODEDATA 16  ///< max size of node data


/**
 * @brief clist node data structure
 * 
 */
typedef struct clist_node_t {
    byte_t next;    ///< index of next node
    byte_t tail;    ///< index of tail data
    byte_t head;    ///< index of head data
    char data[MAXNODEDATA]; ///< node data
} clist_node_t;     ///< node type

/**
 * @brief clist data structure
 * 
 */
typedef struct clist_t {
    byte_t head;    ///< index of head node
    byte_t tail;    ///< index of tail node
    sizem_t size;   ///< size of list
    byte_t locked : 1;  ///< list locked
} clist_t;          ///< clist type

// Function prototypes
void init_clist(void);
byte_t clist_create(void);
void clist_destroy(byte_t clisti);
int clist_size(byte_t clisti);
int clist_push(byte_t clisti, char *data, sizem_t size);
int clist_pop(byte_t clisti, char *data, sizem_t size);

#endif /* CLIST_H */
