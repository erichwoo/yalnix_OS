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

#define TERMINATED -1
#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define DEFUNCT 3

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

typedef struct pcb {
  int pid;
  int ppid;
  // possibly pcb_t* children?                                                               
  int state;
  int exit_status;
  // address space                                                                           
  user_pt_t *reg1; // region 1 page table management                                         
  kernel_stack_pt_t *k_stack; // copy of kernel stack page table                             

  UserContext *uc;
  KernelContext *kc;

} pcb_t;

typedef struct proc_table { // maybe a queue?                                                
  int size; // number of current process under management                                    
  pcb_t *head; //pointer to the head of the queue                                            
} proc_table_t;

// need some queue data struct to manage incoming pipe/lock/cond_var users
// could possibly help with pcb management with proc_table
typedef struct queue {
  
} queue_t;

typedef struct pipe {
  int id;
  // buffer pointing to memory
  lock_t lock;
  // queue of blocked readers
  // queue of blocked writers
} pipe_t;
  
typedef struct lock {
  int id;
  // 0/1 locked or not
  // queue of processes that want to acquire lock
} lock_t;

typedef struct cond_var {
  int id;
  // queue of waiting processes waiting for signal
} cond_t;

typedef void (*trap_handler_t) (UserContext* uc); // defining an arbitrary trap handler func\
tion                                     

/************ Kernel Global Data **************/
trap_handler_t trap_vector[TRAP_VECTOR_SIZE]; // array of pointers to trap handler functs    
proc_table_t *proc_table = NULL;
free_frame_t free_frame = {0, NULL, BASE_FRAME, 0};
kernel_global_pt_t kernel_pt;
void *kernel_brk = NULL; // to be modified by SetKernelBrk

/*********** Function Prototypes and Commented Pseudocode *********/

// Possibly useful functions:

// find one free frame each time ?
//pte_t FindFreeFrame(free_frame_vec_t* vec);

// VM setup, doing gymnastics to set up VM, which should be enabled when it returns
/*
1. set up free_frame_vec
2. set up region 0 page table (assign full space for stack?)
3. turn on VM (do we need to care about region 1 at this point?)
*/
//void VM_setup();

// configure the first pcb during boot time
// for both idle and init?
/*
1. Create proc_table
2. create and add pcb
3. setup region 1
4. Configure UserContext
5. Somehow return to usermode? (how to do that??? can kernel trap itself? if not where to store the UserContext
so the hardware knows? is it uctxt?)
*/
//void PCB_setup();

/////////////// Basic Process Coordination

/* Create a new child pcb_t
 * assign new pid and its ppid
 * Copy region 1 page table, uc, and kc
 * state = ready
 * get new frames for kernel stack
 * copy region 0, change stack part to map to new frames above
 * return (not sure how to get one return pid, other 0. general regs in UC?)
 */
int KernelFork (void);

/* Load Program with exec args and pcb_t  */
int KernelExec (char *filename, char **argvec);

/* assign status & state = zombie
 * free anything malloc'd in pcb. 
 * if parent is dead(not in proc table), remove from proc table
 * otherwise, leave it in proc table 
 */
void KernelExit (int status);

/* Loop through children, removing dead children along the way
 * if there is zombie, return zombie pid and edit status_ptr if nonNULL
 * if no zombie but still alive children, change state = block. 
 *      - loop again after some time? or somehow a child signals its exit
 * if no/all-dead children, return error
 */
int KernelWait (int *status_ptr);

/* Return pid from current pcb*/
int KernelGetPid (void);

/* Check if heap will shrink below to user data or stack or invalid, error if so
 * add or remove page table entries depending on if addr is above or below current brk
 */
int KernelBrk (void *addr);

/* clock_ticks < 1 error checked
 * status = block, track hardware clock trap, after x time
 */
int KernelDelay (int clock_ticks);

////////////// I/O Syscalls

/* Check if TtyReceive has extra chars
 * if not, state = block and wait for TrapTtyReceive to come in
 * Once there are lines to receive, Collect TrapTtyReceive
 * Validate buf based on len, return whatever necessary
 *   - THought: should Trap handler call TtyRecieve or this funct
 */
int KernelTtyRead (int tty_id, void *buf, int len);

/* Write with hardware funct TtyTransmit
 * Validate error, and return whatever necessary
 */
int KernelTtyWrite (int tty_id, void *buf, int len);

//////////////// IPC Syscalls

/* Initialize pipe_t with id, lock, queues*/
int KernelPipeInit (int *pipe_idp);

/* Check if someone is reading/writing, block if so
 * check if there are bytes to read from buffer, if not return.
 * if so, acquire lock
 * put pipe's contents into param buf
 * release lock
 */
int KernelPipeRead (int pipe_id, void *buf, int len);

/* Check if someone is reading/writing, block if so
 * acquire lock, write to pipe buffer (may need to reallocate buffer)
 * release lock
 */
int KernelPipeWrite (int pipe_id, void *buf, int len);

//////////// Synchronization Syscalls

/* initialize new lock_t with its id, locked = 0, initialize queue */
int KernelLockInit (int *lock_idp);

/* if lock is locked
 *      - add process to lock's queue, state = block
 * if not
 *      - make lock locked, take process off queue
 */
int KernelAcquire (int lock_id);

/* make lock variable 0, potentially signal the queue  */
int KernelRelease (int lock_id);

/* Initialize new cond_t with id*/
int KernelCvarInit (int *cvar_idp);

/* Add process to cond_var's queue, state = block
 * return once signaled
 */
int KernelCvarWait (int cvar_id, int lock_id);

/* Signal the next process on cond_var queue */
int KernelCvarSignal (int cvar_id);

/* Signal all the processes on cond_var queue */
int KernelCvarBroadcast (int cvar_id);

/* free and remove all memory in the pipe/lock/condvar of the given id*/
int KernelReclaim (int id);

//////////////// Traps

// Examine the "code" field of user context and decide which syscall to invoke
void TrapKernel(UserContext *uc);
// Check the process table to decide which process to schedule; initialize a context switch if necessary
void TrapClock(UserContext *uc);
// Abort current process, switch context
void TrapIllegal(UserContext *uc);
// Check the page table; if illegal access, abort; otherwise, modify page table to reflect memory allocation
void TrapMemory(UserContext *uc);
// Abort
void TrapMath(UserContext *uc);
// Read input and store in buffer; set a flag that wakes up blocked process waiting for input
// allocate kheap with buf for len (how do we know how long input line is?)
// use TtyReceive() hardware function to collect into buf
void TrapTtyReceive(UserContext *uc);
// Start next transmission; resumes blocked process
void TrapTtyTransmit(UserContext *uc);
// TBD
void TrapDisk(UserContext *uc);

// Modify kernel page table:
// The kernel keeps a separate copy of each process's kernel stack page table,
// and copy them when switching context.
// The bottom half of the page table shall remain the same,
// only to be touched by SetKernelBrk.
// This means all process's will be given the same region0 PTBR?
int SetKernelBrk (void *addr);

/*
1. call VM_setup to set up virtual memory
2. set up trap table
3. call PCB_setup to configure idle
*/
void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

// Kernel Context Switching

KernelContext* KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);

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

int get_frame(unsigned int pfn, int auto_assign) { // find a free physical frame, returns pfn
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
  if ((unsigned int) addr >= DOWN_TO_PAGE(KERNEL_STACK_BASE) ||(unsigned int) addr <= UP_TO_PAGE((unsigned int) _kernel_data_end - 1)) return ERROR;

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

// Examine the "code" field of user context and decide which syscall to invoke
void TrapKernel(UserContext *uc) {
  TracePrintf(1, "The code of syscall is 0x%x\n", uc->code);
}
// Check the process table to decide which process to schedule; initialize a context switch if necessary
void TrapClock(UserContext *uc) {
  TracePrintf(1, "Clock Trap occured!\n");
}
// temporary trap handler funct for all the unhandled traps.
void TrapTemp(UserContext *uc) {
  TracePrintf(1, "This trap type %d is currently unhandled\n", uc->vector);
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
  int i;
  for (i = 0; i < 6; i++)
    trap_vector[rest_of_traps[i]] = TrapTemp;

  // NULL the remaining spaces in handler table (8-15)
  int null_trap;
  for (null_trap = TRAP_DISK + 1; null_trap < TRAP_VECTOR_SIZE; null_trap++)
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
