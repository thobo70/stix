
#ifndef CLIST_H
#define CLIST_H

#include <tdefs.h>

#define MAXNODEDATA 16  ///< max size of node data

// Define the node structure for the circular linked list
typedef struct clist_node_t {
    byte_t next;
    byte_t tail;
    byte_t head;
    char data[MAXNODEDATA];
} clist_node_t;

typedef struct clist_t {
    byte_t head;
    byte_t tail;
    sizem_t size;
    byte_t locked : 1;
} clist_t;

// Function prototypes
void init_clist(void);
byte_t clist_create(void);
void clist_destroy(byte_t clisti);
int clist_size(byte_t clisti);
int clist_push(byte_t clisti, char *data, sizem_t size);
int clist_pop(byte_t clisti, char *data, sizem_t size);

#endif /* CLIST_H */
