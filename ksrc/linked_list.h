/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for linked_list.c
 */

#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H

#include "ykernel.h"

typedef struct node node_t;

struct node {
  void *data; // arbitrary payload
  int code; // if data is pcb, it might be the delay/return code/ . If data is a queue, then code is the identifier of pipe/cvar/lock
  node_t *next;
  node_t *prev;
};

// a doubly-linked-list
typedef struct ll {
  int size;
  node_t *head;
  node_t *tail;
} ll_t;

/* Initialize and return a blank ll_t*
 * Must be free'd later by caller
 *
 * @return a blank ll_t*
 */
ll_t* new_ll(void);

/* Iniatializes and returns a new node_t* 
 * with the given void* data payload
 * Must be free'd later by caller
 *
 * @param data the ptr to data payload
 * @return a new node pointer with specified data
 */
node_t* new_node(void *data);

/* frees the specified node and its data. 
 * Does nothing if node* is already null
 *
 * @param the node pointer to free
 */
void destroy_node(node_t *node);

/* Gets the size of the specified ll
 *
 * @param list the specified ll pointer
 * @return the size of the ll, -1 if NULL list/error
 */
int get_size(ll_t *list);

/* Returns whether (1) or not (0) the specified ll is empty
 * 
 * @param list the specified ll pointer
 * @return 1 if empty, 0 if non-empty, -1 if NULL list/error
 */
int is_empty(ll_t *list);

/* Enqueues the specified node onto the specified ll
 * Does nothing if either param is NULL
 *
 * @param list the specified ll pointer
 * @param n the node pointer to enqueue
 */
void enqueue(ll_t *list, node_t *n);

/* Inserts the specified node to the head of the specified ll
 * Does nothing if either param is NULL
 *
 * @param list the specified ll pointer
 * @param n the node pointer to push
 */
void push(ll_t *list, node_t *n);

/* Dequeues and returns the head of the specified list
 * 
 * @param list the spceified ll pointer
 * @return the dequeued node pointer, or NULL if empty/NULL list
 */
node_t* dequeue(ll_t *list);

/* Pops and returns the tail from specified list
 *
 * @param list the specified ll pointer
 * @return the popped node pointer, or NULL if empty/NULL list
 */ 
node_t* pop(ll_t *list);

/* Removes and returns the specified node from the specified list
 * Assumes the specified node is in the specified list
 *
 * @param list the specified ll pointer
 * @return the removed node pointer, or NULL if empty/NULL list
 */
node_t* remove(ll_t *list, node_t *n);

/* Returns whether (1) or not (0) the specified list contains the specified node
 *
 * @param list the spcified ll pointer
 * @return 1 if the ll has n, 0 if not (or if NULL list/n)
 */
int has_member(ll_t *list, node_t *n);

/* Finds and returns the node with specified code in the specified list
 *
 * @param list the specified ll pointer
 * @param code the node's code to match/look for
 * @return the found node, or NULL if not found, NULL list
 */
node_t* find(ll_t *list, int code);

#endif //__LINKED_LIST_H
