/* Erich WOo & Boxian Wang
 * 26 October 2020
 * Kernel Context Switching
 */

#include <ykernel.h>
#include "linked_list.h"
#include "process.h"
#include "memory.h"
#include "scheduling.h"

extern proc_table_t *procs;
extern kernel_global_pt_t kernel_pt;

KernelContext* KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);

void save_uc(UserContext *uc) {
  pcb_t *curr = procs->running->data;
  curr->uc = *uc;
}

void restore_uc(UserContext *uc) {
  pcb_t *curr = procs->running->data;
  *uc = curr->uc;
}

void add_return_val(int r) {
    pcb_t *curr = procs->running->data;
    curr->uc.regs[0] = r;
}

void switch_proc(node_t *from, node_t *to) {
    WriteRegister(REG_PTBR1, (unsigned int) ((pcb_t *)to->data)->userpt->pt);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
    KernelContextSwitch(KCSwitch, from->data, to->data);
}


void copy_kernel(node_t *child) {
    KernelContextSwitch(KCCopy, child->data, NULL);
}

KernelContext* KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    pcb_t *curr_pcb = (pcb_t *) curr_pcb_p, *next_pcb = (pcb_t *) next_pcb_p;
    curr_pcb->kc = *kc_in;

    // switching kernel stack
    for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) {
        kernel_pt.pt[vpn - BASE_PAGE_0] = next_pcb->kstack->pt[vpn - BASE_PAGE_KSTACK];
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK);
    return &next_pcb->kc; // teleport to next
}

// make sure new_pcb_p is initialized/copied before calling
KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used) {
    
    pcb_t *new_pcb = (pcb_t *) new_pcb_p;
    new_pcb->kc = *kc_in;

    // copy kernel stack; has to be done in a magical stack!
    int dummy = BASE_PAGE_KSTACK - 1;
    for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) {
        set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
        memcpy((void*) (dummy << PAGESHIFT), (void*) (vpn << PAGESHIFT), PAGESIZE);
        WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT); // must flush after use!
        new_pcb->kstack->pt[vpn - BASE_PAGE_KSTACK] = kernel_pt.pt[dummy - BASE_PAGE_0];
    }
    set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 0, NONE, NONE);
    return kc_in; // go back
}
