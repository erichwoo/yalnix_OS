/* Erich Woo & Boxian Wang
 * 31 October 2020
 * Trap functionality
 */

#include "traps.h"

//////////////// Traps

extern proc_table_t *procs;

// ALWAYS COPY THE CURRENT UC TO uc BEFORE RETURNING!
void TrapKernel(UserContext *uc) {
  save_uc(uc);
  TracePrintf(1, "The code of syscall is 0x%x\n", uc->code);
  int return_val;
  switch (uc->code) {
    case YALNIX_FORK:
      return_val = KernelFork();
      break;
    case YALNIX_EXEC:
      return_val = KernelExec((char*) uc->regs[0], (char**) uc->regs[1]);
      if (return_val == SUCCESS) {
        restore_uc(uc); // dont return if successful
        return;
      }
      break;
    case YALNIX_EXIT:
      KernelExit((int) uc->regs[0]);
      break;
    case YALNIX_WAIT:
      return_val = KernelWait((int*) uc->regs[0]);
      break;
    case YALNIX_GETPID:
      return_val = KernelGetPid();
      break;
    case YALNIX_BRK:
      return_val = KernelUserBrk((void*) uc->regs[0]);
      break;
    case YALNIX_DELAY:
      return_val = KernelDelay((int) uc->regs[0]);
      break;
    case YALNIX_TTY_READ:
      return_val = KernelTtyRead((int) uc->regs[0], (void*) uc->regs[1], (int) uc->regs[2]);
      break;
    case YALNIX_TTY_WRITE:
      return_val = KernelTtyWrite((int) uc->regs[0], (void*) uc->regs[1], (int) uc->regs[2]);
      break;
    case YALNIX_PIPE_INIT:
      return_val = KernelPipeInit((void *) uc->regs[0]);
      break;
    case YALNIX_PIPE_READ:
      return_val = KernelPipeRead((int) uc->regs[0], (void*) uc->regs[1], (int) uc->regs[2]);
      break;
    case YALNIX_PIPE_WRITE:
      return_val = KernelPipeWrite((int) uc->regs[0], (void*) uc->regs[1], (int) uc->regs[2]);
      break;
    case YALNIX_LOCK_INIT:
      return_val = KernelLockInit((int *) uc->regs[0]);
      break;
    case YALNIX_LOCK_ACQUIRE:
      return_val = KernelAcquire((int) uc->regs[0]);
      break;
    case YALNIX_LOCK_RELEASE:
      return_val = KernelRelease((int) uc->regs[0]);
      break;
    case YALNIX_CVAR_INIT:
      return_val = KernelCvarInit((int *) uc->regs[0]);
      break;
    case YALNIX_CVAR_SIGNAL:
      return_val = KernelCvarSignal((int) uc->regs[0]);
      break;
    case YALNIX_CVAR_BROADCAST:
      return_val = KernelCvarBroadcast((int) uc->regs[0]);
      break;
    case YALNIX_CVAR_WAIT:
      return_val = KernelCvarWait((int) uc->regs[0], (int) uc->regs[1]);
      break;
    case YALNIX_RECLAIM:
      return_val = KernelReclaim((int) uc->regs[0]);
      break;
    default :
      TracePrintf(1, "syscall unhandled \n");
  }
  add_return_val(return_val);
  restore_uc(uc);
}

// Check the process table to decide which process to schedule; initialize a context switch if necessary                                  
void TrapClock(UserContext *uc) {
  save_uc(uc);
  //TracePrintf(1, "Clock Trap occured!\n");
  // recycle stack frame here?
  check_delay();
  rr_preempt();
  restore_uc(uc);
}

// Abort current process, switch context
void TrapIllegal(UserContext *uc);
// Check the page table; if illegal access, abort; otherwise, modify page table to reflect memory allocation    

void TrapMemory(UserContext *uc) {
  save_uc(uc);
  TracePrintf(1, "!!! 0x%x\n", uc->addr);
  TracePrintf(1, "sp 0x%x\n", uc->sp);
  user_pt_t *userpt = ((pcb_t *)procs->running->data)->userpt;
  if (uc->addr >= uc->sp && (unsigned int) uc->addr < VMEM_1_LIMIT) {
    TracePrintf(1, "Expanding User Stack...\n");
    int curr_vpn = DOWN_TO_PAGE(userpt->stack_low) >> PAGESHIFT;
    int next_vpn = DOWN_TO_PAGE(uc->sp) >> PAGESHIFT;
    TracePrintf(1, "slow 0x%x\n", userpt->stack_low);
    for (int vpn = curr_vpn - 1; vpn >= next_vpn; vpn--) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
      WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
    userpt->stack_low = (void *) (next_vpn << PAGESHIFT);
  }
  else 
    helper_abort("Aborting: virtual address referenced is not within user stack low and user break\n");
  restore_uc(uc);
}

// Abort
void TrapMath(UserContext *uc);
// Read input and store in buffer; set a flag that wakes up blocked process waiting for input
// allocate kheap with buf for len (how do we know how long input line is?)
// use TtyReceive() hardware function to collect into buf
void TrapTtyReceive(UserContext *uc) {
  // error checking !!!!
  TracePrintf(1, "Trap TtyReceive...\n");
  save_uc(uc);
  int tty = uc->code;
  receive(tty);
  restore_uc(uc);
}
// Start next transmission; resumes blocked process
void TrapTtyTransmit(UserContext *uc) {
  // error checking !!!!
  TracePrintf(1, "Trap TtyTransmit...\n");
  save_uc(uc);
  int tty = uc->code;
  write_alert(tty);
  restore_uc(uc);
}
// TBD
void TrapDisk(UserContext *uc);

// temporary trap handler funct for all the unhandled traps.
void TrapTemp(UserContext *uc) {
  TracePrintf(1, "This trap type %d is currently unhandled\n", uc->vector);
}
