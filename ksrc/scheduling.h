/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Process structures header file
 */

#ifndef __SCHEDULING_H
#define __SCHEDULING_H

#include <ykernel.h>
#include "process.h"
#include "linked_list.h"
#include "misc.h"

// typedefs the top-level process table "manager"
typedef struct proc_table {
  node_t *running;  // the current running process (as node)
  ll_t *ready;      // a linked-list queue of ready process nodes
  ll_t *waiting;    // a linked-list of 
  ll_t *delayed;    // a linked-list of delaying process nodes (via Delay syscall)
  ll_t *orphans;    // a linked-list of back-logged orphans to destroy periodically
} proc_table_t;

// THE process table
extern proc_table_t *procs;

// 
extern node_t *init_node, *idle_node;
extern int pilocvar_id;

/**************************** FUNCTION DECLARATIONS ***************************/

/* Init
 */
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
void graveyard(void);
void defunct_blocked(ll_t* blocked, node_t *proc);
node_t* reap_children(ll_t* children);
void reap_orphans(void);


#endif //__SCCHEDULING_H
