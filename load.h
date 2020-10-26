/* Erich WOo & Boxian Wang
 * 26 October 2020
 * header file for load.c
 */

#ifndef __LOAD_H
#define __LOAD_H

#include <fcntl.h>
#include <unistd.h>
#include <ykernel.h>
#include <load_info.h>
#include "process.h"
#include "kmem.h"
#include "global.h"

int LoadProgram(char *name, char *args[], pcb_t *proc);

#endif // __LOAD_H
