/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Kernel Memory Management
 */

#include "kmem.h"

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
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
  return 0;
}
