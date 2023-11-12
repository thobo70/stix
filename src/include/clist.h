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
 * @brief clist data structure
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
 * class node{
 * +next: byte_t
 * +prev: byte_t
 * +data: char[MAXNODEDATA]
 * }
 * 
 * class clist{
 * +head: byte_t
 * +tail: byte_t
 * +size: sizem_t
 * +locked: byte_t
 * }
 * 
 * node "*" -- "1" clist
 * 
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
 */

typedef struct clist_node_t {
    byte_t next;    ///< index of next node
    byte_t tail;    ///< index of tail data
    byte_t head;    ///< index of head data
    char data[MAXNODEDATA]; ///< node data
} clist_node_t;     ///< node type

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
