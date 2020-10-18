/* Erich Woo & Boxian Wang
 * 10 October 2020
 * Kernel functionality
 */

#include "ctype.h"
#include "filesystem.h"
#include "hardware.h"
#include "load_info.h"
#include "yalnix.h"
#include "ykernel.h"
#include "ylib.h"
#include "yuser.h"

#define NUM_PAGES_1 (VMEM_1_SIZE / PAGESIZE)
#define NUM_PAGES_0 (VMEM_0_SIZE / PAGESIZE)
#define NUM_KSTACK_PAGES (KERNEL_STACK_MAXSIZE / PAGESIZE)
<<<<<<< HEAD
#define BASE_PAGE_0 (VMEM_0_BASE >> PAGESHIFT) // starting page num of reg 0
#define LIM_PAGE_0 (VMEM_0_LIMIT >> PAGESHIFT) // first page above reg 0 
#define BASE_PAGE_1 (VMEM_1_BASE >> PAGESHIFT) // starting page num of reg 1
#define LIM_PAGE_1 (VMEM_1_LIMIT >> PAGESHIFT) // first page above reg 1 
#define BASE_KSTACK (KERNEL_STACK_BASE >> PAGESHIFT)
#define LIM_KSTACK (KERNEL_STACK_LIMIT >> PAGESHIFT)
#define BASE_FRAME (PMEM_BASE >> PAGESHIFT)
=======
#define TERMINATED -1
#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define DEFUNCT 3

>>>>>>> 29f063419030c724a0fde86b65a8c8078ea2a1b2
/************ Data Structs *********/

typedef struct pcb {
  int pid;
  int ppid;
  // possibly pcb_t* children?
  int state;
  int exit_status;
  // address space
  reg1_pt_t *reg1; // region 1 page table management
  kernel_stack_pt_t *k_stack; // copy of kernel stack page table

  UserContext *uc;
  KernelContext *kc;

} pcb_t; 
  
typedef struct proc_table { // maybe a queue?
  int size; // number of current process under management
  pcb_t *head; //pointer to the head of the queue
} proc_table_t;

typedef struct f_frame{
// tracking which frames in physical are free
  unsigned int size; // available number of physical frames
  char *bit_vector // pointer to a bit vector 
  unsigned int avail_pfn; // next available pfn
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

/************ Kernel Global Data **************/
void *trap_handlers[TRAP VECTOR SIZE]; // interrupt_handlers 
proc_table_t *proc_table = {0, NULL};
free_frame_t free_frame = {0, NULL, 0};
kernel_global_pt_t kernel_pt;
<<<<<<< HEAD
void *kernel_brk = NULL; // to be modified by SetKernelBrk
void *kernel_data_end = NULL;

=======
void *kernel_brk; // to be modified by SetKernelBrk
>>>>>>> 29f063419030c724a0fde86b65a8c8078ea2a1b2

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

/*********** Function Prototypes and Commented Pseudocode *********/

// Possibly useful functions:

// find one free frame each time ?
pte_t FindFreeFrame(free_frame_vec_t* vec);

// VM setup, doing gymnastics to set up VM, which should be enabled when it returns
/*
1. set up free_frame_vec
2. set up region 0 page table (assign full space for stack?)
3. turn on VM (do we need to care about region 1 at this point?)
*/
void VM_setup();

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
void PCB_setup();

/////////////// Basic Process Coordination

/* Create a new child pcb_t
 * assign new pid and its ppid
 * Copy region 1 page table, uc, and kc
 * state = ready
 * get new frames for kernel stack
 * copy region 0, change stack part to map to new frames above
 * return (not sure how to get one return pid, other 0. general regs in UC?)
 */
int Fork (void);

/* Load Program with exec args and pcb_t  */
int Exec (char *, char **);

/* assign status & state = zombie
 * free anything malloc'd in pcb. 
 * if parent is dead(not in proc table), remove from proc table
 * otherwise, leave it in proc table 
 */
void Exit (int);

/* Loop through children, removing dead children along the way
 * if there is zombie, return zombie pid and edit status_ptr if nonNULL
 * if no zombie but still alive children, change state = block. 
 *      - loop again after some time? or somehow a child signals its exit
 * if no/all-dead children, return error
 */
int Wait (int *);

/* Return pid from current pcb*/
int GetPid (void);

/* Check if heap will shrink below to user data or stack or invalid, error if so
 * add or remove page table entries depending on if addr is above or below current brk
 */
int Brk (void *);

/* clock_ticks < 1 error checked
 * status = block, track hardware clock trap, after x time
 */
int Delay (int);

////////////// I/O Syscalls

/* Check if TtyReceive has extra chars
 * if not, state = block and wait for TrapTtyReceive to come in
 * Once there are lines to receive, Collect TrapTtyReceive
 * Validate buf based on len, return whatever necessary
 *   - THought: should Trap handler call TtyRecieve or this funct
 */
int TtyRead (int, void *, int);

/* Write with hardware funct TtyTransmit
 * Validate error, and return whatever necessary
 */
int TtyWrite (int, void *, int);

//////////////// IPC Syscalls

/* Initialize pipe_t with id, lock, queues*/
int PipeInit (int *);

/* Check if someone is reading/writing, block if so
 * check if there are bytes to read from buffer, if not return.
 * if so, acquire lock
 * put pipe's contents into param buf
 * release lock
 */
int PipeRead (int, void *, int);

/* Check if someone is reading/writing, block if so
 * acquire lock, write to pipe buffer (may need to reallocate buffer)
 * release lock
 */
int PipeWrite (int, void *, int);

//////////// Synchronization Syscalls

/* initialize new lock_t with its id, locked = 0, initialize queue */
int LockInit (int *);

/* if lock is locked
 *      - add process to lock's queue, state = block
 * if not
 *      - make lock locked, take process off queue
 */
int Acquire (int);

/* make lock variable 0, potentially signal the queue  */
int Release (int);

/* Initialize new cond_t with id*/
int CvarInit (int *);

/* Add process to cond_var's queue, state = block
 * return once signaled
 */
int CvarWait (int, int);

/* Signal the next process on cond_var queue */
int CvarSignal (int);

/* Signal all the processes on cond_var queue */
int CvarBroadcast (int);

/* free and remove all memory in the pipe/lock/condvar of the given id*/
int Reclaim (int);

//////////////// Traps

// Examine the "code" field of user context and decide which syscall to invoke
void TrapKernel(UserContext *);
// Check the process table to decide which process to schedule; initialize a context switch if necessary
void TrapClock(UserContext *);
// Abort current process, switch context
void TrapIllegal(UserContext *);
// Check the page table; if illegal access, abort; otherwise, modify page table to reflect memory allocation
void TrapMemory(UserContext *);
// Abort
void TrapMath(UserContext *);
// Read input and store in buffer; set a flag that wakes up blocked process waiting for input
// allocate kheap with buf for len (how do we know how long input line is?)
// use TtyReceive() hardware function to collect into buf
void TrapTtyReceive(UserContext *);
// Start next transmission; resumes blocked process
void TrapTtyTransmit(UserContext *);
// TBD
void TrapDisk(UserContext *);


// Modify kernel page table:
// The kernel keeps a separate copy of each process's kernel stack page table,
// and copy them when switching context.
// The bottom half of the page table shall remain the same,
// only to be touched by SetKernelBrk.
// This means all process's will be given the same region0 PTBR?
int SetKernelBrk (void *);

/*
1. call VM_setup to set up virtual memory
2. set up trap table
3. call PCB_setup to configure idle
*/
void KernelStart (char**, unsigned int, UserContext *);

// Kernel Context Switching
KernelContext* KCSwitch(KernelContext*, void*, void*);
KernelContext* KCCopy(KernelContext*, void*, void*);

<<<<<<< HEAD
void set_pte (pte_t *pte, int valid, int pfn, int prot) {
  if (!(pte->valid = valid)) return;
  pte->pfn = pfn;
  pte->prot = prot;
}

int get_bit(char *bit_array, int index) {
  
}

int SetKernelBrk (void *addr) {
  if (addr >= KERNEL_STACK_BASE || addr < kernel_data_end) return -1;

=======
int SetKernelBrk (void * addr) {
  // check if addr is legal
  //...
>>>>>>> 29f063419030c724a0fde86b65a8c8078ea2a1b2
  if (ReadRegister(REG_VM_ENABLE)) {
    // do frame search stuff
    int vpn = kernel_brk >> PAGESHIFT;
    while  (!kernel_pt.pt[addr >> PAGESHIFT - BASE_PAGE_0].valid) {
      kernel_pt.pt[addr >> PAGESHIFT - BASE_PAGE_0].valid = 1;
      kernel_pt.pt[addr >> PAGESHIFT - BASE_PAGE_0].pfn = find_free_frame();
      update_free_frame(kernel_pt.pt[addr >> PAGESHIFT - BASE_PAGE_0].pfn, 1);
    }
  } 
  kernel_brk = addr;
  return 0;
}

void update_free_frame(int pfn, int on) { // on =1 means turn on a bit. = 0 means turn off
  int cell = (pfn - BASE_FRAME) / (sizeof(char) << 3);
  int offset = (pfn - BASE_FRAME) % (sizeof(char) << 3);
  char mask = (char) (1 << offset);
  if (on) {
    free_frame.bit_vector[cell] |=  mask;
  } else {
    free_frame.bit_vector[cell] &=  ~mask;
  }
}

int find_free_frame() { // find a free physical frame, returns pfn
  for (int i = 0; i < free_frame.size; i++) {
    char c = free_frame.bit_vector[i];
    if (c & (^((char) 0)) != ^((char) 0)) {
      for (j = 0; j < sizeof(char); j++) {
        if ((c | (char) (1 << j)) == 0) {
          return BASE_FRAME + i * sizeof(char) + j;
        }
      }
    }
  }
  return -1;
}

// set all valid bit to zero
void init_reg1(user_pt_t* pt1) {
  for (int vpn = BASE_PAGE_1; vpn < LIM_PAGE_1; vpn++) {
    pt1->pt[vpn - BASE_PAGE_1].valid = 0;
  }
}

void VM_setup(void *init_user_pt, void *init_kstack_pt) {
  // write stuff 
  WriteRegister(REG_PTBR0, (unsigned int) kernel_pt.pt);
  WriteRegister(REG_PTLR0, (unsigned int) NUM_PAGES_0);
  WriteRegister(REG_PTBR1, (unsigned int) init_user_pt->pt);
  WriteRegister(REG_PTLR1, (unsigned int) NUM_PAGES_1);
  // fill in the pagetable so that vpn = pfn
  // kernel pt
<<<<<<< HEAD

  // TODO: change code section to protected
  for (int vpn = BASE_PAGE_0; vpn < LIM_PAGE_0; vpn++) {
    if (vpn <= (kernel_brk >> PAGESHIFT) || vpn >= BASE_KSTACK) {
      kernel_pt.pt[vpn - BASE_PAGE_0].pfn = vpn; // pfn = vfn
      kernel_pt.pt[vpn - BASE_PAGE_0].valid = 1;
      kernel_pt.pt[vpn - BASE_PAGE_0].prot = PROT_ALL;
      update_free_frame(vpn, 0) // to be done
      if (vpn >= BASE_KSTACK) {
        init_kstack_pt->pt[vpn - BASE_KSTACK] = kernel_pt.pt[vpn - BASE_PAGE_0];
      }
    } else {
      kernel_pt.pt[vpn - BASE_PAGE_0].valid = 0;
    }
  }
  // user pt
  init_reg1(init_user_pt);
  int stack_page = LIM_PAGE_1 - 1 - BASE_PAGE_1;
  init_user_pt->pt[stack_page].valid = 1;
  init_user_pt->pt[stack_page].prot = PROT_ALL;
  init_user_pt->pt[stack_page].pfn = find_free_frame();
  update_free_frame(init_user_pt->pt[stack_page].pfn);

  WriteRegister(REG_VM_ENABLE, 1)
=======
  for (int vpn = base_page; pnum < lim_page; vpn++) {
    if (vpn <= (kernel_brk >> PAGESHIFT) || vpn >= (KERNEL_STACK_LIMIT >> PAGESHIFT)) {
      kernel_pt.pt[vpn - base_page].pfn = vpn; // pfn = vfn
      kernel_pt.pt[vpn - base_page].valid = 1;
      
      update_free_frame(); // to be done
    }
  }
  // user pt
    //  int ustack_base_vpn = VMEM_1_LIMIT >> PAGESHIFT - 1, 

    //  init_user_pt->pt
}

void trap_setup(void) {
  
}
>>>>>>> 29f063419030c724a0fde86b65a8c8078ea2a1b2

void PCB_setup(int ppid, user_pt_t* user_pt, kernel_stack_pt_t* k_stack_pt, UserContext* uc) {
  // initialize pcb struct and assign values
  pcb_t* pcb = (pcb_t*) malloc(sizeof(pcb_t));
  pcb->pid = helper_new_pid(user_pt);
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

<<<<<<< HEAD
void KernelStart (char**, unsigned int, UserContext *) {
  kernel_brk =  _kernel_orig_brk; // first thing first
  kernel_data_end = _kernel_data_end
  free_frame.bit_vector = malloc(pmem_size / (PAGESIZE * (sizeof(char) << 3)) + 1);
=======
void DoIdle(void) {
  while(1) {
    TracePrintf(1,"DoIdle\n");
    Pause();
  }
}

void idle_setup(void) {
  pcb_t* idle = proc_table->head;
  idle->uc->pc = &DoIdle; // point to doIdle(); hopefully right way to do this
  idle->uc->sp = idle->reg1->stack_low; // hook up uc stack pointer to top of user stack
}

void KernelStart (char *cmd args[], unsigned int pmem size, UserContext *uctxt) {
  kernel_brk =  _kernel_orig_brk; // first thing first
  
  free_frame = malloc(sizeof(free_tracker_t));
  free_frame->bit_vector = malloc(pmem_size / PAGESIZE / (sizeof(char) << 3));
>>>>>>> 29f063419030c724a0fde86b65a8c8078ea2a1b2

  user_pt_t *init_user_pt = malloc(sizeof(user_pt_t));
  kernel_stack_pt_t *init_kstack_pt = malloc(sizeof(kernel_stack_pt_t));

  proc_table = (proc_table_t*) malloc(sizeof(proc_table_t));

  VM_setup(init_user_pt); // set up free frame tracker, set up region 1 page table and save it, set up kernel page table, fill pt registers, turn on vm (these are all global variables)
  trap_setup(); // set up trap handlers
<<<<<<< HEAD
  PCB_setup(init_user_pt, init_kstack_pt); // set up PCB for the first process
  start_idle(); // manipulate UserContext
=======
  PCB_setup(-1, init_user_pt, init_kstack_pt, uctxt); // set up PCB for the first process. ppid = -1 as kernel is first process
  idle_setup(); // manipulate UserContext
  // idle begins when KernelStart returns
>>>>>>> 29f063419030c724a0fde86b65a8c8078ea2a1b2
}

