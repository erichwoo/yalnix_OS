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
 * @param filename the filename of the new program to run
 * @param argvec the args for this new program
 * @return ERROR on error, otherwise should never return on success
 */
int KernelExec (char *filename, char **argvec);

/* Per Yalnix Manual:
 * - The current process is terminated, status value is saved 
 * for possible later collection by the parent Wait. Frees all unneeded resources.
 * - If the initial process exits, the system will halt.
 * - This call can never return.
 *
 * @param status the status to save for the parent
 */
void KernelExit (int status);

/* Per Yalnix Manual:
 * - Collect the pid and exit status returned by a child process 
 * of the calling program. If status ptr is not null, the exit status
 * is copied to that address.
 * - Block until there is an exiting child to collect
 * 
 * @param status_ptr the pointer to save exit status to
 * @return the pid of exiting child on success, ERROR if no remaining children
 */
int KernelWait (int *status_ptr);

/* Gets the pid of the calling process
 *
 * @return the pid of the calling process
 */
int KernelGetPid (void);

/* Sets the user process' new break to addr, allocating or deallocating
 * enough memory to cover only up to the specified address.
 *
 * @param addr the desired address for the new break
 * @return 0 on success, ERROR otherwise (invalid or not enough memory))
 */
int KernelUserBrk (void *addr);

/* Per Yalnix Manual:
 * - The calling process is blocked until at least 
 * clock-ticks clock interrupts have occurred after the call. 
 *
 * @param clock_ticks the # of clock-ticks to delay
 * @return 0 on completion of delay, ERROR otherwise
 */
int KernelDelay (int clock_ticks);

////////////// I/O Syscalls

/* Per Yalnix Manual:
 * - Read the next line of input from terminal tty id, 
 * copying it into the buffer referenced by buf. The max length of the line
 * to be returned is given by len. The line returned in the buffer is not null-terminated.
 *
 * This function calls wrapper function read_tty(); see documentation in pilocvario.h
 *
 * @param tty_id the terminal id to read from
 * @param buf the buffer to copy into
 * @param len the desired lenth of bytes to copy
 * @return # of bytes actually copied into the calling 
 *         process’s buffer on success, ERROR otherwise
 */
int KernelTtyRead (int tty_id, void *buf, int len);

/* Per Yalnix Manual:
 * - Write the contents of the buffer referenced by buf to the terminal tty id.
 * The length of the buffer in bytes is given by len. The calling process is
 * blocked until all chars from the buffer have been written on the terminal. 
 *
 * This function calls wrapper function write_tty(); see documentation in pilocvario.h
 *
 * @param tty_id the id of terminal to write to
 * @param buf the characters to write to the terminal
 * @param len the length of bytes to write
 * @return # of bytes written (len) on success, ERROR otherwise
 */
int KernelTtyWrite (int tty_id, void *buf, int len);

//////////////// IPC Syscalls  

/* Creates a new pipe and save its identifier at *pipe_idp.
 *
 * @param pipe_idp the pipe identifier pointer
 * @return 0 on success, ERROR otherwise
 */
int KernelPipeInit (int *pipe_idp);

/* Per Yalnix Manual:
 * - Read len consecutive bytes from the named pipe into the buffer
 * starting at address buf, following the standard semantics:
 *   – If the pipe is empty, then block the caller.
 *   – If the pipe has plen ≤ len unread bytes, give all of them to the caller and return.
 *   – If the pipe has plen > len unread bytes, give the first len bytes to caller 
 *                           and return. Retain the unread plen − len bytes in the pipe.
 *
 * This function calls wrapper function read_pipe(). See pilocvario.h for documentation.
 *
 * @param pipe_id the id of the pipe to read from
 * @param buf the buffer to copy the pipe contents to
 * @param len the desired length of bytes to read
 * @return # of bytes read on success, ERROR otherwise
 */
int KernelPipeRead (int pipe_id, void *buf, int len);

/* Per Yalnix Manual:
 * - Write the len bytes starting at buf to the named pipe. 
 * Return as soon as you get the bytes into the buffer.
 *
 * This function calls wrapper function read_pipe(). See pilocvario.h for documentation.
 *
 * @param pipe_id the id of the pipe to write to
 * @param buf the chars to start writing from to the pipe
 * @param len the length of bytes to write
 * @return # of bytes written to the pipe on success, ERROR otherwise
 */
int KernelPipeWrite (int pipe_id, void *buf, int len);

//////////// Synchronization Syscalls

/* Creates a new lock and save its identifier at *lock_idp.
 *
 * @param lock_idp the lock identifier pointer
 * @return 0 on success, ERROR otherwise
 */
int KernelLockInit (int *lock_idp);

/* Acquires the specified lock.
 *
 * This function calls wrapper function acquire(). See pilocvario.h for documentation.
 * @param lock_id the id of the lock to acquire
 * @return 0 on success, ERROR otherwise
 */
int KernelAcquire (int lock_id);

/* Releases the specified lock.
 *
 * This function  calls wrapper function release(). See pilocvario.h for documentation.
 *
 * @param lock_id the id of the lock to release
 * @return 0 on success, ERROR otherwise
 */
int KernelRelease (int lock_id);

/* Creates a new cvar and save its identifier at *cvar_idp.
 *
 * @param cvar_idp the cvar identifier pointer
 * @return 0 on success, ERROR otherwise
 */
int KernelCvarInit (int *cvar_idp);

/* Signals the condition variable specified
 * 
 * This function calls wrapper function signal_cvar(). See pilocvario.h for documentation.
 *
 * @param cvar_id the id of cvar to signal
 * @return 0 on success, ERROR otherwise
 */
int KernelCvarSignal (int cvar_id);

/* Broadcasts the condition variable specified
 *
 * This function calls wrapper function broadcast(). See pilocvario.h for documentation.
 *
 * @param cvar_id the id of cvar to signal
 * @return 0 on success, ERROR otherwise
 */
int KernelCvarBroadcast (int cvar_id);

/* Releases the specified lock and waits on the specified condition variable
 * When the lock is finally acquired, the call returns to userland.   
 * 
 * This function calls wrapper function wait_cvar(). See pilocvario.h for documentation.
 *
 * @param cvar_id the id of cvar to wait on
 * @param lock_id the lock the cvar is contained in
 * @return 0 on success, ERROR otherwise
 */
int KernelCvarWait (int cvar_id, int lock_id);

/* Reclaims/destroys the specified pipe/lock/cvar and its contents
 *
 * @param id the id of pipe/lock/cvar to reclaim/destroy
 * @return 0 on success, ERROR if no pipe/lock/cvar found with specified id
 */
int KernelReclaim (int id);

#endif // __SYSCALLS_H
