/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "hardware.h"

/************ Data Structs *********/

typedef struct pcb {
  int pid;
  int ppid;
  // possibly int* children?
  int state;
  int status;
  // address space
  UserContext uc;
  KernelContext kc;
} pcb_t; 
  
typedef struct proc_table {

} proc_table_t;

typedef struct free_frame_vec {

} free_frame_vec_t;

typedef struct page_table {

} page_table_t;

/*********** Function Prototypes *********/

// Basic Process Coordination

/* Create a new child pcb_t
 * assign new pid and its ppid
 * Copy region 1 page table
 * get new frames for kernel stack
 * copy region 0, change stack part to map to new frames above
 * state = ready
 * copy uc and kc
 * return (not sure how to get one return pid, other 0)
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

// I/O Syscalls
int TtyRead (int, void *, int);
int TtyWrite (int, void *, int);

// IPC Syscalls
int PipeInit (int *);
int PipeRead (int, void *, int);
int PipeWrite (int, void *, int);

// Synchronization Syscalls
int LockInit (int *);
int Acquire (int);
int Release (int);
int CvarInit (int *);
int CvarWait (int, int);
int CvarSignal (int);
int CvarBroadcast (int);
int Reclaim (int);

// Traps

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
void TrapTtyReceive(UserContext *);
// Start next transmission; resumes blocked process
void TrapTtyTransmit(UserContext *);
// TBD
void TrapDisk(UserContext *);


// Modify page table entry accordingly; do we need to update all the kernel page tables?
int SetKernelBrk (void *);

/* This is the primary entry point into the kernel */
void KernelStart (char**, unsigned int, UserContext *);

// Kernel Context Switching
KernelContext* KCSwitch(KernelContext*, void*, void*);
KernelContext* KCCopy(KernelContext*, void*, void*);

/********** Function Pseudocodes **********/


