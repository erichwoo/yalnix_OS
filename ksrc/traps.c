/* Erich Woo & Boxian Wang
 * 31 October 2020
 * Trap functionality. Read trap.h for detailed documentation
 */

#include "traps.h"

// the process table for TrapMemory use
extern proc_table_t* procs;

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

void TrapClock(UserContext *uc) {
  save_uc(uc);
  check_delay();
  rr_preempt();
  restore_uc(uc);
}

void TrapIllegal(UserContext *uc) {
  TracePrintf(0, "Process %d aborting from TrapIllegal\n", KernelGetPid());
  KernelExit(ERROR);
}

void TrapMemory(UserContext *uc) {
  save_uc(uc);
  TracePrintf(1, "TrapMemory at 0x%x\n", uc->addr);
  user_pt_t *userpt = ((pcb_t *)procs->running->data)->userpt;
  if ((unsigned int) uc->addr >= UP_TO_PAGE(userpt->brk) + PAGESIZE && (unsigned int) uc->addr <= DOWN_TO_PAGE(userpt->stack_low)) {
    TracePrintf(1, "Expanding User Stack...\n");
    int curr_vpn = DOWN_TO_PAGE(userpt->stack_low) >> PAGESHIFT;
    int next_vpn = DOWN_TO_PAGE(uc->addr) >> PAGESHIFT;
    if (curr_vpn - next_vpn > frames_left()) {
      TracePrintf(0, "Not enough free frames, aborting\n");
      KernelExit(ERROR);
    }
    for (int vpn = curr_vpn - 1; vpn >= next_vpn; vpn--) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
      WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
    userpt->size += curr_vpn - next_vpn;
    userpt->stack_low = (void *) (next_vpn << PAGESHIFT);
  }
  else { 
    TracePrintf(0, "Aborting: virtual address referenced is not within user stack low and user break\n");
    KernelExit(ERROR);
  }
  restore_uc(uc);
}

void TrapMath(UserContext *uc) {
  TracePrintf(0, "Process %d aborting from TrapMath\n", KernelGetPid());
  KernelExit(ERROR);
}

void TrapTtyReceive(UserContext *uc) {
  TracePrintf(1, "Trap TtyReceive...\n");
  save_uc(uc);
  int tty = uc->code;
  receive(tty);
  restore_uc(uc);
}

void TrapTtyTransmit(UserContext *uc) {
  TracePrintf(1, "Trap TtyTransmit...\n");
  save_uc(uc);
  int tty = uc->code;
  write_alert(tty);
  restore_uc(uc);
}

void TrapDisk(UserContext *uc) {
  TracePrintf(1, "Trap Disk is unimplemented in this OS!\n");
}
