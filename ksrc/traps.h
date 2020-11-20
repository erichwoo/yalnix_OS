/* Erich Woo & Boxian Wang
 * 31 October 2020
 * header file for traps.c
 */

#ifndef __TRAPS_H
#define __TRAPS_H

#include <ykernel.h>
#include "memory.h"
#include "syscalls.h"
#include "cswitch.h"
#include "scheduling.h"
#include "process.h"
#include "pilocvario.h"

// a clean typedef for ptr to a general trap-handler function below
// used when booting to hook up the trap-handler vector table
typedef void (*trap_handler_t) (UserContext* uc); 

/********************************** FUNCTION DECLARATIONS ******************************************/

// All Trap Functions below, besides aborting traps,
// first put a COPY the uc* param into the current process and
// end by RESTORING the value of uc* param to equal the current process's uc

/* Executes the requested syscall depending on the given uc*.
 * value of 'code' in uc dictates which syscall, and its arguments
 * in the uc's registers. Saves the return value of the syscall in the uc's regs[0]
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapKernel(UserContext *uc);

/* Decrements the delay-time of all the currently delaying processes, 
 * unblocking those reaching 0. Also round-robin preempts the next process,
 * dispatching idle if there are no runnable processes
 *
 * @param uc a pointer to the running process' current UserContext 
 */
void TrapClock(UserContext *uc);

/* Aborts the current process, and continues running other processes.
 * param uc is unused.
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapIllegal(UserContext *uc);

/* Determines whether this trap is an implicit request by the current process
 * enlarge the amount of memory allocated to the process’s stack, in which case
 * the function allocates pages to "cover" up to 'addr' in the uc param, and returns.
 *
 * Otherwise, or if the above allocation errors, the current process aborts
 * and the function dispatches other processes
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapMemory(UserContext *uc);

/* Aborts the current process, and continues running other processes.
 * param uc is unused.
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapMath(UserContext *uc);

/* Reads input from the indicated terminal using TtyReceive hardware function,
 * and stores it in the terminal's input buffer,
 * unblocking any processes waiting to TtyRead from that terminal
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapTtyReceive(UserContext *uc);

/* Unblocks the process which last wrote/TtyTransmit'd (and caused this Trap), 
 * and resets the Terminal's buffer for subsequent Transmissions
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapTtyTransmit(UserContext *uc);

/* Does nothing, as this OS doesn't implement disk functionality
 *
 * @param uc a pointer to the running process' current UserContext
 */
void TrapDisk(UserContext *uc);

#endif // __TRAPS_H
