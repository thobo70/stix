/**
 * @file clist.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief character list implementation
 * 
 * The clist data structure is a singly linked list of nodes. The nodes are
 * stored in a fixed size array. The clist data structure is also stored in a
 * fixed size array. The clist data structure contains the index of the head
 * and tail node of the list. The nodes contain the index of the next node.
 * The nodes also contain the data of the node.
 * 
 * The clist data structure also contains a size field. This field is used to
 * keep track of the number of characters in the list (all valid node.data characters).
 * The clist data structure also contains a locked field. This field is used to lock the
 * list. The list * is locked when a process is accessing the list. The list is unlocked
 * when the process is done accessing the list.
 * 
 * The clist data structure also contains a free list of nodes. The free list is a
 * singly linked list of nodes. The nodes in the free list are not used in any list.
 * The free list is used to allocate nodes for a list. The free list is also used to
 * free nodes from a list.
 * 
 * The clist data structure also contains a free list of clists. The free list is a
 * singly linked list of clists. The clists in the free list are not used in any list.
 * The free list is used to allocate clists for a list. The free list is also used to
 * free clists from a list.
 * 
 * The clist works as follows:
 * - The clist is initialized by calling init_clist().
 * - A new clist is created by calling clist_create().
 * - Data is added to the clist by calling clist_push().
 * - Data is removed from the clist by calling clist_pop().
 * - The clist is destroyed by calling clist_destroy().
 * 
 * 
 * 
 * @startuml
 * class clist{
 * +head: byte_t
 * +tail: byte_t
 * +size: sizem_t
 * +locked: byte_t
 * }
 * 
 * class node{
 * +next: byte_t
 * +prev: byte_t
 * +data: char[MAXNODEDATA]
 * }
 * 
 * clist "1" -- "*" node
 * 
 * @enduml
 * 
 * @startuml
 * class freenode {
 * freenode: byte_t
 * }
 * 
 * class nodeArry {
 * +nodes: node[MAXNODES]
 * }
 * 
 * class clistArry {
 * +clists: clist[MAXCLISTS]
 * }
 * 
 * class freeClist {
 * freeClist: byte_t
 * }
 * 
 * nodeArry o-- freeNode1
 * nodeArry o-- freeNode2
 * freeNode1 --> freeNode2
 * freenode --> freeNode1
 * nodeArry o-- headNode
 * nodeArry o-- tailNode
 * headNode --> tailNode
 * clistArry o-- clist1
 * clist1 --> headNode
 * clist1 --> tailNode
 * clistArry o-- freeClist1
 * clistArry o-- freeClist2
 * freeClist1 --> freeClist2
 * freeClist --> freeClist1
 * @enduml
 * 
 * @version 0.1
 * @date 2023-11-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/**
 * @brief clist data structure

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
    node->head += n;
    size -= n;
    data += n;
    list->size += n;
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
      list->size -= n;
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

