/* Erich WOo & Boxian Wang
 * 26 October 2020
 * Kernel Context Switching
 */

#include "cswitch.h"

KernelContext* KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    pcb_t *curr_pcb = (pcb_t *) curr_pcb_p, *next_pcb = (pcb_t *) next_pcb_p;
    //TracePrintf(0, "kcswitch");
    *(curr_pcb->kc) = *kc_in;

    for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) {
        kernel_pt.pt[vpn - BASE_PAGE_0] = next_pcb->k_stack->pt[vpn - BASE_PAGE_KSTACK];
        //TracePrintf(0, "stack%d is now %d\n", vpn, kernel_pt.pt[vpn - BASE_PAGE_0].pfn);
        //WriteRegister(REG_TLB_FLUSH, vpn << PAGESHIFT);
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);
    return next_pcb->kc; // teleport to next
}

// make sure new_pcb_p is initialized before calling !!!
KernelContext* KCCopy(KernelContext *kc_in, void *new_pcb_p, void *not_used) {
    // clone user page tbale?
    
    pcb_t *new_pcb = (pcb_t *) new_pcb_p;
    *new_pcb->kc = *kc_in;

    // copy kernel stack
    int dummy = BASE_PAGE_KSTACK - 1;
    for (int vpn = BASE_PAGE_KSTACK; vpn < LIM_PAGE_KSTACK; vpn++) {
        set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 1, get_frame(NONE, AUTO), PROT_READ|PROT_WRITE);
        //WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT);
        memcpy((void*) (dummy << PAGESHIFT), (void*) (vpn << PAGESHIFT), PAGESIZE);
        WriteRegister(REG_TLB_FLUSH, dummy << PAGESHIFT);
        //TracePrintf(0, "memcpy %d %d\n", dummy, vpn);
        //TracePrintf(0, "memcmp %d\n", memcmp((void*) (dummy << PAGESHIFT), (void*) (vpn << PAGESHIFT), PAGESIZE));
        //memcpy((void*) (dummy << PAGESHIFT), (void*) (vpn << PAGESHIFT), PAGESIZE);
        new_pcb->k_stack->pt[vpn - BASE_PAGE_KSTACK] = kernel_pt.pt[dummy - BASE_PAGE_0];
    }
    set_pte(&kernel_pt.pt[dummy - BASE_PAGE_0], 0, NONE, NONE);
    
    //TracePrintf(0, "kccopy\n");
    //TracePrintf(0, "base %d\n", BASE_PAGE_KSTACK);
    //TracePrintf(0, "dummy %d\n", dummy);
    return kc_in; // go back
}
