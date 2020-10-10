/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "hardware.h"

/************ Data Structs *********/

typedef struct pcb {
  
} pcb_t; 
  
typedef struct proc_table {

} proc_table_t;

typedef struct free_frame_vec {

} free_frame_vec_t
/*********** Function Prototypes *********/
int Fork (void);
int Exec (char *, char **);
void Exit (int);
int Wait (int *);
int GetPid (void);
int Brk (void *);
int Delay (int);
int TtyRead (int, void *, int);
int TtyWrite (int, void *, int);

int PipeInit (int *);
int PipeRead (int, void *, int);
int PipeWrite (int, void *, int);

int LockInit (int *);
int Acquire (int);
int Release (int);
int CvarInit (int *);
int CvarWait (int, int);
int CvarSignal (int);
int CvarBroadcast (int);
int Reclaim (int);

void TrapKernel(UserContext);
void TrapClock(UserContext);
void TrapIllegal(UserContext);
void TrapMemory(UserContext);
void TrapMath(UserContext);
void TrapTtyReceive(UserContext);
void TrapTtyTransmit(UserContext);
void TrapDisk(UserContext);

/********** Function Pseudocodes **********/
