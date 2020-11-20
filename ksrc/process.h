/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for process.c
 */

#ifndef __PROCESS_H
#define __PROCESS_H

#include <ykernel.h>
#include "linked_list.h"
#include "memory.h"

// the payload of a pcb
typedef struct pcb {
  int pid;
  node_t *parent;    // quick parent-tracking
  ll_t *a_children;  // linked-list of alive children, different nodes but same pcbs
  ll_t *d_children;  // linked-list of defunct children
  user_pt_t *userpt; // user page table
  kernel_stack_pt_t *kstack; // copy of kernel stack page table
  UserContext uc;    
  KernelContext kc;
} pcb_t;

/********************** FUNCTION DECLARATIONS *********************/
// Note: for a given process node,
// node->data is the pcb_t payload typedef'd above

/* Gets the pid of the given process node
 * 
 * @param proc a pointer to the process node to get pid from
 * @return the pid of proc
 */
int get_pid(node_t *proc);

/* Gets the parent of the given process node
 *
 * @param proc a pointer to the process node to get parent from
 * @return the node of its parent process, NULL if no alive parent
 */
node_t *get_parent(node_t *proc);

/* Sets up a blank process node.
 * Allocates the pcb_t payload and creates the node
 *
 * @return the blank process node
 */
node_t *process_init(void);

/* Copies the given process node (prob parent), along with
 * its memory content (user and kernel stack). Does NOT copy 
 * kernel context because whose timing is critical
 * Intended to be used when fork-ing
 *
 * @param parent node to copy 
 * @return the child process node copy
 */
node_t *process_copy(node_t* parent);

/* Terminates a process, freeing all its user frames and
 * unneeded data, orphaning its alive children (make their parent NULL),
 * destroying its defunct children, and storing its return code in node->code
 *
 * Saves the pid value and rc
 *
 * @param proc the process node to terminate
 * @param rc the return code to store
 */
void process_terminate(node_t *proc, int rc);

/* Copmletely destroys/frees the process node and
 * all its associated memory. This funct call must
 * be executed by a different process (not proc)
 *
 * This completes the destruction of a process node,
 * which begins with process_terminate
 *
 * @param proc the process node to destroy
 */
void process_destroy(node_t *proc);

#endif //__PROCESS_H
