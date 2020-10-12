/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "hardware.h"
#include "ykernel.h"

/************ Data Structs *********/

void *interrupt_handlers[TRAP VECTOR SIZE]; // interrupt_handlers 

typedef struct pcb {
  int pid;
  int ppid;
  // possibly pcb_t* children?
  int state;
  int status;
  // address space
  reg1_pt_t *reg1; // region 1 page table management
  kernel_stack_pt_t *k_stack; // copy of kernel stack page table

  UserContext *uc;
  KernelContext *kc;

} pcb_t; 
  
typedef struct proc_table { // maybe a queue?
  int size; // number of current process under management
  pcb_t *head; //pointer to the head of the queue
} proc_table_t;

// tracking which frames in physical are free
typedef struct free_frame_vec {
  int size; // available physical memory size
  char *bit_vector // pointer to a bit vector 
} free_frame_vec_t;

typedef struct reg1_pt { // userland page table
  void *heap_low // end of data and start of heap
  void *heap_high // brk or the address just below brk?
  void *stack_low // top of the user stack
  pte_t pt[]; // actual entries
} reg1_pt_t;

typedef struct kernel_stack_pt { // kernel stack page_table
  void *stack_low // top of the kernel stack
  pte_t pt[]; // actual entries
} kernel_stack_pt_t;

typedef struct kernel_heap_pt {
  void *brk // to be modified by SetKernelBrk
  pte_t pt[]; // actual entries
} kernel_heap_pt_t;

// need some queue data struct to manage incoming pipe/lock/cond_var users
// could possibly help with pcb management with proc_table
typedef struct queue {
  
} queue_t;

typedef struct pipe {
  int id;
  // buffer pointing to memory
  lock_t lock;
  // queue of blocked readers
  // queue of blocked writers
} pipe_t;
  
typedef struct lock {
  int id;
  // 0/1 locked or not
  // queue of processes that want to acquire lock
} lock_t;

typedef struct cond_var {
  int id;
  // queue of waiting processes waiting for signal
} cond_t;

/*********** Function Prototypes and Commented Pseudocode *********/

// Possibly useful functions:

// find one free frame each time ?
pte_t FindFreeFrame(free_frame_vec_t* vec);

// VM setup, doing gymnastics to set up VM, which should be enabled when it returns
/*
1. set up free_frame_vec
2. set up region 0 page table (assign full space for stack?)
3. turn on VM (do we need to care about region 1 at this point?)
*/
void VM_setup();

// configure the first pcb during boot time
// for both idle and init?
/*
1. Create proc_table
2. create and add pcb
3. setup region 1
4. Configure UserContext
5. Somehow return to usermode? (how to do that??? can kernel trap itself? if not where to store the UserContext
so the hardware knows? is it uctxt?)
*/
void PCB_setup();

/////////////// Basic Process Coordination

/* Create a new child pcb_t
 * assign new pid and its ppid
 * Copy region 1 page table, uc, and kc
 * state = ready
 * get new frames for kernel stack
 * copy region 0, change stack part to map to new frames above
 * return (not sure how to get one return pid, other 0. general regs in UC?)
 */
int Fork (void);

/* Load Program with exec args and pcb_t  */
int Exec (char *, char **);

/* assign status & state = zombie
 * free anything malloc'd in pcb. 
 * if parent is dead(not in proc table), remove from proc table
 * otherwise, leave it in proc table 
 */
void Exit (int);

/* Loop through children, removing dead children along the way
 * if there is zombie, return zombie pid and edit status_ptr if nonNULL
 * if no zombie but still alive children, change state = block. 
 *      - loop again after some time? or somehow a child signals its exit
 * if no/all-dead children, return error
 */
int Wait (int *);

/* Return pid from current pcb*/
int GetPid (void);

/* Check if heap will shrink below to user data or stack or invalid, error if so
 * add or remove page table entries depending on if addr is above or below current brk
 */
int Brk (void *);

/* clock_ticks < 1 error checked
 * status = block, track hardware clock trap, after x time
 */
int Delay (int);

////////////// I/O Syscalls

/* Check if TtyReceive has extra chars
 * if not, state = block and wait for TrapTtyReceive to come in
 * Once there are lines to receive, Collect TrapTtyReceive
 * Validate buf based on len, return whatever necessary
 *   - THought: should Trap handler call TtyRecieve or this funct
 */
int TtyRead (int, void *, int);

/* Write with hardware funct TtyTransmit
 * Validate error, and return whatever necessary
 */
int TtyWrite (int, void *, int);

//////////////// IPC Syscalls

/* Initialize pipe_t with id, lock, queues*/
int PipeInit (int *);

/* Check if someone is reading/writing, block if so
 * check if there are bytes to read from buffer, if not return.
 * if so, acquire lock
 * put pipe's contents into param buf
 * release lock
 */
int PipeRead (int, void *, int);

/* Check if someone is reading/writing, block if so
 * acquire lock, write to pipe buffer (may need to reallocate buffer)
 * release lock
 */
int PipeWrite (int, void *, int);

//////////// Synchronization Syscalls

/* initialize new lock_t with its id, locked = 0, initialize queue */
int LockInit (int *);

/* if lock is locked
 *      - add process to lock's queue, state = block
 * if not
 *      - make lock locked, take process off queue
 */
int Acquire (int);

/* make lock variable 0, potentially signal the queue  */
int Release (int);

/* Initialize new cond_t with id*/
int CvarInit (int *);

/* Add process to cond_var's queue, state = block
 * return once signaled
 */
int CvarWait (int, int);

/* Signal the next process on cond_var queue */
int CvarSignal (int);

/* Signal all the processes on cond_var queue */
int CvarBroadcast (int);

/* free and remove all memory in the pipe/lock/condvar of the given id*/
int Reclaim (int);

//////////////// Traps

// Examine the "code" field of user context and decide which syscall to invoke
void TrapKernel(UserContext *);
// Check the process table to decide which process to schedule; initialize a context switch if necessary
void TrapClock(UserContext *);
// Abort current process, switch context
void TrapIllegal(UserContext *);
// Check the page table; if illegal access, abort; otherwise, modify page table to reflect memory allocation
void TrapMemory(UserContext *);
// Abort
void TrapMath(UserContext *);
// Read input and store in buffer; set a flag that wakes up blocked process waiting for input
// allocate kheap with buf for len (how do we know how long input line is?)
// use TtyReceive() hardware function to collect into buf
void TrapTtyReceive(UserContext *);
// Start next transmission; resumes blocked process
void TrapTtyTransmit(UserContext *);
// TBD
void TrapDisk(UserContext *);


// Modify kernel page table:
// The kernel keeps a separate copy of each process's kernel stack page table,
// and copy them when switching context.
// The bottom half of the page table shall remain the same,
// only to be touched by SetKernelBrk.
// This means all process's will be given the same region0 PTBR?
int SetKernelBrk (void *);

/*
1. call VM_setup to set up virtual memory
2. set up trap table
3. call PCB_setup to configure idle
*/
void KernelStart (char**, unsigned int, UserContext *);

// Kernel Context Switching
KernelContext* KCSwitch(KernelContext*, void*, void*);
KernelContext* KCCopy(KernelContext*, void*, void*);
