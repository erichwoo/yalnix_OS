#define TERMINATED -1
#define RUNNING 0
#define READY 1
#define BLOCKED 2
#define DEFUNCT 3

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
  // KernelContext *kc;
} pcb_t;

typedef struct proc_table { // maybe a queue?
  int size; // number of current process under management
  pcb_t *head; //pointer to the head of the queue
} proc_table_t;

// need some queue data struct to manage incoming pipe/lock/cond_var users
// could possibly help with pcb management with proc_table
typedef struct queue {
  
} queue_t;
