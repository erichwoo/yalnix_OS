#include <ykernel.h>
#include "syscalls.h"
#include "memory.h"
#include "linked_list.h"
#include "process.h"
#include "scheduling.h"
#include "pilocvario.h"
#include "misc.h"

/////////////// Basic Process Coordination
extern proc_table_t *procs;
extern node_t *init_node, *idle_node;

int KernelFork(void) {
  node_t *parent = procs->running;
  node_t *child_node = process_copy(parent);
  ready(child_node); // must do it here, because the next line would copy the kernel
  TracePrintf(1, "%d\n", procs->ready->size);
  copy_kernel(child_node); // duplicate
  if (procs->running == parent) return get_pid(child_node); // who am i?
  return 0;
}

/* Load Program with exec args and pcb_t  */
int KernelExec (char *filename, char **argvec) {
  // !!!check parameters here, make sure is legal and readable and are actual strings and have NULL


  LoadProgram(filename, argvec, procs->running->data); // good to go
  return 0; // pretend to return something, if ERROR we actually return to the calling function (if applicable)
}

void KernelExit (int status) {
  if (procs->running == init_node) Halt();
  reap_orphans(); // do the good deed
  process_terminate(procs->running, status);
  TracePrintf(1, "child is %d\n", status);
  check_wait(get_parent(procs->running));
  defunct();
}

int KernelWait (int *status_ptr) {
  // !!!check parameter here, make sure is writable

  pcb_t *parent = procs->running->data;
  if (is_empty(parent->children)) return ERROR;

  node_t *child;
  while ((child = reap_children(parent->children)) == NULL) block_wait(); // have no dead babies
  // now I have the child
  int cid = get_pid(child);

  if (status_ptr != NULL) *status_ptr = child->code;
  reap_orphans(); // destroy children
  return cid;
}

/* Return pid from current pcb*/
int KernelGetPid (void) {
  return get_pid(procs->running);
}


int KernelUserBrk (void *addr) {
  user_pt_t *userpt = ((pcb_t *) procs->running->data)->userpt;
  // check if has enough memory

  if ((unsigned int) addr >= DOWN_TO_PAGE(userpt->stack_low) ||
    (unsigned int) addr < UP_TO_PAGE(userpt->data_end)) return ERROR;

  unsigned int curr_brk_vpn = (UP_TO_PAGE(userpt->brk) >> PAGESHIFT) - 1; // greatest vpn in use
  unsigned int next_brk_vpn = (UP_TO_PAGE(addr) >> PAGESHIFT) - 1; // greatest vpn to be used

  if (next_brk_vpn > curr_brk_vpn) {
    for (int vpn = curr_brk_vpn + 1; vpn <= next_brk_vpn; vpn++) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
      TracePrintf(1, "0x%x\n", vpn << PAGESHIFT);
      WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
  } else if (next_brk_vpn < curr_brk_vpn) {
    // freeing frames
    for (int vpn = curr_brk_vpn; vpn > next_brk_vpn; vpn--) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 0, vacate_frame(userpt->pt[vpn - BASE_PAGE_1].pfn), NONE);
      WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
  }
  
  userpt->brk = (void *) UP_TO_PAGE(addr);
  return 0;
}


int KernelDelay (int clock_ticks) {
  if (clock_ticks < 0) {
    TracePrintf(1, "Error calling KernelDelay(): clock_ticks < 0.\n");
    return ERROR;
  }
  else if (clock_ticks > 0) {
    block_delay(clock_ticks);
  }
  return 0;
}
   
////////////// I/O Syscalls


int KernelTtyRead (int tty_id, void *buf, int len) {
  /// !!!  check param id valid? buf?
  return read_tty(tty_id, len, buf);
}

/* Write with hardware funct TtyTransmit
 * Validate error, and return whatever necessary
 */
int KernelTtyWrite (int tty_id, void *buf, int len) {
  /// !!!  check param id valid? buf?
  return write_tty(tty_id, len, buf);
}

//////////////// IPC Syscalls


int KernelPipeInit (int *pipe_idp) {
  /// check param !!!
  node_t* p = new_pipe(new_id());
  if (pipe_idp != NULL) *pipe_idp = p->code;
  return 0;
}

int KernelPipeRead (int pipe_id, void *buf, int len) {
  /// check param !!!
  node_t *p = find_pipe(pipe_id); /// what if failed?
  return read_pipe(p, len, buf);
}

/* Check if someone is reading/writing, block if so
 * acquire lock, write to pipe buffer (may need to reallocate buffer)
 * release lock
 */
int KernelPipeWrite (int pipe_id, void *buf, int len) {
  /// check param !!!
  node_t *p = find_pipe(pipe_id); /// what if failed?
  return write_pipe(p, len, buf);
}

//////////// Synchronization Syscalls

int KernelLockInit (int *lock_idp) {
  /// check param !!!
  node_t* l = new_lock(new_id());
  if (lock_idp != NULL) *lock_idp = l->code;
  return 0;
}

int KernelAcquire (int lock_id) {
   /// check param !!!
  node_t* l = find_lock(lock_id);
  acquire(l);
  return 0;
}

int KernelRelease (int lock_id) {
   /// check param !!!
  node_t* l = find_lock(lock_id);
  release(l);
  return 0;
}

int KernelCvarInit (int *cvar_idp) {
  /// check param !!!
  node_t* c = new_cvar(new_id());
  if (cvar_idp != NULL) *cvar_idp = c->code;
  return 0;
}

int KernelCvarWait (int cvar_id, int lock_id) {
  /// check param !!!
  node_t *l = find_lock(lock_id), *c = find_cvar(cvar_id);
  wait_cvar(c, l);
  return 0;
}

int KernelCvarSignal (int cvar_id) {
  /// check param !!!
  node_t* c = find_cvar(cvar_id);
  signal_cvar(c);
  return 0;
}

/* Signal all the processes on cond_var queue */
int KernelCvarBroadcast (int cvar_id) {
  node_t* c = find_cvar(cvar_id);
  broadcast(c);
  return 0;
}

/* free and remove all memory in the pipe/lock/condvar of the given id*/
int KernelReclaim (int id) {
  node_t *l = find_lock(id), *c = find_cvar(id), *p = find_pipe(id);
  if (!(p || c || l)) return ERROR;
  else if (p) return destroy_pipe(p);
  else if (c) return destroy_cvar(c);
  else return destroy_lock(l);
}


