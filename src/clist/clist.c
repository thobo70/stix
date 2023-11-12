/**
 * @file clist.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief character list implementation
 * @version 0.1
 * @date 2023-11-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "clist.h"
#include "utils.h"

#define MAXNODES 100    ///< max number of nodes
#define MAXCLISTS 20    ///< max number of clists

clist_node_t nodes[MAXNODES];
byte_t freenode = 0;
clist_t clists[MAXCLISTS];
byte_t freeclist = 0;


/**
 * @brief initialize clist
 * 
 */
void init_clist(void)
{
  mset(nodes, 0, sizeof(nodes));
  mset(clists, 0, sizeof(clists));
  freenode = 1;
  for (int i = 0; i < MAXNODES - 1; i++)
    nodes[i].next = i + 2;
  nodes[MAXNODES - 1].next = 0;
  freeclist = 1;
  for (int i = 0; i < MAXCLISTS - 1; i++)
    clists[i].head = i + 2;
  clists[MAXCLISTS - 1].head = 0;
}


/**
 * @brief create a new clist
 * 
 * @return byte_t   clist index
 */
byte_t clist_create(void)
{
  byte_t i = freeclist;

  if (i == 0)
    return 0;
  clist_t *clist = &clists[i - 1];
  freeclist = clist->head;
  clist->head = 0;
  clist->tail = 0;
  clist->size = 0;
  return i;
}



/**
 * @brief destroy a clist
 * 
 * @param clisti  clist index
 */
void clist_destroy(byte_t clisti)
{
  ASSERT(clisti <= MAXCLISTS);
  clist_t *list = &clists[clisti - 1];
  ASSERT(list->head <= MAXNODES);
  ASSERT(list->tail <= MAXNODES);
  ASSERT((list->head == 0) ? (list->tail == 0) : (list->tail != 0));
  if (list->head != 0) {  // free all referenced nodes
    nodes[list->tail - 1].next = freenode;
    freenode = list->head;
  }
  list->head = freeclist;
  freeclist = clisti;
}



/**
 * @brief size of clist
 * 
 * @param clisti  clist index
 * @return int    size of clist
 */
int clist_size(byte_t clisti) {
  ASSERT(clisti <= MAXCLISTS);
  clist_t *list = &clists[clisti - 1];
  ASSERT(list->head <= MAXNODES);
  ASSERT(list->tail <= MAXNODES);
  ASSERT((list->head == 0) ? (list->tail == 0) : (list->tail != 0));
  return list->size;
}



/**
 * @brief get a new node
 * 
 * @return byte_t   node index
 */
byte_t getnode(void)
{
  byte_t i = freenode;

  if (i == 0)
    return 0;
  clist_node_t *node = &nodes[i - 1];
  freenode = node->next;
  node->next = 0;
  node->tail = 0;
  node->head = 0;
  return i;
}



/**
 * @brief put a node back
 * 
 * @param nodei   node index
 */
void putnode(byte_t nodei)
{
  ASSERT(nodei <= MAXNODES);
  clist_node_t *node = &nodes[nodei - 1];
  ASSERT(node->next <= MAXNODES);
  ASSERT(node->tail <= node->head);
  ASSERT(node->head <= MAXNODEDATA);
  ASSERT((node->head == 0) ? (node->tail == 0) : (node->tail != 0));
  node->next = freenode;
  freenode = nodei;
}



/**
 * @brief push data to clist
 * 
 * @param clisti  clist index
 * @param data    data to push
 * @param size    size of data
 * @return int    0 on success, -1 on error
 */
int clist_push(byte_t clisti, char *data, sizem_t size)
{
  ASSERT(clisti <= MAXCLISTS);
  ASSERT(data);
  ASSERT(size > 0);
  clist_t *list = &clists[clisti - 1];
  ASSERT(list->head <= MAXNODES);
  ASSERT(list->tail <= MAXNODES);
  ASSERT((list->head == 0) ? (list->tail == 0) : (list->tail != 0));
  if (list->head == 0) {
    byte_t i = getnode();
    if (i == 0)
      return -1;
    list->head = list->tail = i;
  }
  while (size > 0)
  {
    clist_node_t *node = &nodes[list->head - 1];
    ASSERT(node->next <= MAXNODES);
    ASSERT(node->tail <= node->head);
    ASSERT(node->head <= MAXNODEDATA);
    if (node->head == MAXNODEDATA) {
      byte_t i = getnode();
      if (i == 0)
        return -1;
      node->next = i;
      node = &nodes[i - 1];
      list->head = i;
    }
    sizem_t n = MIN(size, MAXNODEDATA - (sizem_t)node->head);
    mcpy(node->data + node->head, data, n);
    node->next += n;
    size -= n;
    data += n;
  }
  return 0;
}



/**
 * @brief pop data from clist
 * 
 * @param clisti  clist index
 * @param data    data to pop
 * @param size    size of data
 * @return int    0 on success, -1 on error
 */
int clist_pop(byte_t clisti, char *data, sizem_t size)
{
  ASSERT(clisti <= MAXCLISTS);
  ASSERT(data);
  ASSERT(size > 0);
  clist_t *list = &clists[clisti - 1];
  ASSERT(list->head <= MAXNODES);
  ASSERT(list->tail <= MAXNODES);
  ASSERT((list->head == 0) ? (list->tail == 0) : (list->tail != 0));
  do 
  {
    if (list->tail == 0)
      return -1;
    clist_node_t *node = &nodes[list->tail - 1];
    ASSERT(node->next <= MAXNODES);
    ASSERT(node->tail <= node->head);
    ASSERT(node->head <= MAXNODEDATA);
    sizem_t n = MIN(size, (sizem_t)(node->head - node->tail));
    if (n > 0) {
      mcpy(data, node->data + node->tail, n);
      node->tail += n;
      size -= n;
      data += n;
    }
    if (node->tail == node->head) {
      byte_t i = node->next;
      putnode(list->tail);
      if (i == 0) {
        list->head = list->tail = 0;
      } else
        list->tail = i;
    }
  } while (size > 0);
  return 0;
}

