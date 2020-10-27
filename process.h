/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Process structures header file
 */

#ifndef __PROCESS_H
#define __PROCESS_H

#include "global.h"
#include "kmem.h" // for user_pt and k_stack_pt manipulation
#include "cswitch.h"

typedef struct pcb {
  int pid;
  int ppid;
  // possibly pcb_t* children?
  int state;
  int rc; // return code
  int regs[8];
  // address space
  user_pt_t *reg1; // region 1 page table management
  kernel_stack_pt_t *k_stack; // copy of kernel stack page table

  UserContext *uc;
  KernelContext *kc;
} pcb_t;

typedef struct pcb_node {
  pcb_t* data;     // payload
  struct pcb_node *next; // for ll and queue
} pcb_node_t;

// pcb linked list
// can be used as queue or simple ll
typedef struct pcb_ll {
  int id; // tells which ll/queue it is use state MACROS
  int count;
  pcb_node_t* head;
  pcb_node_t* tail;
} pcb_ll_t;

// implement round robin aka preemptive FCFS via interrupt clock
typedef struct proc_table { // maybe a queue?
  int count; // number of current process under management
  pcb_node_t *curr; // running process

  pcb_ll_t* new;     // a setup pcb to be admitted to ready queue
  pcb_ll_t* ready;   // will utilize ll's queue functions
  pcb_ll_t* blocked;
  pcb_ll_t* defunct;
} proc_table_t;

// pcb manipulation

pcb_t* initialize_pcb(int ppid, user_pt_t* user_pt, kernel_stack_pt_t* k_stack_pt, UserContext* uc);

int get_pid(void);

// pcb linked list manipulation

pcb_node_t* find(pcb_ll_t* ll, int pid);

// proc table

void initialize_ptable(void);

// wrappers
void schedule_next(void);
void new(pcb_node_t* pcb);
void ready(pcb_node_t* pcb);
void block(void);
void defunct(int rc);
void terminate(void);
void new_ready(int pid);
void unblock(int pid);
void rr_preempt(void);
void print_ptable(void);

//testing
void processtest(void);
#endif //__PROCESS_H
