/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Kernel Syscalls
 */

#include "syscall.h"

/////////////// Basic Process Coordination

/* Create a new child pcb_t
 * assign new pid and its ppid
 * Copy region 1 page table, uc, and kc
 * state = ready
 * get new frames for kernel stack
 * copy region 0, change stack part to map to new frames above
 * return (not sure how to get one return pid, other 0. general regs in UC?)
 */
int KernelFork (void);

/* Load Program with exec args and pcb_t  */
int KernelExec (char *filename, char **argvec);

/* assign status & state = zombie
 * free anything malloc'd in pcb.
 * if parent is dead(not in proc table), remove from proc table
 * otherwise, leave it in proc table                          
 */
void KernelExit (int status);

/* Loop through children, removing dead children along the way
 * if there is zombie, return zombie pid and edit status_ptr if nonNULL
 * if no zombie but still alive children, change state = block.
 *      - loop again after some time? or somehow a child signals its exit
 * if no/all-dead children, return error
 */
int KernelWait (int *status_ptr);

/* Return pid from current pcb*/
int KernelGetPid (void);

/* Check if heap will shrink below to user data or stack or invalid, error if so
 * add or remove page table entries depending on if addr is above or below current brk                                                    
 */
int KernelBrk (void *addr);

/* clock_ticks < 1 error checked
 * status = block, track hardware clock trap, after x time
 */
int KernelDelay (int clock_ticks);

////////////// I/O Syscalls

/* Check if TtyReceive has extra chars
 * if not, state = block and wait for TrapTtyReceive to come in
 * Once there are lines to receive, Collect TrapTtyReceive
 * Validate buf based on len, return whatever necessary
 *   - THought: should Trap handler call TtyRecieve or this funct
 */
int KernelTtyRead (int tty_id, void *buf, int len);

/* Write with hardware funct TtyTransmit
 * Validate error, and return whatever necessary
 */
int KernelTtyWrite (int tty_id, void *buf, int len);

//////////////// IPC Syscalls

/* Initialize pipe_t with id, lock, queues*/
int KernelPipeInit (int *pipe_idp);

/* Check if someone is reading/writing, block if so
 * check if there are bytes to read from buffer, if not return.
 * if so, acquire lock
 * put pipe's contents into param buf
 * release lock
 */
int KernelPipeRead (int pipe_id, void *buf, int len);

/* Check if someone is reading/writing, block if so
 * acquire lock, write to pipe buffer (may need to reallocate buffer)
 * release lock
 */
int KernelPipeWrite (int pipe_id, void *buf, int len);

//////////// Synchronization Syscalls

/* initialize new lock_t with its id, locked = 0, initialize queue */
int KernelLockInit (int *lock_idp);

/* if lock is locked
 *      - add process to lock's queue, state = block
 * if not
 *      - make lock locked, take process off queue
 */
int KernelAcquire (int lock_id);

/* make lock variable 0, potentially signal the queue  */
int KernelRelease (int lock_id);

/* Initialize new cond_t with id*/
int KernelCvarInit (int *cvar_idp);

/* Add process to cond_var's queue, state = block
 * return once signaled
 */
int KernelCvarWait (int cvar_id, int lock_id);

/* Signal the next process on cond_var queue */
int KernelCvarSignal (int cvar_id);

/* Signal all the processes on cond_var queue */
int KernelCvarBroadcast (int cvar_id);

/* free and remove all memory in the pipe/lock/condvar of the given id*/
int KernelReclaim (int id);

//////////////// Traps

// Examine the "code" field of user context and decide which syscall to invoke
void TrapKernel(UserContext *uc) {
  TracePrintf(1, "The code of syscall is 0x%x\n", uc->code);
}

// Check the process table to decide which process to schedule; initialize a context switch if necessary                                  
void TrapClock(UserContext *uc) {
  TracePrintf(1, "Clock Trap occured!\n");
}

// Abort current process, switch context
void TrapIllegal(UserContext *uc);
// Check the page table; if illegal access, abort; otherwise, modify page table to reflect memory allocation                              
void TrapMemory(UserContext *uc);
// Abort
void TrapMath(UserContext *uc);
// Read input and store in buffer; set a flag that wakes up blocked process waiting for input
// allocate kheap with buf for len (how do we know how long input line is?)
// use TtyReceive() hardware function to collect into buf
void TrapTtyReceive(UserContext *uc);
// Start next transmission; resumes blocked process
void TrapTtyTransmit(UserContext *uc);
// TBD
void TrapDisk(UserContext *uc);

// temporary trap handler funct for all the unhandled traps.
void TrapTemp(UserContext *uc) {
  TracePrintf(1, "This trap type %d is currently unhandled\n", uc->vector);
}