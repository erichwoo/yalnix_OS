/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Process structures header file
 */

#ifndef __SCHEDULING_H
#define __SCHEDULING_H

#include <ykernel.h>
#include "process.h"
#include "linked_list.h"
#include "cswitch.h"

// typedefs the top-level process table "manager"
typedef struct proc_table {
  node_t *running;  // the current running process (as node)
  ll_t *ready;      // a linked-list queue of ready process nodes
  ll_t *waiting;    // a linked-list of blocked processes (specificity unneeded, unlike 'delayed' below)
  ll_t *delayed;    // a linked-list of delaying process nodes (via Delay syscall)
  ll_t *orphans;    // a linked-list of back-logged DEAD orphans to destroy periodically
} proc_table_t;

// THE process table
extern proc_table_t *procs;

// THE idle process node
extern node_t *idle_node;
//extern int pilocvar_id;

/**************************** FUNCTION DECLARATIONS ***************************/

/* Initializes a pointer to a blank proc_table_t and returns it
 * Must be free'd elsewhere
 *
 * @return a pointer to a blank proc_table_t
 */
proc_table_t *proc_table_init(void);

/////////////// Scheduling

/* Enqueues the given process node on the proc_table's ready queue
 *
 * @param proc the process node to enqueue
 */
void ready(node_t *proc);

/* Switches the current running process out with the given process node
 * Calls KCS KCSwitch to switch kernel contexts from CURR -> NEXT
 *
 * @param next the next process node to switch to
 */
void run_next(node_t *next);

/* Runs the next process at the head of the ready queue.
 * If there are no processes in the ready queue, dispatch idle.
 * Calls run_next after determinig which process node to run next
 */
void run_next_ready(void);

/* Preempts the running process with the given next node.
 * ready()'s the current process and run_next()'s on the given node.
 * Idle is NEVER put on the ready queue, however.
 *
 * Note: this function is currently unused in our OS
 *
 * @param next the next process node to preempt
 */
void preempt(node_t* next);

/* Round Robin Preempts, switching the current process with the 
 * head of the ready queue, if non-empty. ready()'s the current
 * process (unless IDLE), and run_next_ready()'s
 */
void rr_preempt(void);

/* Unblocks the given process node from the given blocked ll.
 * Removes proc from the blocked ll, and enqueues it to the ready queue
 *
 * Does nothing if the blocked ll is empty, or FINISH LATER...
 * 
 * @param blocked the blocked ll to unblock the process from
 * @param proc the process node to unblock
 */
void unblock(ll_t* blocked, node_t *proc);

/* Unblock the head of the given blocked ll and 
 * enqueue it to the ready queue.
 * Calls unblock() with proc = head of ll
 *
 * @param blocked the blocked ll to unblock the head from
 */
void unblock_head(ll_t *blocked);

/* Unblock all process nodes on the given blocked ll,
 * enqueueing them to the ready queue.
 * The resulting blocked ll will be empty when returning 
 *
 * @param blocked the blocked ll to unblock all
 */
void unblock_all(ll_t *blocked);

/* Block the current process and KCswitch processes to the next ready
 * Enqueues the current process to the given block ll, and run_next_ready()'s
 *
 * @param block_list the ll to put the current process on
 */
void block(ll_t* block_list);

/* Same as block() above, but instead inserts the current process at the head
 * of the block list. Used for quick (and specified) removal later on
 *
 * @param block_list the ll to put the current process on
 */
void h_block(ll_t* block_list);

/* Wrapper that calls block(), with param block_list = waiting ll. */
void block_wait(void);

/* Checks if the given parent node was Wait()-ing for a child to terminate.
 * Unblocks the parent from the waiting ll if so.
 * Called when a child exits
 *
 * @param parent the parent to check if waiting
 */
void check_wait(node_t* parent);

/* Wrapper that calls block(), with param blcok_list = delayed ll.
 * Also sets the current process node's code value to the given delay
 * int (aka # of clock-ticks to delay)
 *
 * @param delay the # of clock-ticks to delay
 */
void block_delay(int delay);

/* Iterates through each node in the delayed ll, decrementing 
 * each node's code/delay value, unblocking those that reach 0
 */
void check_delay(void);

/* Adds the current process to the graveyard, and Kswitches to the next ready
 * 1. If the proc has a parent:
 *       proc is added to parent's ll of defunct children
 *       and check_wait()'s if its parent was waiting for their death
 * 2. If no parent:
 *       The proc is added to the proc table's DEAD orphan ll, to be destroyed later
 * Calls run_next_ready() afterwards to run next
 * 
 * Note: the current process should be process_terminate()'d before this function is called
 * as this function immediately switches to the next process
 */
void graveyard(void);

/* For some reason if we have to kill a blocked process, graveyard this proc
 * 
 * Note: currently unused in OS
 *
 * @param blocked the blocked ll the proc was in
 * @param proc the process node to kill
 */
void defunct_blocked(ll_t* blocked, node_t *proc);

/* node_t* reap_children(ll_t* children);*/

/* Reap/Destroy all dead orphans on the proc table's orphan ll
 * Removes each orphan from the list and Calls process_destroy() on each one
 */
void reap_orphans(void);


#endif //__SCHEDULING_H
