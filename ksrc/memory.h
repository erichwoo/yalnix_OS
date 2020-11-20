/* Erich Woo & Boxian Wang
 * 23 October 2020
 * header file for memory.c
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
  int size; // num physical pages                                                               
} user_pt_t;

typedef struct kernel_stack_pt { // kernel stack page_table
  pte_t pt[NUM_KSTACK_PAGES]; // actual entries
} kernel_stack_pt_t;

typedef struct kernel_global_pt { // includes code, data, heap
  pte_t pt[NUM_PAGES_0]; // actual entries
  void *brk;
} kernel_global_pt_t;

/* Sets the page table entry
 *
 * @param pte the page table entry to set
 * @param valid 0/1 for invalid/valid
 * @param pfn the page frame number
 * @param prot the protections
 */
void set_pte(pte_t *pte, int valid, int pfn, int prot);

/* Vacates the specified fram
 *
 * @param the frame to vacate
 * @return 0
 */
int vacate_frame(unsigned int pfn);

/* Gets a free physical frame, marks it as occupied, 
 * and returns the frame #
 *
 * @param pfn if 
 */
int get_frame(unsigned int pfn, int auto_assign);

/* Sets the new kernel break to addr
 *
 * @param addr the desired new brk
 * @return 0 on success, ERR)R if invalid addr
 */
int SetKernelBrk(void *addr);

/* Initializes and returns a new user page table
 *
 * @return the initialized user page table
 */
user_pt_t *new_user_pt(void);

/* Copies user memory content
 *
 * @param origin the original user page table
 * @param dst the destination user page table
 */
void copy_user_mem(user_pt_t *origin, user_pt_t *dst);

/* Destroys user memory for the specified user page table,
 * vacating all user frames
 *
 * @param userpt the user page table to vacate
 */
void destroy_usermem(user_pt_t *userpt);

/* Destroies the kernel stack specified, vacating
 * all kstack frames.
 * Cannot and must not be done in the process that
 * owns this kernel stack
 *
 * @param kstack the kstack page table to vacate
 */
void destroy_kstack(kernel_stack_pt_t* kstack);

/* Checks the specified addr in specified user page table
 * and returns whether the addr has the specified prot
 * protection (can have more protections)
 *
 * @param addr the address to check
 * @param prot the desired protections to check
 * @param curr_pt the user page table to check
 * @return 1 if the addr has specified prot protection , 0 if invalid address
 */
int check_addr(void *addr, int prot, user_pt_t* curr_pt);

/* Check the buffer
 * 
 * @param len
 * @param addr
 * @param prot
 * @param curr_pt
 * @return 1 on success, 0 otherwise
 */
int check_buffer(int len, void *addr, int prot, user_pt_t* curr_pt);

/* Check the string
 *
 * @param addr the addr to check
 * @param curr_pt the user page table to check
 * @return 1 on sucesss, 0 otherwise
 */
int check_string(char* addr, user_pt_t* curr_pt);

/* Check the args
 *
 * @param args
 * @param curr_pt the user page table to check
 * @return 1 on success, 0 otherwise
 */
int check_args(char** args, user_pt_t* curr_pt);

/* Returns whether there are 'left' enough free frames and 
 * if the kbrk hasn't reached the kstack yet
 *
 * @param left the # of frames to check if theres enough
 * @return 1 if not enough kernel memory, 0 if enough
 */
int no_kernel_memory(int left);

/* Returns the # of free frames left
 *
 * @return the # of free frames left
 */
int frames_left(void);

#endif //__MEMORY_H
