/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Process structures header file
 */

#ifndef _scheduling_h
#define _scheduling_h

#include "linked_list.h"

typedef struct proc_table {
  node_t *running;
  ll_t *ready;
  ll_t *waiting;
  ll_t *delayed;
  //ll_t *defunct;
} proc_table_t;

proc_table_t *proc_table_init(void);

void ready(node_t *proc);
void run_next(node_t *next);
void run_next_ready(void);
void preempt(node_t* next);
void rr_preempt(void);
void unblock(ll_t* blocked, node_t *proc);
void unblock_head(ll_t *blocked);
void unblock_all(ll_t *blocked);
void block(ll_t* block_list);
void h_block(ll_t* block_list);
void block_wait(void);
void check_wait(node_t* parent);
void block_delay(int delay);
void check_delay(void);
void defunct(void);
void defunct_blocked(ll_t* blocked, node_t *proc);
node_t* reap_children(ll_t* children);
void reap_orphans(void);


#endif //__PROCESS_H
