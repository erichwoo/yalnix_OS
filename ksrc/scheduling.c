/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Everything related to process scheduling. See scheduling.h for detailed documentation
 */

#include "scheduling.h"

proc_table_t *proc_table_init(void) {
  proc_table_t *p = malloc(sizeof(proc_table_t));
  p->running = NULL;
  p->waiting = new_ll();
  p->ready = new_ll();
  p->delayed = new_ll();
  //p->defunct = new_ll();
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
  insert_head(block_list, procs->running);
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
  //enqueue(procs->defunct, procs->running); // even if is an orphan, must move to graveyard before destroying it externally
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

void defunct_blocked(ll_t* blocked, node_t *proc) { // for some reason if we have to kill a blocked process
  //enqueue(procs->defunct, remove(blocked, proc));
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

/*
node_t* reap_children(ll_t* d_children) { // search graveyard for children; return NULL if not found; dont remove children, just orphane it.
  node_t *curr;
  for (curr = procs->defunct->head; curr != NULL; curr = curr->next) {
    if (has_member(children, curr)) {
      ((pcb_t *)curr->data)->parent = NULL; // orphane, will be destroyed next
      return curr;
    }
  }
  return NULL;
}
*/
void reap_orphans(void) { // destroy all defunct orphans; do so every so often (after wake up from wait, for example)
  /*node_t *curr = procs->defunct->head;
  while (curr != NULL) {
    node_t *next = curr->next;
    if (get_parent(curr) == NULL) {
      remove(procs->defunct, curr); // first remove from queue
      process_destroy(curr); // then destroy altogether
    }
    curr = next;
  }
  */
  // remove and destroy all orphans
  while (!is_empty(procs->orphans))
    process_destroy(dequeue(procs->orphans));
}
