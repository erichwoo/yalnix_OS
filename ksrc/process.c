/* Erich Woo & Boxian Wang
 * 23 October 2020
 * process manipulation. See process.h for detailed documentation
 */

#include "process.h"

int get_pid(node_t *proc) {
  return ((pcb_t *) proc->data)->pid;
}

node_t *get_parent(node_t *proc) {
  return ((pcb_t *) proc->data)->parent;
}

node_t *process_init(void) {
  pcb_t *new_pcb = malloc(sizeof(pcb_t));
  new_pcb->parent = NULL;
  new_pcb->a_children = new_ll();
  new_pcb->d_children = new_ll();
  new_pcb->userpt = new_user_pt();
  new_pcb->kstack = malloc(sizeof(kernel_stack_pt_t));
  new_pcb->pid = helper_new_pid(new_pcb->userpt->pt);
  return new_node((void *) new_pcb);
}

node_t *process_copy(node_t* parent) {
  node_t *child_node = process_init();
  pcb_t *child = child_node->data, *parent_pcb = parent->data;
  node_t *child_copy = new_node((void *) child);
  enqueue(parent_pcb->a_children, child_copy);
  child->parent = parent;
  child->uc = parent_pcb->uc;
  copy_user_mem(parent_pcb->userpt, child->userpt);
  return child_node;
}

void process_terminate(node_t *proc, int rc) { // return code
  // setup process for defunct state
  // zero/NULL variables so any future errant access is safe
  proc->code = rc;
  pcb_t *p = proc->data;
  helper_retire_pid(p->pid);
  destroy_usermem(p->userpt);
  // orphan alive children
  node_t *last = NULL;
  for (node_t *curr = p->a_children->head; curr != NULL; curr = curr->next) {
    ((pcb_t *) curr->data)->parent = NULL;
    free(last);
    last = curr;
  }
  free(last);
  free(p->a_children);
  p->a_children = NULL;
  // destroy defunct children
  for (node_t* curr = p->d_children->head; curr != NULL; curr = curr->next)
    process_destroy(curr);
  free(p->d_children);
  p->d_children = NULL;
}

void process_destroy(node_t *proc) {
  pcb_t *p = proc->data;
  destroy_kstack(p->kstack);
  free(p->kstack);
  free(p->userpt);
  destroy_node(proc);
}

