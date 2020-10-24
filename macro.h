/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Defining all kernel macros
 */

#ifndef __MACRO_H
#define __MACRO_H

#include "ykernel.h"

#define NUM_PAGES_1 (VMEM_1_SIZE / PAGESIZE)
#define NUM_PAGES_0 (VMEM_0_SIZE / PAGESIZE)
#define NUM_KSTACK_PAGES (KERNEL_STACK_MAXSIZE / PAGESIZE)
#define BASE_PAGE_0 (VMEM_0_BASE >> PAGESHIFT) // starting page num of reg 0                               
#define LIM_PAGE_0 (VMEM_0_LIMIT >> PAGESHIFT) // first page above reg 0                                   
#define BASE_PAGE_1 (VMEM_1_BASE >> PAGESHIFT) // starting page num of reg 1                               
#define LIM_PAGE_1 (VMEM_1_LIMIT >> PAGESHIFT) // first page above reg 1                                   
#define BASE_PAGE_KSTACK (KERNEL_STACK_BASE >> PAGESHIFT)
#define LIM_PAGE_KSTACK (KERNEL_STACK_LIMIT >> PAGESHIFT)
#define BASE_FRAME (PMEM_BASE >> PAGESHIFT)
#define CELL_SIZE (sizeof(char) << 3)

#define AUTO 1
#define FIXED 0
#define NONE 0

#define TERMINATED -1
#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define DEFUNCT 3

#endif //__MACRO_H
