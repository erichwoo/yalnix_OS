/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Everything related to process scheduling. See scheduling.h for detailed documentation
 */

#include "scheduling.h"

// THE process table
extern proc_table_t *procs;

// THE idle process node
extern node_t *idle_node;

proc_table_t *proc_table_init(void) {
  proc_table_t *p = malloc(sizeof(proc_table_t));
  p->running = NULL;
  p->waiting = new_ll();
  p->ready = new_ll();
  p->delayed = new_ll();
  p->orphans = new_ll();
  return p;
}

void ready(node_t *proc) {
  enqueue(procs->ready, proc);
}

void run_next(node_t *next) { 
  TracePrintf(1, "next id: %d\n", get_pid(next));
  node_t *curr = procs->running;
  procs->running = next;
  switch_proc(curr, next);
}

void run_next_ready(void) {
  node_t *next;
  if (is_empty(procs->ready)) next = idle_node;
  else next = dequeue(procs->ready);
  run_next(next);
}

void preempt(node_t* next) {
  if (procs->running != idle_node) ready(procs->running);
  run_next(next);
}

void rr_preempt(void) {
  if (is_empty(procs->ready)) return;
  if (procs->running != idle_node) ready(procs->running);
  run_next_ready();
}

void unblock(ll_t* blocked, node_t *proc) {
  if (is_empty(blocked)) return; // safety
  ready(remove(blocked, proc));
}

void unblock_head(ll_t *blocked) {
  unblock(blocked, blocked->head);
}

void unblock_all(ll_t *blocked) {
  while (!is_empty(blocked))
    ready(dequeue(blocked));
}

void block(ll_t* block_list) { 
  enqueue(block_list, procs->running);
  run_next_ready();
}

void h_block(ll_t* block_list) {
  push(block_list, procs->running);
  run_next_ready();
}

void block_wait(void) { 
  block(procs->waiting);
}

void check_wait(node_t* parent) {
  node_t *curr;
  if (has_member(procs->waiting, parent)) unblock(procs->waiting, parent);
}

void block_delay(int delay) { 
  procs->running->code = delay;
  block(procs->delayed);
}

void check_delay(void) {
  node_t *curr;
  for (curr = procs->delayed->head; curr != NULL; curr = curr->next) {
    if (--(curr->code) == 0) unblock(procs->delayed, curr);
  }
}

void graveyard(void) { 
  node_t* parent = get_parent(procs->running);
  if (parent != NULL) { // put onto parent's defunct children queue
    enqueue(((pcb_t*) parent->data)->d_children, procs->running);
    check_wait(parent); // unblock if parent was waiting                                           
    parent = NULL; // NULL parent for future errant access
  }
  else // add to proc table's orphan list to be reaped
    enqueue(procs->orphans, procs->running);    
  run_next_ready();
}

void defunct_blocked(ll_t* blocked, node_t *proc) { 
  remove(blocked, proc);
  node_t* parent = get_parent(proc);
  if (parent != NULL) {// put onto parent's defunct children queue
    enqueue(((pcb_t*) parent->data)->d_children, proc);
    check_wait(parent);
    parent = NULL;
  }
  else // add to proc table's orphan list to be reaped
    enqueue(procs->orphans, proc);
}

void reap_orphans(void) { 
  // remove and destroy all orphans
  while (!is_empty(procs->orphans))
    process_destroy(dequeue(procs->orphans));
}
