/* Erich Woo & Boxian Wang
 * 31 October 2020
 * Syscall FUnctionality. See syscall.h for detailed documentation
 */

#include "syscalls.h"

// to access the current process and THE proc table
extern proc_table_t *procs;

// for halting if curr is init
extern node_t *init_node;

int KernelFork(void) {
  user_pt_t *curr_pt = ((pcb_t*) procs->running->data)->userpt;
  if (no_kernel_memory(curr_pt->size + 3)) {
    TracePrintf(1, "no memory left for forking\n");
    return ERROR;
  } 
  TracePrintf(1, "Process %d Fork-ing\n", ((pcb_t*) procs->running->data)->pid);
  node_t *parent = procs->running;
  node_t *child_node = process_copy(parent);
  ready(child_node); // must do it here, because the next line would copy the kernel
  copy_kernel(child_node); // duplicate
  if (procs->running == parent) return get_pid(child_node); // who am i?
  return 0;
}

/* Load Program with exec args and pcb_t  */
int KernelExec (char *filename, char **argvec) {
  user_pt_t *curr_pt = ((pcb_t*) procs->running->data)->userpt;
  if (!check_string(filename, curr_pt) || !check_args(argvec, curr_pt)) return ERROR;
  TracePrintf(1, "Process %d Exec-ing into program %s\n", ((pcb_t*) procs->running->data)->pid, filename);
  int code = LoadProgram(filename, argvec, procs->running->data); // good to go
  if (code == KILL) {
    TracePrintf(1, "Load Program Error, killing process...");
    KernelExit(ERROR);
  }
  return code; // error
}

void KernelExit (int status) {
  TracePrintf(1, "Process %d Exiting...\n", ((pcb_t*) procs->running->data)->pid);
  if (procs->running == init_node) Halt();
  reap_orphans(); // do the good deed and cleanup accumulated orphans
  process_terminate(procs->running, status);
  //TracePrintf(1, "Exit status is %d\n", status);
  graveyard(); // give self to parent or orphan self, and schdeule next
}

int KernelWait (int *status_ptr) {
  user_pt_t *curr_pt = ((pcb_t*) procs->running->data)->userpt;
  if (status_ptr != NULL && !check_addr(status_ptr, PROT_WRITE, curr_pt)) return ERROR;
  TracePrintf(1, "Process %d Waiting for a child...\n", ((pcb_t*) procs->running->data)->pid);
  pcb_t *parent = procs->running->data;
  if (is_empty(parent->a_children) && is_empty(parent->d_children)) {
    TracePrintf(1, "Process has no children to wait on. Returning error\n");
    return ERROR;
  }
  
  node_t *child;
  while ((child = dequeue(parent->d_children)) == NULL)
    block_wait();
  
  // now I have the child
  int cid = get_pid(child);

  if (status_ptr != NULL) *status_ptr = child->code;
  process_destroy(child); // destroy the reaped child
  reap_orphans(); // do the good deed
  return cid;
}

int KernelGetPid (void) {
  return get_pid(procs->running);
}

int KernelUserBrk (void *addr) {
  TracePrintf(1, "User Process %d Brk-ing to address 0x%x\n", ((pcb_t*) procs->running->data)->pid, addr);
  user_pt_t *userpt = ((pcb_t *) procs->running->data)->userpt;
  // check if has enough memory

  if ((unsigned int) addr >= DOWN_TO_PAGE(userpt->stack_low) - PAGESIZE ||
    (unsigned int) addr < UP_TO_PAGE(userpt->data_end)) return ERROR;

  unsigned int curr_brk_vpn = (UP_TO_PAGE(userpt->brk) >> PAGESHIFT) - 1; // greatest vpn in use
  unsigned int next_brk_vpn = (UP_TO_PAGE(addr) >> PAGESHIFT) - 1; // greatest vpn to be used

  if (next_brk_vpn > curr_brk_vpn) {
    if (next_brk_vpn - curr_brk_vpn > frames_left()) return ERROR;
    userpt->size += next_brk_vpn - curr_brk_vpn;
    for (int vpn = curr_brk_vpn + 1; vpn <= next_brk_vpn; vpn++) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
      WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
  } else if (next_brk_vpn < curr_brk_vpn) {
    // freeing frames
    userpt->size -= curr_brk_vpn - next_brk_vpn;
    for (int vpn = curr_brk_vpn; vpn > next_brk_vpn; vpn--) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 0, vacate_frame(userpt->pt[vpn - BASE_PAGE_1].pfn), NONE);
      WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
  }
  
  userpt->brk = (void *) UP_TO_PAGE(addr);
  return 0;
}

int KernelDelay (int clock_ticks) {
  TracePrintf(1, "Process %d Delaying for %d clock ticks\n", ((pcb_t*) procs->running->data)->pid, clock_ticks);
  if (clock_ticks < 0) {
    TracePrintf(1, "Error calling KernelDelay(): clock_ticks < 0.\n");
    return ERROR;
  }
  else if (clock_ticks > 0) {
    block_delay(clock_ticks);
  }
  TracePrintf(1, "Process %d returning from Delay\n", ((pcb_t*) procs->running->data)->pid);
  return 0;
}
   
////////////// I/O Syscalls

int KernelTtyRead (int tty_id, void *buf, int len) {
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (tty_id >= NUM_TERMINALS || !check_buffer(len, buf, PROT_READ, curr_pt)) return ERROR;
  return read_tty(tty_id, buf, len);
}

int KernelTtyWrite (int tty_id, void *buf, int len) {
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (tty_id >= NUM_TERMINALS || !check_buffer(len, buf, PROT_WRITE, curr_pt)) return ERROR;
  return write_tty(tty_id, buf, len);
}

//////////////// IPC Syscalls

int KernelPipeInit (int *pipe_idp) {
  if (no_kernel_memory(1)) return ERROR;
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (!check_addr(pipe_idp, PROT_WRITE, curr_pt)) return ERROR;
  node_t* p = new_pipe(new_id());
  if (pipe_idp != NULL) *pipe_idp = p->code;
  return 0;
}

int KernelPipeRead (int pipe_id, void *buf, int len) {
  node_t *p = find_pipe(pipe_id);
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (p == NULL || !check_buffer(len, buf, PROT_READ, curr_pt)) return ERROR; /// what if failed?
  return read_pipe(p, buf, len);
}

int KernelPipeWrite (int pipe_id, void *buf, int len) {
  node_t *p = find_pipe(pipe_id); /// what if failed?
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (p == NULL || !check_buffer(len, buf, PROT_WRITE, curr_pt)) return ERROR;
  return write_pipe(p, buf, len);
}

//////////// Synchronization Syscalls

int KernelLockInit (int *lock_idp) {
  if (no_kernel_memory(1)) return ERROR;
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (!check_addr(lock_idp, PROT_WRITE, curr_pt)) return ERROR;
  node_t* l = new_lock(new_id());
  if (lock_idp != NULL) *lock_idp = l->code;
  return 0;
}

int KernelAcquire (int lock_id) {
  node_t* l = find_lock(lock_id);
  if (l == NULL) return ERROR;
  return acquire(l);
}

int KernelRelease (int lock_id) {
  node_t* l = find_lock(lock_id);
  if (l == NULL) return ERROR;
  return release(l);
}

int KernelCvarInit (int *cvar_idp) {
  if (no_kernel_memory(1)) return ERROR;
  user_pt_t *curr_pt = ((pcb_t *) procs->running->data)->userpt;
  if (!check_addr(cvar_idp, PROT_WRITE, curr_pt)) return ERROR;
  node_t* c = new_cvar(new_id());
  if (cvar_idp != NULL) *cvar_idp = c->code;
  return 0;
}

int KernelCvarWait (int cvar_id, int lock_id) {
  node_t *l = find_lock(lock_id), *c = find_cvar(cvar_id);
  if (l == NULL || c == NULL) return ERROR;
  wait_cvar(c, l);
  return 0;
}

int KernelCvarSignal (int cvar_id) {
  node_t* c = find_cvar(cvar_id);
  if (c == NULL) return ERROR;
  signal_cvar(c);
  return 0;
}

int KernelCvarBroadcast (int cvar_id) {
  node_t* c = find_cvar(cvar_id);
  if (c == NULL) return ERROR;
  broadcast(c);
  return 0;
}

int KernelReclaim (int id) {
  node_t *l = find_lock(id), *c = find_cvar(id), *p = find_pipe(id);
  if (!(p || c || l)) return ERROR;
  else if (p) return destroy_pipe(p);
  else if (c) return destroy_cvar(c);
  else return destroy_lock(l);
}
