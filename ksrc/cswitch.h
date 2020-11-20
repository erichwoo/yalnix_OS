/* Erich WOo & Boxian Wang
 * 26 October 2020
 * header file for cswitch.c, and load.c
 */

#ifndef __CSWITCH_H
#define __CSWITCH_H

#include <ykernel.h>
#include "linked_list.h"
#include "process.h"
#include "memory.h"
#include "scheduling.h"

// THE proc table
extern proc_table_t *procs;

// THE global kernel page tables
extern kernel_global_pt_t kernel_pt;

/* Read load.c for documentation */
int LoadProgram(char *name, char *args[], pcb_t *proc);

/* Copies the kernel context into the child's kc, 
 * and copies the kernel stack into the child's kstack
 * The child's pcb must be initialized before calling
 *
 * Wrapper for KCCopy. See cswitch.c for KCCopy documentation
 *
 * @param child the child node to copy kernel into
 */
void copy_kernel(node_t *child);

/* Switches Kernel Contexts from process 'from' to process 'to'.
 *
 * Wrapper for KCSwitch. See cswitch.c for KCSwitch documentation
 *
 * @param from the process node to switch from
 * @param to the process node to switch to
 */
void switch_proc(node_t *from, node_t *to);

/* Saves the CONTENTS of the User Context pointer into the current process
 *
 * @param uc the UserContext pointer to save from
 */
void save_uc(UserContext *uc);

/* Restores the value of the specified UserContext Pointer to
 * equal the current process' UserContext value
 *
 * @param uc the UserContext pointer to restore
 */
void restore_uc(UserContext *uc);

/* Adds the specified return code r to the 0th index of the register (regs[0])
 * in the current process' UserContext.
 *
 * @param r the return value to store in regs[0]
 */
void add_return_val(int r);

#endif // __CSWITCH_H
