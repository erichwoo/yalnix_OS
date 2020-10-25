/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for syscalls.c
 */

#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "ykernel.h"
#include "process.h" // to access pcb methods when in syscall
#include "kmem.h" // for setKernelBrk

/////////////// Basic Process Coordination

int KernelFork (void);

int KernelExec (char *filename, char **argvec);

void KernelExit (int status);

int KernelWait (int *status_ptr);

int KernelGetPid (void);

int KernelBrk (void *addr);

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

int KernelCvarWait (int cvar_id, int lock_id);

int KernelCvarSignal (int cvar_id);

int KernelCvarBroadcast (int cvar_id);

int KernelReclaim (int id);

//////////////// Traps

void TrapKernel(UserContext *uc);

void TrapClock(UserContext *uc);

void TrapIllegal(UserContext *uc);

void TrapMemory(UserContext *uc);

void TrapMath(UserContext *uc);

void TrapTtyReceive(UserContext *uc);

void TrapTtyTransmit(UserContext *uc);

void TrapDisk(UserContext *uc);

void TrapTemp(UserContext *uc);

#endif //__SYSCALL_H
