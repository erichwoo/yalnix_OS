/* Erich WOo & Boxian Wang
 * 26 October 2020
 * Kernel Context Switching. See cswitch.h for detailed documentation
 */

#include "cswitch.h"

// THE proc table
extern proc_table_t *procs;

// THE global kernel page tables
extern kernel_global_pt_t kernel_pt;

/********************* FUNCTION DECLARATIONS ***********************/

/* Switches Kernel Context and kernel stack pages
 * from the specified current process, to the specified next.
 * A copy of the KernelContext is stored into the current process so that when the
 * current process resumes at some later time, it will return from the KCS call.
 * The kc* of the next process is returned, so that the kernel teleports to the next.
 *
 * @param kc_in the Kernel Context pointer of the caller
 * @param curr_pcb_p the current process' pcb pointer
 * @param next_pcb_p the next process' pcb pointer
 * @return the KernelContext pointer of the next process
*/
KernelContext* KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);

/* From the Yalnix Manual: 
 * - KCCopy will simply copy the kernel context from *kc_in into the new pcb, 
 * and copy the contents of the current kernel stack
 * into the frames that have been allocated for the new process’s kernel stack. 
 * However, it will then return kc in.
 * 
 * new_pcb_p must be an initialized/copied pcb_t* before calling KCCopy
 *
 * @param kc_in the Kernel Context pointer of the caller
 * @param new_pcb_p the child's pcb pointer
 * @param not_used an unused parameter
 * @return the KernelContext pointer both parent and child return from
 */
KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used);

/********************* FUNCTIONS ***********************/

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
    WriteRegister(REG_PTBR1, (unsigned int) next_pcb->userpt->pt);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
    return &next_pcb->kc; // teleport to next
}

KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used) {    
    pcb_t *new_pcb = (pcb_t *) new_pcb_p;
    new_pcb->kc = *kc_in;

    // copy kernel stack; has to be done in a magical stack!
    int dummy = BASE_PAGE_KSTACK - 1;
    for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) {
        set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
        WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT); // must flush after use!
        memcpy((void*) (dummy << PAGESHIFT), (void*) (vpn << PAGESHIFT), PAGESIZE);
        
        new_pcb->kstack->pt[vpn - BASE_PAGE_KSTACK] = kernel_pt.pt[dummy - BASE_PAGE_0];
    }
    set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 0, NONE, NONE);
    WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT);
    return kc_in; // go back
}
