/* Erich Woo & Boxian Wang
 * 23 October 2020
 * process manipulation
 */

#include "process.h"
int isEmpty(pcb_ll_t* ll) {
  int empty_head = (ll->head == NULL);
  int empty_tail = (ll->tail == NULL);
  int sum = empty_head + empty_tail;
  if (sum == 0)
    return 1;
  else if (sum == empty_head || sum == empty_tail)
    return -1; // error one is NULL, other isn't
  else
    return 0;
}

int compare(pcb_t* a, pcb_t* b) {
  return a->data->pid - b->data->pid;
}

int PCB_setup(proc_table_t* ptable, int ppid, user_pt_t* user_pt, kernel_stack_pt_t* k_stack_pt, UserContext* uc) {
  // initialize pcb struct and assign values
  pcb_t* pcb = (pcb_t*) malloc(sizeof(pcb_t));
  pcb->data = (data_t*) malloc(sizeof(data_t));
  
  pcb->data->pid = helper_new_pid(user_pt->pt);
  pcb->data->ppid = ppid;
  pcb->data->state = READY;
  pcb->data->rc = 0; // will be changed if process error or return value needed
  pcb->data->reg1 = user_pt;
  pcb->data->k_stack = k_stack_pt;
  pcb->data->uc = uc;
  pcb->next = NULL;
  
  enqueue(ptable->new, pcb);
  ptable->count++;
  return pcb->data->pid;
}

void free_pcb(pcb_t* pcb) {
  // free reg1, k_stack, uc;
  free(pcb->data);
  pcb->data = NULL;

  free(pcb);
  pcb = NULL;
}

void initialize_ll(pcb_ll_t* ll) {
  ll->count = 0;
  ll->head = NULL;
  ll->tail = NULL;
}

void initialize_ptable(proc_table_t* ptable) {
  ptable->count = 0;
  ptable->curr = NULL;
  ptable->new = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));
  ptable->ready = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));
  ptable->blocked = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));
  ptable->defunct = (pcb_ll_t*) malloc(sizeof(pcb_ll_t));

  initialize_ll(ptable->new);
  initialize_ll(ptable->ready);
  initialize_ll(ptable->blocked);
  initialize_ll(ptable->defunct);
}

void free_ptable(proc_table_t* ptabe) {
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
    if (ll->count == 0)
      ll->tail = pcb;
    ll->count++;
  }
}

void add_tail(pcb_ll_t* ll, pcb_t* pcb) {
  if (ll != NULL) {
    if (ll->count == 0) { // initially empty
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
  if (ll == NULL || ll->count == 0) // error or length 0
    return NULL;

  pcb_t* ret = ll->head;
  ll->head = ll->head->next; // move head forward
  if (ll->head == NULL) // if new head is null aka ll was length 1, make tail null too                        
    ll->tail = NULL;
  else // new head is temp->next, so if it isn't already NULL, do so before returning    
    ret->next = NULL;
  ll->count--;
  return ret;
}

pcb_t* remove_tail(pcb_ll_t* ll) {
  if (ll == NULL || ll->count == 0)      // error or length 0
    return NULL;

  pcb_t* pcb = ll->head;
  if (ll->head->next == NULL) { // length 1
    ll->head == NULL;
    ll->tail == NULL;
    ll->count--;
    return pcb;
  }

  // traverse till reaching second to last pcb
  for (pcb = ll->head; pcb->next->next != NULL; pcb = pcb->next); // length 2+
  pcb_t* ret = pcb->next; // save deleted pcb for return
  pcb->next = NULL; // clear out last node/tail
  ll->tail = pcb;   // reset tail
  ll->count--;
  return ret;
}

pcb_t* find_prev(pcb_ll_t* ll, int pid) {
  if (ll == NULL || ll->count == 0 || ll->count == 1) // error or length 0 or length 1
    return NULL;

  for (prev = ll->head; prev->next->data->pid != pid; prev = prev->next); // length 2+
  return prev; //prev will be NULL if not found
}

pcb_t* find(pcb_ll_t* ll, int pid) {
  if (ll == NULL || ll->count == 0) // error or length 0
    return NULL;
  
  if (ll->head->data->pid == pid) // length 1
    return ll->head;
  return find_prev(ll, pid)->next; // length 2+
}

// copy b into a
void copy_pcb(pcb_t* a, pcb_t* b) {
  if (a != NULL && b != NULL) {
    // copy payload
    a->data = b->data;
    // copy next
    a->next = b->next;
  }
}

//
pcb_t* remove(pcb_ll_t* ll, pcb_t* pcb) {
  if (pcb->next == NULL) { // if tail, or tail & haed
    remove_tail(ll);
  }

  pcb_t* ret = pcb;
  copy_pcb(pcb, pcb->next);
  // if head
  if (ll->head->data->pid == pcb->data->pid)
  
  return NULL;
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

void terminate(proc_table_t* ptable) {
  free_pcb(ptable->curr); // curr will be NULL'd 
  ptable->count--;
}

void schedule_next(proc_table_t* ptable) {
  ptable->curr = remove_head(ptable->ready); // make p_table's current the next up on queue
}

void block(proc_table_t* ptable) {
  add_tail(ptable->blocked, ptable->curr);
  ptable->curr = NULL;
}

void defunct(proc_table_t* ptable) {
  add_tail(ptable->defunct, ptable->curr);
  ptable->curr = NULL;
}

// preempt round robin style
// adds current process to the ready queue
// and makes next up on ready queue as the new current process
void rr_preempt(proc_table_t* ptable) {
  add_tail(ptable->ready, ptable->curr); // push current process to back of queue
  schedule_next(ptable); // will replace curr
}

void print_ptable(proc_table_t* ptable) {
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

