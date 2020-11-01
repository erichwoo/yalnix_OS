/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Process structures header file
 */

#ifndef _process_h
#define _process_h

#include "linked_list.h"
#include "memory.h"
#include <ykernel.h>

typedef struct pcb {
  int pid;
  node_t *parent;
  //ll_t* children;
  ll_t *a_children; // alive children
  ll_t *d_children; // defunct children
  user_pt_t *userpt; // user page table
  kernel_stack_pt_t *kstack; // copy of kernel stack page table
  UserContext uc;
  KernelContext kc;
} pcb_t;

int get_pid(node_t *proc);
node_t *get_parent(node_t *proc);
node_t *process_init(void);
node_t *process_copy(node_t* parent);
void process_terminate(node_t *proc, int rc);
void process_destroy(node_t *proc);

#endif //__PROCESS_H
