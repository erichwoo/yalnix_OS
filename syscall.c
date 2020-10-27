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
int KernelFork(void);

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
int KernelGetPid (void) {
  int pid = get_pid();
  TracePrintf(1, "KernelGetPid() got %d\n", pid);
  return pid;
}

/* Check if heap will shrink below to user data or stack or invalid, error if so
 * add or remove page table entries depending on if addr is above or below current brk
 */
int KernelBrk (void *addr) {
  user_pt_t* user_pt = ptable->curr->data->reg1;
  TracePrintf(1, "The stack starts at     0x%x\n", user_pt->stack_low);
  TracePrintf(1, "The brk is currently    0x%x\n", user_pt->heap_high);
  TracePrintf(1, "The heap starts at      0x%x\n", user_pt->heap_low);
  TracePrintf(1, "Trying to brk to addr   0x%x\n", addr);
  // error if addr is DOWN into user data or UP into user stack
  if ((unsigned int) addr >= (unsigned int) user_pt->stack_low ||
      (unsigned int) addr <= (unsigned int) user_pt->heap_low) {
    TracePrintf(1, "Trying to brk into stack or into data. Error\n");
    return ERROR;
  }

  unsigned int new_brk = UP_TO_PAGE(addr); // round up the page not to include
  unsigned int curr_brk_vpn = (unsigned int) user_pt->heap_high >> PAGESHIFT;
  unsigned int new_brk_vpn = (unsigned int) new_brk >> PAGESHIFT; 

  // since not including uptopage(addr), if addr is above current brk 
  if (new_brk_vpn > curr_brk_vpn) { // DEPENDS ON IF BRK IS PART OF HEAP OR NAH
    // walk up each page starting at current brk upto JUST BEFORE the new brk
    // for each page, find free frame map it to VALID page table entry
    for (int vpn = curr_brk_vpn; vpn < new_brk_vpn; vpn++) {
      set_pte(&user_pt->pt[vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
      TracePrintf(1, "Allocating page %d\n", vpn);
    }
  }
  // if addr is lower than current brk
  else if (new_brk_vpn < curr_brk_vpn) {
    // walk down pages startin at current brk down to new brk (INCLUSIVE)
    // and vacate frame for each page
    for (int vpn = curr_brk_vpn; vpn > new_brk_vpn; vpn--) {
      set_pte(&user_pt->pt[vpn - BASE_PAGE_1], 0, vacate_frame(user_pt->pt[vpn - BASE_PAGE_1].pfn), NONE);
      TracePrintf(1, "Deallocating page %d\n", vpn);
    }
  }
  // do nothing if new_brk is same as curr_brk
  user_pt->heap_high = (void*) new_brk; // change brk to new_brk
  TracePrintf(1, "The new brk is 0x%x\n", new_brk);
  return 0;
}

/* clock_ticks < 1 error checked
 * status = block, track hardware clock trap, after x time
 */
int KernelDelay (int clock_ticks) {
  if (clock_ticks < 0)
    TracePrintf(1, "Error calling KernelDelay(): clock_ticks < 0.\n");
  else if (clock_ticks == 0)
    return 0;

  // assign number of ticks to current pcb's reg
  ptable->curr->data->regs[0] = clock_ticks;
  // block current process
  TracePrintf(1, "Pid %d blocking for %d clock_ticks...\n", ptable->curr->data->pid, clock_ticks);
  block();
  schedule_next();
  return 0;
}
   
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
  /*if (uc->code == YALNIX_FORK)
    KernelFork();
  else if (uc->code == YALNIX_EXEC)
    KernelExec((char*) uc->regs[0], (char**) uc->regs[1]);
  else if (uc->code == YALNIX_EXIT)
    KernelExit((int) uc->regs[0]);
  else if (uc->code == YALNIX_WAIT)
    KernelWait((int*) uc->regs[0]);
  */
  if (uc->code == YALNIX_GETPID)
    KernelGetPid();
  else if (uc->code == YALNIX_BRK)
    KernelBrk((void*) uc->regs[0]);
  else if (uc->code == YALNIX_DELAY)
    KernelDelay((int) uc->regs[0]);
  /*
    else if (uc->code == YALNIX_TTY_READ)
    KernelTtyRead((int) uc->regs[0], (void*) uc->regs[1], (int) uc->regs[2] );
  else if (uc->code == YALNIX_TTY_WRITE)
    KernelTtyWrite();
  */
  
  /*else if (uc->code == YALNIX_REGISTER)
    KernelExit();
  else if (uc->code == YALNIX_SEND)
    KernelExit();
  else if (uc->code == YALNIX_RECEIVE)
    KernelExit();
  else if (uc->code == YALNIX_RECEIVESPECIFIC)
    KernelExit();
  else if (uc->code == YALNIX_REPLY)
    KernelExit();
  else if (uc->code == YALNIX_FORWARD)
    KernelExit();
  else if (uc->code == YALNIX_COPY_FROM)
    KernelExit();
  else if (uc->code == YALNIX_COPY_TO)
    KernelExit();
  else if (uc->code == YALNIX_READ_SECTOR)
    KernelExit();
  else if (uc->code == YALNIX_WRITE_SECTOR)
    KernelExit();
  */

  /*else if (uc->code == YALNIX_PIPE_INIT)
    KernelPipeInit();
  else if (uc->code == YALNIX_PIPE_READ)
    KernelPipeRead();
  else if (uc->code == YALNIX_PIPE_WRITE)
    KernelPipeWrite();
  */

  /*else if (uc->code == YALNIX_NOP)
    KernelExit();
  else if (uc->code == YALNIX_SEM_INIT)
    KernelExit();
  else if (uc->code == YALNIX_SEM_UP)
    KernelExit();
  else if (uc->code == YALNIX_SEM_DOWN)
    KernelExit();
  */

  /*else if (uc->code == YALNIX_LOCK_INIT)
    KernelLockInit();
  else if (uc->code == YALNIX_LOCK_ACQUIRE)
    KernelAcquire();
  else if (uc->code == YALNIX_LOCK_RELEASE)
    KernelRelease();
  else if (uc->code == YALNIX_CVAR_INIT)
    KernelCvarInit();
  else if (uc->code == YALNIX_CVAR_SIGNAL)
    KernelCvarSignal();
  else if (uc->code == YALNIX_CVAR_BROADCAST)
    KernelCvarBroadcast();
  else if (uc->code == YALNIX_CVAR_WAIT)
    KernelCvarWait();
  else if (uc->code == YALNIX_RECLAIM)
    KernelReclaim();
  */
  
  /*else if (uc->code == YALNIX_CUSTOM_0)
    KernelExit();
  else if (uc->code == YALNIX_CUSTOM_1)
    KernelExit();
  else if (uc->code == YALNIX_CUSTOM_2)
    KernelExit();
  else if (uc->code == YALNIX_ABORT)
    KernelExit();
  else if (uc->code == YALNIX_BOOT)
    KernelExit();
  */
}

// Check the process table to decide which process to schedule; initialize a context switch if necessary                                  
void TrapClock(UserContext *uc) {
  TracePrintf(1, "Clock Trap occured!\n");
  // copy uc into pcb
  ptable->curr->data->uc = uc;

  // Look at all blocked and decrement clock_ticks
  for (pcb_node_t* pcb_node = ptable->blocked->head; pcb_node != NULL; pcb_node = pcb_node->next) {
    if (pcb_node->data->regs[0]) { // if delayed
      pcb_node->data->regs[0]--;
      TracePrintf(1, "Pid %d's delay has been decremented and is now %d\n", pcb_node->data->pid, pcb_node->data->regs[0]);
      if (pcb_node->data->regs[0] == 0) {
	TracePrintf(1, "Pid %d is being unblocked from its delay\n", pcb_node->data->pid);
	unblock(pcb_node->data->pid);
      }
    }
  }

  // preempt
  rr_preempt();
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
