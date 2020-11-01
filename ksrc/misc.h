/* Erich WOo & Boxian Wang
 * 26 October 2020
 * header file for load.c
 */

#ifndef __MISC_H
#define __MISC_H

#include <ykernel.h>
#include "process.h"
#include "linked_list.h"

int LoadProgram(char *name, char *args[], pcb_t *proc);
void copy_kernel(node_t *child);
void switch_proc(node_t *from, node_t *to);
void save_uc(UserContext *uc);
void restore_uc(UserContext *uc);
void add_return_val(int r);

#endif // __MISC_H
