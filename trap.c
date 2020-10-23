typedef void (*trap_handler_t) (UserContext* uc);

// Examine the "code" field of user context and decide which syscall to invoke
void TrapKernel(UserContext *uc) {
  TracePrintf(1, "The code of syscall is 0x%x\n", uc->code);
}
// Check the process table to decide which process to schedule; initialize a context switch if necessary
void TrapClock(UserContext *uc) {
  TracePrintf(1, "Clock Trap occured!\n");
}
// temporary trap handler funct for all the unhandled traps. 
void TrapTemp(UserContext *uc) {
  TracePrintf(1, "This trap type %d is currently unhandled\n", uc->vector);
}