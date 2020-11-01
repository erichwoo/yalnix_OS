/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Process structures header file
 */

#ifndef __LINKED_LIST_H
#define __LINKED_LIST_H

#include "ykernel.h"

typedef struct node node_t;

struct node {
  void *data;
  int code; // if data is pcb, it might be the delay/return code/ . If data is a queue, then code is the identifier of pipe/cvar/lock
  node_t *next;
  node_t *prev;
};

typedef struct ll {
  int size;
  node_t *head;
  node_t *tail;
} ll_t;

ll_t* new_ll(void);
node_t* new_node(void *data);
void destroy_node(node_t *node);
int get_size(ll_t *list);
int is_empty(ll_t *list);
void enqueue(ll_t *list, node_t *n);
void insert_head(ll_t *list, node_t *n);
node_t* dequeue(ll_t *list);
node_t* pop(ll_t *list);
node_t* remove(ll_t *list, node_t *n);
int has_member(ll_t *list, node_t *n);
node_t* find(ll_t *list, int code);

#endif //__LINKED_LIST_H
