/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "hardware.h"

/************ Data Structs *********/

typedef struct pcb {
  int pid;
  int state;
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

void KernelStart(char*, unsigned int, UserContext*);
int SetKernelBrk(void*);

// Kernel Context Switching
KernelContext* KCSwitch(KernelContext*, void*, void*);
KernelContext* KCCopy(KernelContext*, void*, void*);

// Basic Process Coordination
int Fork (void);
int Exec (char *, char **);
void Exit (int);
int Wait (int *);
int GetPid (void);
int Brk (void *);
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


/* 
 * Definitions of functions to be written by student
 */
// Modify page table entry accordingly; do we need 
int SetKernelBrk (void *);

/* This is the primary entry point into the kernel */
void KernelStart (char**, unsigned int, UserContext *);

/********** Function Pseudocodes **********/


