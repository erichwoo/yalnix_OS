/* Erich Woo & Boxian Wang
 * 23 October 2020
 * Kernel Memory Management. See memory.c for detailed documentation
 */

#include "memory.h"

extern kernel_global_pt_t kernel_pt;
extern free_frame_t free_frame;

/*********************** Functions ***********************/

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
  //TracePrintf(1, "freed frame %d\n", pfn);
  return 0;
}

int get_frame(unsigned int pfn, int auto_assign) { 
  if (free_frame.filled >= free_frame.size) return ERROR;
  if (auto_assign)
    pfn = free_frame.avail_pfn;
  
  set_bit(free_frame.bit_vector, pfn - BASE_FRAME, 1);
  free_frame.filled++;

  // find next free 
  if (pfn == free_frame.avail_pfn) {
    int next_pfn;
    for (next_pfn = pfn; get_bit(free_frame.bit_vector, next_pfn - BASE_FRAME) && 
      next_pfn - BASE_FRAME < free_frame.size; next_pfn++);
    free_frame.avail_pfn = next_pfn;
  }
  // uncomment for frame tracking
  /*TracePrintf(1, "Got frame %d\n", pfn);
  TracePrintf(1, "Next free: %d\n", free_frame.avail_pfn);
  TracePrintf(1, "left: %d\n", free_frame.size - free_frame.filled);
  */
  return pfn;
}

void set_pte(pte_t *pte, int valid, int pfn, int prot) {
  if (!(pte->valid = valid)) return; // turn off valid bit, others don't matter
  pte->pfn = pfn;
  pte->prot = prot;
}

int SetKernelBrk(void *addr) {
  if ((unsigned int) addr >= DOWN_TO_PAGE(KERNEL_STACK_BASE) - PAGESIZE ||
    (unsigned int) addr < UP_TO_PAGE(_kernel_data_end)) return ERROR;

  if (ReadRegister(REG_VM_ENABLE)) {
    unsigned int curr_brk_vpn = (UP_TO_PAGE(kernel_pt.brk) >> PAGESHIFT) - 1; // greatest vpn in use
    unsigned int next_brk_vpn = (UP_TO_PAGE(addr) >> PAGESHIFT) - 1; // greatest vpn to be used
    if (next_brk_vpn > curr_brk_vpn) {
      for (int vpn = curr_brk_vpn + 1; vpn <= next_brk_vpn; vpn++) {
        set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
        WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
      }
    } else if (next_brk_vpn < curr_brk_vpn) {
      // freeing frames
      for (int vpn = curr_brk_vpn; vpn > next_brk_vpn; vpn--) {
        set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 0, vacate_frame(kernel_pt.pt[vpn - BASE_PAGE_0].pfn), NONE);
        WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
      }
    }
  }
  kernel_pt.brk = (void *) UP_TO_PAGE(addr);
  return 0;
}

user_pt_t *new_user_pt(void) {
  user_pt_t *new = malloc(sizeof(user_pt_t));
  for (int vpn = BASE_PAGE_1; vpn < LIM_PAGE_1; vpn++) {
    set_pte(&new->pt[vpn - BASE_PAGE_1], 0, NONE, NONE);
  }
  new->size = 0;
  return new;
}

void copy_user_mem(user_pt_t *origin, user_pt_t *dst) {
  int dummy = BASE_PAGE_KSTACK - 1;
  for (int vpn = BASE_PAGE_1; vpn < LIM_PAGE_1; vpn++) {
      if (origin->pt[vpn-BASE_PAGE_1].valid) {
        int privilege = origin->pt[vpn-BASE_PAGE_1].prot;
        set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
        WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT);
        memcpy((void*) (dummy << PAGESHIFT), (void*) (vpn << PAGESHIFT), PAGESIZE);
        dst->pt[vpn - BASE_PAGE_1] = kernel_pt.pt[dummy - BASE_PAGE_0];
        dst->pt[vpn - BASE_PAGE_1].prot = privilege;
      }
  }
  set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 0, NONE, NONE);
  WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT);
  dst->brk = origin->brk;
  dst->data_end = origin->data_end;
  dst->stack_low = origin->stack_low;
}

// vacate all user frames
void destroy_usermem(user_pt_t *userpt) {
  for (int vpn = BASE_PAGE_1; vpn < LIM_PAGE_1; vpn++) {
    if (userpt->pt[vpn-BASE_PAGE_1].valid) {
      set_pte(&userpt->pt[vpn - BASE_PAGE_1], 0, vacate_frame(userpt->pt[vpn - BASE_PAGE_1].pfn), NONE);
    }
  } 
}

void destroy_kstack(kernel_stack_pt_t* kstack) { // don't do this in the same process!
  for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) {
    if (kstack->pt[vpn - BASE_PAGE_KSTACK].valid) {
      set_pte(&kstack->pt[vpn - BASE_PAGE_KSTACK], 0, vacate_frame(kstack->pt[vpn - BASE_PAGE_KSTACK].pfn), NONE);
    }
  }
}

int check_addr(void *addr, int prot, user_pt_t* curr_pt) {
  if ((unsigned int) addr < VMEM_1_BASE || (unsigned int) addr >= VMEM_1_LIMIT) return 0; // dont touch kernel!
  pte_t p = curr_pt->pt[((unsigned int)addr >> PAGESHIFT) - BASE_PAGE_1];
  if (!p.valid) return 0; // must be valid
  if ((p.prot == PROT_READ|PROT_WRITE) || (p.prot == PROT_ALL)) return 1;
  if (prot == PROT_READ) {
    return (p.prot == PROT_READ);
  } else {
    return (p.prot == PROT_WRITE);
  }
}

int check_buffer(int len, void *addr, int prot, user_pt_t* curr_pt) {
  for (int i = 0; i < len; i++) if (!check_addr((char * ) addr + i, prot, curr_pt)) return 0;
  return 1;
}

int check_string(char* addr, user_pt_t* curr_pt) {
  for (int i = 0; i < MAX_CHECK; i++) {
    if (!check_addr(addr + i, PROT_READ, curr_pt)) return 0;
    if (*(addr + i) == '\0') return 1; // check complete
  }
  return 0; // exceeded max len
}

int check_args(char** args, user_pt_t* curr_pt) {
  for (int i = 0; i < MAX_CHECK; i++) {
    if (!check_addr(args + i, PROT_READ, curr_pt)) return 0; // fisrt check if char* is stored at a good location
    if (*(args + i) == NULL) return 1; // check complete
    if (!check_string(*(args + i), curr_pt)) return 0; // then check the string char* points to
  }
  return 0;
}

int no_kernel_memory(int left) {
  return (frames_left() < left || (unsigned int) kernel_pt.brk >= DOWN_TO_PAGE(KERNEL_STACK_BASE) - PAGESIZE);
}

int frames_left(void) { 
  return free_frame.size - free_frame.filled;
}
