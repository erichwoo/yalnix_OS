/* Erich Woo & Boxian Wang
 * 23 October 2020
 * process manipulation
 */

#include <ykernel.h>
#include "process.h"
#include "linked_list.h"
#include "scheduling.h"
#include "misc.h"

/******** Process management ********/
extern proc_table_t *procs;
extern node_t *init_node, *idle_node;
extern int pilocvar_id;


proc_table_t *initialize_proc_table(void) {
  proc_table_t *p = malloc(sizeof(proc_table_t));
  p->running = NULL;
  p->waiting = new_ll();
  p->ready = new_ll();
  p->delayed = new_ll();
  p->defunct = new_ll();
  return p;
}

void ready(node_t *proc) { // put something in the ready queue
  enqueue(procs->ready, proc);
}

// further suppose before executing this function, the running process is the one we are switching from
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
}// run the head of the ready queue take care of idle; suppose we have node called idle_node; contains actual switching


// preempt the running process with next; idle is never put in the ready queue
void preempt(node_t* next) {
  if (procs->running != idle_node) ready(procs->running);
  run_next(next);
}

void rr_preempt(void) { // ready the current process and run the head of the ready queue
  if (is_empty(procs->ready)) return;
  if (procs->running != idle_node) ready(procs->running);
  run_next_ready();
}

void unblock(ll_t* blocked, node_t *proc) { // remove proc from block and move to ready queue
  if (is_empty(blocked)) return; // safety
  ready(remove(blocked, proc));
}

void unblock_head(ll_t *blocked) {
  unblock(blocked, blocked->head);
}

void unblock_all(ll_t *blocked) {
  for (node_t *curr = blocked->head; curr != NULL; curr = curr->next) 
    unblock(blocked, curr);
}

void block(ll_t* block_list) { // block current, switch to next ready
  enqueue(block_list, procs->running);
  //TracePrintf(1, "my size %d\n", procs->delayed->size);
  run_next_ready();
}

void h_block(ll_t* block_list) { // block current, but put to head, switch to next ready
  insert_head(block_list, procs->running);
  run_next_ready();
}

void block_wait(void) { 
  block(procs->waiting);
}

void check_wait(node_t* parent) { // when a child dies check this
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

void defunct(void) { // move running to graveyard; run next; before doing so the process should be terminated
  enqueue(procs->defunct, procs->running); // even if is an orphan, must move to graveyard before destroying it externally
  run_next_ready();
}

void defunct_blocked(ll_t* blocked, node_t *proc) { // for some reason if we have to kill a blocked process
  enqueue(procs->defunct, remove(blocked, proc));
}

node_t* reap_children(ll_t* children) { // search graveyard for children; return NULL if not found; dont remove children, just orphane it.
  node_t *curr;
  for (curr = procs->defunct->head; curr != NULL; curr = curr->next) {
    if (has_member(children, curr)) {
      ((pcb_t *)curr->data)->parent = NULL; // orphane, will be destroyed next
      return curr;
    }
  }
  return NULL;
}

void reap_orphans(void) { // destroy all defunct orphans; do so every so often (after wake up from wait, for example)
  node_t *curr = procs->defunct->head;
  while (curr != NULL) {
    node_t *next = curr->next;
    if (get_parent(curr) == NULL) {
      remove(procs->defunct, curr); // first remove from queue
      process_destroy(curr); // then destroy altogether
    }
    curr = next;
  }
}

// functionalities for wait(), fork(), exec(), exit() and rr sceduling






// TODO: functionalities for pipe and tty I/O 



// TODO: functionalities for locks and cvars