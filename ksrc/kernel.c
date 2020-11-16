#include <ykernel.h>
#include "linked_list.h"
#include "memory.h"
#include "process.h"
#include "traps.h"
#include "scheduling.h"
#include "misc.h"
#include "pilocvario.h"


trap_handler_t trap_vector[TRAP_VECTOR_SIZE]; // array of pointers to trap handler functs
proc_table_t *procs;
free_frame_t free_frame;
kernel_global_pt_t kernel_pt;
node_t *init_node = NULL, *idle_node = NULL;
io_control_t *io;
pilocvar_t *pilocvar;

/*********************** Functions ***********************/
void VM_setup(void) {
  // write stuff

  user_pt_t *bogus = new_user_pt();
  WriteRegister(REG_PTBR0, (unsigned int) kernel_pt.pt);
  WriteRegister(REG_PTLR0, (unsigned int) NUM_PAGES_0);
  WriteRegister(REG_PTBR1, (unsigned int) bogus->pt);
  WriteRegister(REG_PTLR1, (unsigned int) NUM_PAGES_1);
  // fill in the pagetable so that vpn = pfn

  for (int vpn = BASE_PAGE_0; vpn < LIM_PAGE_0; vpn++) {
    if (vpn < (unsigned int) _kernel_data_start >> PAGESHIFT) {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(vpn, FIXED), PROT_READ|PROT_EXEC);
    } else if (vpn <= (unsigned int) kernel_pt.brk >> PAGESHIFT) {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(vpn, FIXED), PROT_READ|PROT_WRITE);
    } else if (vpn >= BASE_PAGE_KSTACK) {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(vpn, FIXED), PROT_READ|PROT_WRITE);
    } else {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 0, NONE, NONE);
    }
  }
  WriteRegister(REG_VM_ENABLE, 1);
}

void trap_setup(void) {
  // hookup trap handler funct pointers to the handler table
  trap_vector[TRAP_KERNEL] = TrapKernel;
  trap_vector[TRAP_CLOCK] = TrapClock;
  trap_vector[TRAP_TTY_RECEIVE] = TrapTtyReceive;
  trap_vector[TRAP_TTY_TRANSMIT] = TrapTtyTransmit;
  trap_vector[TRAP_ILLEGAL] = TrapIllegal;
  trap_vector[TRAP_MEMORY] = TrapMemory;
  trap_vector[TRAP_MATH] = TrapMath;
  trap_vector[TRAP_DISK] = TrapDisk;

  // NULL the remaining spaces in handler table (8-15)
  for (int null_trap = TRAP_DISK + 1; null_trap < TRAP_VECTOR_SIZE; null_trap++)
    trap_vector[null_trap] = NULL;

  // write handler table to register
  WriteRegister(REG_VECTOR_BASE, (unsigned int) trap_vector);
}

void DoIdle(void) {
  while(1) {
    TracePrintf(1,"DoIdle\n");
    Pause();
  }
}

void idle_setup(UserContext* uctxt) {
  idle_node = process_init();
  pcb_t *idle_pcb = idle_node->data; 

  idle_pcb->uc = *uctxt; // cp usercontext

  unsigned int usr_stack_vpn = LIM_PAGE_1 - 1;
  set_pte(&idle_pcb->userpt->pt[usr_stack_vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE); // assign one page

  idle_pcb->uc.pc = DoIdle; // point to doIdle();
  idle_pcb->uc.sp = (void *)((unsigned int) VMEM_1_LIMIT - 4); // hook up uc stack pointer to top of user stack
  copy_kernel(idle_node);
}

void init_load(char *name, char *args[], UserContext *uctxt) {
  init_node = process_init();
  pcb_t *init_pcb = init_node->data;

  init_pcb->uc = *uctxt;

  for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) { // create copy of kernel stack mapping
    init_pcb->kstack->pt[vpn - BASE_PAGE_KSTACK] = kernel_pt.pt[vpn - BASE_PAGE_0];
  }

  WriteRegister(REG_PTBR1, (unsigned int) init_pcb->userpt->pt); // 
  LoadProgram(name, args, init_pcb); 
  procs->running = init_node; // set manually
} 

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
  // initialize vital global data structures
  
  // Memory
  kernel_pt.brk = _kernel_orig_brk; // first thing first
  free_frame.size = pmem_size / PAGESIZE;
  free_frame.avail_pfn = BASE_FRAME;
  free_frame.filled = 0;
  free_frame.bit_vector = malloc(free_frame.size / CELL_SIZE + 1);
  for (int i = 0; i < free_frame.size / CELL_SIZE + 1; i++) free_frame.bit_vector[i] = (char) 0; // initialzied
  
  VM_setup();
  // Traps
  trap_setup();
  // Process control
  procs = proc_table_init();
  // Terminal IO
  io = io_control_init();
  // Pipes, Locks, Cvars
  pilocvar = pilocvar_init();
  
  //!!! cmdline args checking here
  init_load(cmd_args[0], cmd_args, uctxt);

  idle_setup(uctxt); 
  if (procs->running == init_node) TracePrintf(1, "Leaving KStart\n");
  //*uctxt = ((pcb_t *)init_node->data)->uc;
  restore_uc(uctxt); // by design choice both init and idle would go through here.  
}
