/* Erich Woo & Boxian Wang
 * 31 October 2020
 * header file for traps.c
 */

#ifndef __TRAPS_H
#define __TRAPS_H

#include <ykernel.h>
#include "memory.h"
#include "syscalls.h"
#include "misc.h"
#include "scheduling.h"
#include "process.h"
#include "pilocvario.h"

typedef void (*trap_handler_t) (UserContext* uc);

void TrapKernel(UserContext *uc);
                            
void TrapClock(UserContext *uc);

void TrapIllegal(UserContext *uc);
                            
void TrapMemory(UserContext *uc);

void TrapMath(UserContext *uc);

void TrapTtyReceive(UserContext *uc);

void TrapTtyTransmit(UserContext *uc);

void TrapDisk(UserContext *uc);

#endif // __TRAPS_H
