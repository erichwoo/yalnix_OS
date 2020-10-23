trap_handler_t trap_vector[TRAP_VECTOR_SIZE]; // array of pointers to trap handler functs 
proc_table_t *proc_table = NULL;
free_frame_t free_frame = {0, NULL, BASE_FRAME, 0};
kernel_global_pt_t kernel_pt;
void *kernel_brk = NULL; // to be modified by SetKernelBrk



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
  for (int i = 0; i < free_frame.size / CELL_SIZE + 1; i++) free_frame.bit_vector[i] = (char) 0; // initialzie 


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