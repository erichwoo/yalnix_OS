/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for kmem.c
 */

#ifndef __MEMORY_H
#define __MEMORY_H

#include <ykernel.h>

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

#define MAX_CHECK 256 // max len of arg or string

typedef struct f_frame { // tracking which frames in physical are free                                     
  int size; // available number of physical frames                                                         
  unsigned char * bit_vector; // pointer to a bit vector                                                   
  unsigned int avail_pfn; // next available pfn                                                            
  int filled;
} free_frame_t;

typedef struct user_pt { // userland page table                                                            
  void *data_end; // end of data                                                     
  void *brk; // brk                                                  
  void *stack_low; // top of the user stack                                                                
  pte_t pt[NUM_PAGES_1]; // actual entries                                                                 
} user_pt_t;

typedef struct kernel_stack_pt { // kernel stack page_table
  pte_t pt[NUM_KSTACK_PAGES]; // actual entries
} kernel_stack_pt_t;

typedef struct kernel_global_pt { // includes code, data, heap
  pte_t pt[NUM_PAGES_0]; // actual entries
  void *brk;
} kernel_global_pt_t;

void set_pte(pte_t *pte, int valid, int pfn, int prot);

int vacate_frame(unsigned int pfn);

int get_frame(unsigned int pfn, int auto_assign);

int SetKernelBrk(void *addr);

user_pt_t *new_user_pt(void);

void copy_user_mem(user_pt_t *origin, user_pt_t *dst);

void destroy_usermem(user_pt_t *userpt);

void destroy_kstack(kernel_stack_pt_t* kstack);

int check_addr(void *addr, int prot, user_pt_t* curr_pt);

int check_buffer(int len, void *addr, int prot, user_pt_t* curr_pt);

int check_string(char* addr, user_pt_t* curr_pt);

int check_args(char** args, user_pt_t* curr_pt);

#endif //__MEMORY_H
