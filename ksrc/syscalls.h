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

/****************************** FUNCTION DECLARATIONS *******************************/
// All functions are invoked by TrapKernel

/////////////// Basic Process Coordination

/* Forks a new process as a copy of the current process/parent
 *
 * @return the pid of the child if process is parent, 0 if child, 
 *         ERROR if no memory left for forking
 */
int KernelFork (void);

/* Per Yalnix Manual:
 * - Replace the currently running program in the calling process’s memory 
 * with the program stored in the file named by filename. argvec points
 * to a vector of arguments to pass to the new program as its argument list.
 *
 * @param filename
 * @param argvec
 * @return
 */
int KernelExec (char *filename, char **argvec);

/*
 * @param status
 */
void KernelExit (int status);

/*
 * @param status_ptr
 * @return
 */
int KernelWait (int *status_ptr);

/*
 * @return
 */
int KernelGetPid (void);

/*
 * @param addr
 * @return
 */
int KernelUserBrk (void *addr);

/*
 * @param clock_ticks
 * @return
 */
int KernelDelay (int clock_ticks);

////////////// I/O Syscalls

/*
 * @param tty_id
 * @param buf
 * @param len
 * @return
 */
int KernelTtyRead (int tty_id, void *buf, int len);

/*
 * @param tty_id
 * @param buf
 * @param len
 * @return
 */
int KernelTtyWrite (int tty_id, void *buf, int len);

//////////////// IPC Syscalls  

/*
 * @param pipe_idp
 * @return
 */
int KernelPipeInit (int *pipe_idp);

/*
 * @param tty_id
 * @param buf
 * @param len
 * @return
 */
int KernelPipeRead (int pipe_id, void *buf, int len);

/*
 * @param tty_id
 * @param buf
 * @param len
 * @return
 */
int KernelPipeWrite (int pipe_id, void *buf, int len);

//////////// Synchronization Syscalls

/*
 * @param lock_idp
 * @return
 */
int KernelLockInit (int *lock_idp);

/*
 * @param lock_id
 * @return
 */
int KernelAcquire (int lock_id);

/*
 * @param lock_id
 * @return
 */
int KernelRelease (int lock_id);

/*
 * @param cvar_idp
 * @return
 */
int KernelCvarInit (int *cvar_idp);

/*
 * @param cvar_id
 * @return
 */
int KernelCvarSignal (int cvar_id);

/*
 * @param cvar_id
 * @return
 */
int KernelCvarBroadcast (int cvar_id);

/*
 * @param cvar_id
 * @param lock_id
 * @return
 */
int KernelCvarWait (int cvar_id, int lock_id);

/*
 * @param id
 * @return
 */
int KernelReclaim (int id);


#endif // __SYSCALLS_H
