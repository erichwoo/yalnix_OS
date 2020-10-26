/* Ericch Woo & Boxian Wang
 * 26 October 2020
 * header file for cswitch.c
 */

#ifndef __CSWITCH_H
#define __CSWITCH_H

#include "process.h"
#include "kmem.h"
#include "global.h"
#include "ykernel.h"

KernelContext* KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);

#endif // __CSWITCH_H