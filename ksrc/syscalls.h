/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for syscalls.c
 */

#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#include <ykernel.h>
#include "memory.h"
#include "linked_list.h"
#include "process.h"
#include "scheduling.h"
#include "pilocvario.h"
#include "cswitch.h"

// to access the current proc and proccess table
extern proc_table_t *procs;

extern node_t *init_node, *idle_node;

/****************************** FUNCTION DECLARATIONS *******************************/
// All functions are invoked by TrapKernel

/////////////// Basic Process Coordination

/* Forks a new process as a copy of the current process/parent. 
 */
int KernelFork (void);

int KernelExec (char *filename, char **argvec);

void KernelExit (int status);

int KernelWait (int *status_ptr);

int KernelGetPid (void);

int KernelUserBrk (void *addr);

int KernelDelay (int clock_ticks);

////////////// I/O Syscalls

int KernelTtyRead (int tty_id, void *buf, int len);

int KernelTtyWrite (int tty_id, void *buf, int len);

//////////////// IPC Syscalls  

int KernelPipeInit (int *pipe_idp);

int KernelPipeRead (int pipe_id, void *buf, int len);

int KernelPipeWrite (int pipe_id, void *buf, int len);

//////////// Synchronization Syscalls

int KernelLockInit (int *lock_idp);

int KernelAcquire (int lock_id);

int KernelRelease (int lock_id);

int KernelCvarInit (int *cvar_idp);

int KernelCvarSignal (int cvar_id);

int KernelCvarBroadcast (int cvar_id);

int KernelCvarWait (int cvar_id, int lock_id);

int KernelReclaim (int id);


#endif // __SYSCALLS_H
