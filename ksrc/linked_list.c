/* Erich Woo & Boxian Wang
 * 23 October 2020
 * process manipulation
 */

#include "linked_list.h"

/******** doubly linked list ****/

ll_t* new_ll(void) {
  ll_t* new = malloc(sizeof(ll_t));
  new->head = new->tail = NULL;
  new->size = 0;
  return new;
}

node_t* new_node(void *data) {
  node_t* new = malloc(sizeof(node_t));
  new->data = data;
  new->next = new->prev = NULL;
  return new;
}

void destroy_node(node_t *node) {
  free(node->data);
  free(node);
}

int is_empty(ll_t *list) {
  return (list->size == 0);
}

int get_size(ll_t *list) {
  return list->size;
}

void enqueue(ll_t *list, node_t *n) {
  if (list->head == NULL) {
    list->head = list->tail = n;
    n->next = n->prev = NULL;
  } else {
    n->next = NULL;
    n->prev = list->tail;
    list->tail->next = n;
    list->tail = n;
  }
  list->size++;
}

void insert_head(ll_t *list, node_t *n) {
  if (list->head == NULL) {
    list->head = list->tail = n;
    n->next = n->prev = NULL;
  } else {
    n->next = list->head;
    n->prev = NULL;
    list->head->prev = n;
    list->head = n;
  }
  list->size++;
}

// list nonempty
node_t* pop(ll_t *list) {
  if (list->size == 0) {
    TracePrintf(1, "Popping empty list\n");
    return NULL;
  }
  node_t *t = list->tail;
  if (list->head == list->tail) {
    list->head = list->tail = NULL;
  } else {
    list->tail = list->tail->prev;
    list->tail->next = NULL;
  }
  list->size--;
  t->next = t->prev = NULL;
  return t;
}

node_t* dequeue(ll_t *list) {
  if (list->size == 0) {
    //TracePrintf(1, "Dequeueing empty list\n");
    return NULL;
  }
  node_t *h = list->head;
  if (list->head == list->tail) {
    list->head = list->tail = NULL;
  } else {
    list->head = list->head->next;
    list->head->prev = NULL;
  }
  list->size--;
  h->next = h->prev = NULL;
  return h;
}
// n must be in list
node_t* remove(ll_t *list, node_t *n) {
  if (list->size == 0) {
    TracePrintf(1, "Removing from empty list\n");
    return NULL;
  }
  if (list->head == n) dequeue(list);
  else if (list->tail == n) pop(list);
  else {
    n->next->prev = n->prev;
    n->prev->next = n->next;
    n->next = n->prev = NULL;
    list->size--;
  }
  return n;
}

int has_member(ll_t *list, node_t *n) {
  node_t *curr;
  for (curr = list->head; curr != NULL; curr = curr->next) if (curr == n) return 1;
  return 0;
}

node_t* find(ll_t *list, int code) { // return NULL if not found
  node_t *curr;
  for (curr = list->head; curr != NULL && curr->code != code; curr = curr->next);
  return curr;
}
