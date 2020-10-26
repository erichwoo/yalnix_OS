/* Erich Woo & Boxian Wang
 * 23 October 2020
 * process manipulation
 */

#include "process.h"

int is_empty(pcb_ll_t* ll) {
  int yes_head = (ll->head != NULL); // 0 if NULL
  int yes_tail = (ll->tail != NULL); // 0 if NULL
  int sum = yes_head + yes_tail + ll->count; // all three should be 0 if empty
  //  TracePrintf(1, "yes_head = %d, yes_tail = %d, ll->count = %d\n", yes_head, yes_tail, ll->count);
  // empty
  if (sum == 0)
    return 1;
  // error if one of head/tail/count is 0/NULL, other(s) aren't
  else if (sum != 0 && (yes_head == 0 || yes_tail == 0 || ll->count == 0)) {
    TracePrintf(1, "Error - invalid ll state: head, tail, and count not matching\n");
    return -1;
  }
  // not empty
  return 0;
}

int compare(pcb_t* a, pcb_t* b) {
  return a->data->pid - b->data->pid;
}

int PCB_setup(int ppid, user_pt_t* user_pt, kernel_stack_pt_t* k_stack_pt, UserContext* uc) {
  // initialize pcb struct and assign values
  pcb_t* pcb = (pcb_t*) malloc(sizeof(pcb_t));
  pcb->data = (data_t*) malloc(sizeof(data_t));

  pcb->data->pid = helper_new_pid(user_pt->pt);
  pcb->data->ppid = ppid;
  //pcb->data->state = READY;
  pcb->data->rc = 0; // will be changed if process error or return value needed
  pcb->data->reg1 = user_pt;
  pcb->data->k_stack = k_stack_pt;
  pcb->data->uc = uc;
  pcb->next = NULL;
  
  new(pcb); // put pcb in NEW processes list
  return pcb->data->pid;
}

int get_pid(void) {
  if (ptable == NULL || ptable->curr == NULL) {
    TracePrintf(1, "Error in get_pid(): NULL ptable or NULL current process.\n");
    return -1;
  }
  return ptable->curr->data->pid;
}

void free_pcb(pcb_t* pcb) {
  // free reg1, k_stack, uc;
  free(pcb->data->reg1);
  free(pcb->data->kstack_pt);
  free(pcb->data);
  pcb->data = NULL;
  
  free(pcb);
  pcb = NULL;
}

void initialize_ll(pcb_ll_t* ll, int id) {
  ll->id = id;
  ll->count = 0;
  ll->head = NULL;
  ll->tail = NULL;
}

void initialize_ptable(void) {
  ptable->count = 0;
  ptable->curr = NULL;
  ptable->new = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));
  ptable->ready = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));
  ptable->blocked = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));
  ptable->defunct = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));

  initialize_ll(ptable->new, NEW);
  initialize_ll(ptable->ready, READY);
  initialize_ll(ptable->blocked,BLOCKED);
  initialize_ll(ptable->defunct, DEFUNCT);
}

void free_ptable(void) {
  free(ptable->new);
  free(ptable->ready);
  free(ptable->blocked);
  free(ptable->defunct);
  ptable->new = NULL;
  ptable->ready = NULL;
  ptable->blocked = NULL;
  ptable->defunct = NULL;

  free(ptable);
  ptable = NULL;
}

void add_head(pcb_ll_t* ll, pcb_t* pcb) {
  if (ll != NULL) {
    pcb->next = ll->head; // could be NULL aka empty
    ll->head = pcb;
    if (is_empty(ll)) {
      ll->tail = pcb;
    }
    ll->count++;
  }
}

void add_tail(pcb_ll_t* ll, pcb_t* pcb) {
  if (ll != NULL) {
    if (is_empty(ll)) { // initially empty
      ll->head = pcb;
      ll->tail = pcb;
    }
    else {
      ll->tail->next = pcb;
      ll->tail = pcb;
      ll->tail->next = NULL; //make sure the pcb you added has NULL'd next
    }
    ll->count++;
  }
}

pcb_t* remove_head(pcb_ll_t* ll) {
  if (ll == NULL || is_empty(ll)) // error or length 0
    return NULL;

  pcb_t* ret = ll->head;
  ll->head = ll->head->next; // move head forward
  if (ll->head == NULL) // if new head is null aka ll was length 1, make tail null too                        
    ll->tail = NULL;
  else // before returning removed head, make its next = NULL
    ret->next = NULL;
  ll->count--;
  return ret;
}

// since not doubly linked, takes O(n)
pcb_t* remove_tail(pcb_ll_t* ll) {
  if (ll == NULL || is_empty(ll))      // error or length 0
    return NULL;

  pcb_t* pcb = ll->head;
  if (pcb->next == NULL) { // length 1
    ll->head == NULL;
    ll->tail == NULL;
    ll->count--;
    return pcb;
  }

  // traverse till reaching second to last pcb
  for (pcb = ll->head; pcb->next->next != NULL; pcb = pcb->next); // length 2+
  pcb_t* ret = pcb->next; // save last pcb for return
  pcb->next = NULL; // clear out last node/tail
  ll->tail = pcb;   // reset tail
  ll->count--;
  return ret;
}

pcb_t* find_prev(pcb_ll_t* ll, int pid) {
  if (ll == NULL || ll->count == 0 || ll->count == 1) // error or length 0 or length 1
    return NULL;

  pcb_t* prev;
  for (prev = ll->head; prev->next->data->pid != pid; prev = prev->next); // length 2+
  return prev; //prev will be NULL if not found
}

pcb_t* find(pcb_ll_t* ll, int pid) {
  if (ll == NULL || is_empty(ll)) // error or length 0
    return NULL;
  
  if (ll->head->data->pid == pid) // length 1 and found
    return ll->head;
  return find_prev(ll, pid)->next; // length 2+ or length 1 and not found
}

/*// copy b into a
void copy_pcb(pcb_t* a, pcb_t* b) {
  if (a != NULL && b != NULL) {
    // copy payload
    a->data = b->data;
    // copy next
    a->next = b->next;
  }
}
*/

// find and remove will not decrement proc_table
pcb_t* remove(pcb_ll_t* ll, int pid) {
  if (ll == NULL || is_empty(ll))
    return NULL;

  pcb_t* prev;
  pcb_t* ret = NULL;
  // if pid to remove is head; solves length 1 and luckily pid at head
  if (ll->head->data->pid == pid)
    ret = remove_head(ll);  // count is decremented in here
  else {
    prev = find_prev(ll, pid);
    if (prev != NULL) {
      ret = prev->next;
      if (ret->next == NULL)  // if one to remove is tail
	remove_tail(ll);      // count is decremented in here
      else {
	prev->next = prev->next->next; // reroute prev to next's next
	ll->count--;
      }
    }
    // does nothing if prev is NULL, so ret is NULL
  }
  
  return ret; // caller must deal with freeing removed pcb
}

void print_ll(pcb_ll_t* ll) {
  TracePrintf(1, "{");
  if (ll->head == NULL)
    TracePrintf(1, "NULL");
  else {
    for (pcb_t* pcb = ll->head; pcb != NULL; pcb = pcb->next) {
      TracePrintf(1, "%d", pcb->data->pid);
      if (pcb->next != NULL)
	TracePrintf(1, " -> ");
    }
  }
  TracePrintf(1, "}\n");
}

// only works if curr is NULL
void schedule_next(void) {
  if (ptable != NULL && ptable->curr == NULL) {
    ptable->curr = remove_head(ptable->ready); // make p_table's current the next up on queue
    TracePrintf(1, "Process %d has been scheduled to run.\n", ptable->curr->data->pid);
  }
}

void new(pcb_t* pcb) {
  if (ptable!= NULL && pcb != NULL) {
    add_tail(ptable->new, pcb);
    ptable->count++; // increment count; this is only way into ptable
    TracePrintf(1, "New Process %d created.\n", pcb->data->pid);
  }
}

void ready(pcb_t* pcb) {
  if (ptable != NULL && pcb != NULL) {
    add_tail(ptable->ready, pcb);
    TracePrintf(1, "Process %d added to ready queue.\n", pcb->data->pid);
  }
}

void block(void) {
  if (ptable != NULL && ptable->curr != NULL) {
    int pid = ptable->curr->data->pid;
    add_tail(ptable->blocked, ptable->curr);
    ptable->curr = NULL;
    TracePrintf(1, "Current Process %d moved to blocked.\n", pid);
  }
}

void defunct(int rc) {
  if (ptable != NULL && ptable->curr != NULL) {
    int pid = ptable->curr->data->pid;
    ptable->curr->data->rc = rc;
    add_tail(ptable->defunct, ptable->curr);
    ptable->curr = NULL;
    TracePrintf(1, "Current Process %d moved to defunct with rc %d.\n", pid, rc);
  }
}

// gotta free
void terminate(void) {
  if (ptable != NULL && ptable->curr != NULL) {
    int pid = ptable->curr->data->pid;
    free_pcb(ptable->curr); // curr will be NULL'd 
    ptable->count--;
    TracePrintf(1, "Current Process %d has been terminated.\n", pid);
  }
}

void new_ready(int pid) {
  pcb_t* pcb = remove(ptable->new, pid);
  if (pcb == NULL) // error pid not there
    TracePrintf(1, "Can't move from New -> Ready: pid not found\n");
  ready(pcb);
}

void unblock(int pid) {
  pcb_t* pcb = remove(ptable->blocked, pid);
  if (pcb == NULL)
    TracePrintf(1, "Can't move from Block -> Ready: pid not found\n");
  ready(pcb);
}

// preempt round robin style
// adds current process to the ready queue
// and makes next up on ready queue as the new current process
void rr_preempt(void) {
  TracePrintf(1, "Preempting Round-Robin Style!\n");
  pcb_t *this = ptable->curr; 
  ready(ptable->curr);
  ptable->curr = NULL;
  schedule_next();
  pcb_t *next = ptable->curr; 
  WriteRegister(REG_PTBR1, (unsigned int) next->pt);
  WriteRegister(REG_PTLR1, (unsigned int) NUM_PAGES_1);
  KernelContextSwitch(KCSwitch, (void *)this, (void *)next);
}

void print_ptable(void) {
  TracePrintf(1, "Total number of processes: %d\n", ptable->count);
  if (ptable->curr == NULL)
    TracePrintf(1, "Running Process id: NULL\n");
  else
    TracePrintf(1, "Running Process id: %d\n", ptable->curr->data->pid);
  TracePrintf(1, "New Processes: ");
  print_ll(ptable->new);
  TracePrintf(1, "Ready Queue: ");
  print_ll(ptable->ready);
  TracePrintf(1, "Blocked Processes: ");
  print_ll(ptable->blocked);
  TracePrintf(1, "Defunct Processes: ");
  print_ll(ptable->defunct);
}
