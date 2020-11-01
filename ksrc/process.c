/* Erich Woo & Boxian Wang
 * 23 October 2020
 * process manipulation
 */

#include "process.h"

/******** PCB management *******/

int get_pid(node_t *proc) {
  return ((pcb_t *) proc->data)->pid;
}

node_t *get_parent(node_t *proc) {
  return ((pcb_t *) proc->data)->parent;
}

// set up blank pcb node
node_t *process_init(void) {
  pcb_t *new_pcb = malloc(sizeof(pcb_t));
  new_pcb->parent = NULL;
  //new_pcb->children = new_ll();
  new_pcb->a_children = new_ll();
  new_pcb->d_children = new_ll();
  new_pcb->userpt = new_user_pt();
  new_pcb->kstack = malloc(sizeof(kernel_stack_pt_t));
  new_pcb->pid = helper_new_pid(new_pcb->userpt->pt);
  return new_node((void *) new_pcb);
}

// copy a process, along with its  memory content (user and kernel stack); DOES NOT copy kernel context because whose timing is critical
node_t *process_copy(node_t* parent) {
  node_t *child_node = process_init();
  pcb_t *child = child_node->data, *parent_pcb = parent->data;
  enqueue(parent_pcb->a_children, child_node);
  child->parent = parent;
  child->uc = parent_pcb->uc;
  copy_user_mem(parent_pcb->userpt, child->userpt);
  return child_node;
}

// terminate a process, meaning freeing all its frames (user), orphane its children list and free it, store its return code.
void process_terminate(node_t *proc, int rc) { // return code
  // setup process for defunct state
  proc->code = rc;
  pcb_t *p = proc->data;
  destroy_usermem(p->userpt);
  /*for (node_t *curr = p->children->head; curr != NULL; curr = curr->next) {// orphane children
    ((pcb_t *) curr->data)->parent = NULL;
  }
  free(p->children);
  */
  
  // orphan alive children
  for (node_t *curr = p->a_children->head; curr != NULL; curr = curr->next) 
    ((pcb_t *) curr->data)->parent = NULL;
  free(p->a_children);
  p->a_children = NULL;
  // destroy defunct children
  for (node_t* curr = p->d_children->head; curr != NULL; curr = curr->next)
    process_destroy(curr);
  free(p->d_children);
  p->d_children = NULL;
}

// free the node and all associated memory (free cached kernel also); must in a DIFFERENT process
void process_destroy(node_t *proc) {
  pcb_t *p = proc->data;
  helper_retire_pid(p->pid);
  destroy_kstack(p->kstack);
  free(p->kstack);
  free(p->userpt);
  destroy_node(proc);
}

