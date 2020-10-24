/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for kmem.c
 */

#ifndef __KMEM_H
#define __KMEM_H

#include "ykernel.h"
#include "macro.h"

/************ Data Structs *********/

typedef struct f_frame { // tracking which frames in physical are free                                     
  int size; // available number of physical frames                                                         
  unsigned char * bit_vector; // pointer to a bit vector                                                   
  unsigned int avail_pfn; // next available pfn                                                            
  int filled;
} free_frame_t;

typedef struct user_pt { // userland page table                                                            
  void *heap_low; // end of data and start of heap                                                         
  void *heap_high; // brk or the address just below brk?                                                   
  void *stack_low; // top of the user stack                                                                
  pte_t pt[NUM_PAGES_1]; // actual entries                                                                 
} user_pt_t;

typedef struct kernel_stack_pt { // kernel stack page_table
  pte_t pt[NUM_KSTACK_PAGES]; // actual entries
} kernel_stack_pt_t;

typedef struct kernel_global_pt { // includes code, data, heap
  pte_t pt[NUM_PAGES_0]; // actual entries
} kernel_global_pt_t;

void set_pte(pte_t *pte, int valid, int pfn, int prot);

int get_bit(unsigned char *bit_array, int index);

void set_bit(unsigned char *bit_array, int index, int bit);

int vacate_frame(unsigned int pfn);

int get_frame(unsigned int pfn, int auto_assign);

int SetKernelBrk(void *addr);

#endif //__KMEM_H
