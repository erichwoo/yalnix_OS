/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "syscall.h"
#include <ykernel.h>
#include "macro.h"
#include "process.h"
#include "kmem.h"

/************ Data Structs *********/

typedef struct lock {
  int id;
  // 0/1 locked or not
  // queue of processes that want to acquire lock
} lock_t;

typedef struct cond_var {
  int id;
  // queue of waiting processes waiting for signal
} cond_t;

typedef struct pipe {
  int id;
  // buffer pointing to memory
  lock_t lock;
  // queue of blocked readers
  // queue of blocked writers
} pipe_t;

typedef void (*trap_handler_t) (UserContext* uc); // defining an arbitrary trap handler function

/************ Kernel Global Data **************/
trap_handler_t trap_vector[TRAP_VECTOR_SIZE]; // array of pointers to trap handler functs
proc_table_t *proc_table = NULL;
free_frame_t free_frame = {0, NULL, BASE_FRAME, 0};
kernel_global_pt_t kernel_pt;
void *kernel_brk = NULL; // to be modified by SetKernelBrk

/*********************** Functions ***********************/
void VM_setup(user_pt_t *init_user_pt, kernel_stack_pt_t *init_kstack_pt) {
  // write stuff
  WriteRegister(REG_PTBR0, (unsigned int) kernel_pt.pt);
  WriteRegister(REG_PTLR0, (unsigned int) NUM_PAGES_0);
  WriteRegister(REG_PTBR1, (unsigned int) init_user_pt->pt);
  WriteRegister(REG_PTLR1, (unsigned int) NUM_PAGES_1);
  // fill in the pagetable so that vpn = pfn
  // kernel pt

  // TODO: change code section to protected
  for (int vpn = BASE_PAGE_0; vpn < LIM_PAGE_0; vpn++) {
    if (vpn < (unsigned int) _kernel_data_start >> PAGESHIFT) {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(vpn, FIXED), PROT_READ|PROT_EXEC);
    } else if (vpn <= (unsigned int) kernel_brk >> PAGESHIFT) {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(vpn, FIXED), PROT_READ|PROT_WRITE);
    } else if (vpn >= BASE_PAGE_KSTACK) {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 1, get_frame(vpn, FIXED), PROT_READ|PROT_WRITE);
      init_kstack_pt->pt[vpn - BASE_PAGE_KSTACK] = kernel_pt.pt[vpn - BASE_PAGE_0];
    } else {
      set_pte(&kernel_pt.pt[vpn - BASE_PAGE_0], 0, NONE, NONE);
    }
  }
  // user pt                                                                                           
  for (int vpn = BASE_PAGE_1; vpn < LIM_PAGE_1; vpn++) {
    set_pte(&init_user_pt->pt[vpn - BASE_PAGE_1], 0, NONE, NONE);
  }
  unsigned int usr_stack_vpn = LIM_PAGE_1 - 1;
  set_pte(&init_user_pt->pt[usr_stack_vpn - BASE_PAGE_1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
  //set_pte(&init_user_pt->pt[usr_stack_vpn - BASE_PAGE_1 - 1], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
  //init_user_pt->stack_low = (void *)((unsigned int) DOWN_TO_PAGE(VMEM_1_LIMIT - 1)); // have to point to somewhere lower than the top
  init_user_pt->stack_low = (void *)((unsigned int) VMEM_1_LIMIT - 4);

  WriteRegister(REG_VM_ENABLE, 1);
}

void trap_setup(void) {
  // hookup trap handler funct pointers to the handler table
  trap_vector[TRAP_KERNEL] = TrapKernel;
  trap_vector[TRAP_CLOCK] = TrapClock;
  /*
  trap_vector[TRAP_ILLEGAL] = TrapIllegal;
  trap_vector[TRAP_MEMORY] = TrapMemory;
  trap_vector[TRAP_MATH] = TrapMath;
  trap_vector[TRAP_TTY_RECEIVE] = TrapTtyReceive;
  trap_vector[TRAP_TTY_TRANSMIT] = TrapTtyTransmit;
  trap_vector[TRAP_DISK] = TrapDisk;
  */
  // temporarily map rest of traps to TrapTemp until we handle them
  int rest_of_traps[] = {TRAP_ILLEGAL, TRAP_MEMORY, TRAP_MATH, TRAP_TTY_RECEIVE, TRAP_TTY_TRANSMIT, TRAP_DISK};
  
  for (int i = 0; i < 6; i++)
    trap_vector[rest_of_traps[i]] = TrapTemp;

  // NULL the remaining spaces in handler table (8-15)
  for (int null_trap = TRAP_DISK + 1; null_trap < TRAP_VECTOR_SIZE; null_trap++)
    trap_vector[null_trap] = NULL;

  // write handler table to register
  WriteRegister(REG_VECTOR_BASE, (unsigned int) trap_vector);
}

void PCB_setup(int ppid, user_pt_t* user_pt, kernel_stack_pt_t* k_stack_pt, UserContext* uc) {
  // initialize pcb struct and assign values
  pcb_t* pcb = (pcb_t*) malloc(sizeof(pcb_t));
  pcb->pid = helper_new_pid(user_pt->pt);
  pcb->ppid = ppid;
  pcb->state = READY;
  pcb->exit_status = 0; // will be changed if process error or return value needed
  pcb->reg1 = user_pt;
  pcb->k_stack = k_stack_pt;
  pcb->uc = uc;
  // add to proc_table here
  // for now since proc_table structure isn't decided, will add to head.
  proc_table->head = pcb;
  //return pcb->pid;
}

void DoIdle(void) {
  while(1) {
    TracePrintf(1,"DoIdle\n");
    Pause();
  }
}

void idle_setup(void) {
  pcb_t* idle = proc_table->head;
  idle->uc->pc = DoIdle; // point to doIdle();
  idle->uc->sp = idle->reg1->stack_low; // hook up uc stack pointer to top of user stack
  //TracePrintf(1,"sp: 0x%08X\n", idle->reg1->stack_low);
}

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt) {
  // initialize vital global data structures
  kernel_brk =  _kernel_orig_brk; // first thing first
  
  free_frame.size = pmem_size / PAGESIZE;
  free_frame.bit_vector = malloc(free_frame.size / CELL_SIZE + 1);
  for (int i = 0; i < free_frame.size / CELL_SIZE + 1; i++) free_frame.bit_vector[i] = (char) 0; // initialzied
  
  user_pt_t *init_user_pt = malloc(sizeof(user_pt_t));
  kernel_stack_pt_t *init_kstack_pt = malloc(sizeof(kernel_stack_pt_t));

  proc_table = (proc_table_t*) malloc(sizeof(proc_table_t));

  VM_setup(init_user_pt, init_kstack_pt); // set up free frame tracker, set up region 1 page table and save it, set up kernel page table, fill pt registers, turn on vm (these are all global variables)
  trap_setup(); // set up trap handlers
  PCB_setup(-1, init_user_pt, init_kstack_pt, uctxt); // set up PCB for the first process. ppid = -1 as kernel is first process
  idle_setup(); // manipulate UserContext
  TracePrintf(1,"Leaving Kstart\n");
  // idle begins when KernelStart returns
}
