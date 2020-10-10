/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "hardware.h"

/************ Data Structs *********/

typedef struct pcb {
  int pid;
  int state;
  // address space
  UserContext uc;
  KernelContext kc;
} pcb_t; 

typedef struct bitvector;
/*********** Function Prototypes *********/

void KernelStart(char*, unsigned int, UserContext*);
int SetKernelBrk(void*);

// Kernel Context Switching
KernelContext* KCSwitch(KernelContext*, void*, void*);
KernelContext* KCCopy(KernelContext*, void*, void*);

// Basic Process Coordination
int Fork (void);
int Exec (char *, char **);
void Exit (int);
int Wait (int *);
int GetPid (void);
int Brk (void *);
int Delay (int);

// I/O Syscalls
int TtyRead (int, void *, int);
int TtyWrite (int, void *, int);

// IPC Syscalls
int PipeInit (int *);
int PipeRead (int, void *, int);
int PipeWrite (int, void *, int);

// Synchronization Syscalls
int LockInit (int *);
int Acquire (int);
int Release (int);
int CvarInit (int *);
int CvarWait (int, int);
int CvarSignal (int);
int CvarBroadcast (int);
int Reclaim (int);

// Traps
void TrapKernel(UserContext);
void TrapClock(UserContext);
void TrapIllegal(UserContext);
void TrapMemory(UserContext);
void TrapMath(UserContext);
void TrapTtyReceive(UserContext);
void TrapTtyTransmit(UserContext);
void TrapDisk(UserContext);

/********** Function Pseudocodes **********/
