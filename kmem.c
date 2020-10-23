/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */


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



extern free_frame_t free_frame;
extern kernel_global_pt_t kernel_pt;
extern void *kernel_brk; // to be modified by SetKernelBrk

int SetKernelBrk(void *addr);


// Kernel Context Switching

/*********************** Functions ***********************/
void set_pte(pte_t *pte, int valid, int pfn, int prot) {
  if (!(pte->valid = valid)) return; // turn off valid bit, others don't matter
  pte->pfn = pfn;
  pte->prot = prot;
}

int get_bit(unsigned char *bit_array, int index) {
  int cell = index / CELL_SIZE;
  int offset = index % CELL_SIZE;
  // added extra parens for & operation
  return ((bit_array[cell] & (1 << offset)) == 0? 0: 1); 
}

void set_bit(unsigned char *bit_array, int index, int bit) {
  int cell = index / CELL_SIZE;
  int offset = index % CELL_SIZE;
  if (bit) {
    bit_array[cell] |= (1 << offset);
  } else {
    bit_array[cell] &= ~(1 << offset);
  }
}

int vacate_frame(unsigned int pfn) { // mark pfn as free
  set_bit(free_frame.bit_vector, pfn - BASE_FRAME, 0);
  free_frame.filled--;
  if (pfn < free_frame.avail_pfn) free_frame.avail_pfn = pfn;
  TracePrintf(1, "freed frame %d\n", pfn);
  return pfn;
}

int get_frame(unsigned int pfn, int auto_assign) { // find a free physical frame, mark as occupied, returns pfn
  if (free_frame.filled >= free_frame.size) return ERROR;
  if (auto_assign)
    pfn = free_frame.avail_pfn;
  
  set_bit(free_frame.bit_vector, pfn - BASE_FRAME, 1);
  free_frame.filled++;
  //TracePrintf(1, "%d\n", (unsigned int) free_frame.bit_vector[0]);

  // find next free 
  if (pfn == free_frame.avail_pfn) {
    int n_pfn;
    for (n_pfn = pfn; get_bit(free_frame.bit_vector, n_pfn - BASE_FRAME) && 
      n_pfn  - BASE_FRAME < free_frame.size; n_pfn++);
    free_frame.avail_pfn = n_pfn;
  }
  //TracePrintf(1, "Got frame %d\n", pfn);
  //TracePrintf(1, "Next: %d\n", free_frame.avail_pfn);
  return pfn;
}

int SetKernelBrk(void *addr) {
  if ((unsigned int) addr >= DOWN_TO_PAGE(KERNEL_STACK_BASE) ||
    (unsigned int) addr <= UP_TO_PAGE((unsigned int) _kernel_data_end - 1)) return ERROR;

  if (ReadRegister(REG_VM_ENABLE)) {
    unsigned int curr_brk_vpn = (unsigned int) kernel_brk >> PAGESHIFT;
    unsigned int next_brk_vpn = (unsigned int) addr >> PAGESHIFT;
    if (next_brk_vpn > curr_brk_vpn) {
      for (int vpn = curr_brk_vpn + 1; vpn <= next_brk_vpn; vpn++) {
        set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
      }
    } else if (next_brk_vpn < curr_brk_vpn) {
      // theoretically doesn't have to do anything, but freeing frames nonetheless
      for (int vpn = curr_brk_vpn; vpn > next_brk_vpn; vpn--) {
        set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 0, vacate_frame(kernel_pt.pt[vpn - BASE_PAGE_0].pfn), NONE);
      }
    }
  } 
  kernel_brk = addr;
  return 0;
}



