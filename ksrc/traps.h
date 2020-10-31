#include <ykernel.h>

typedef void (*trap_handler_t) (UserContext* uc);



void TrapKernel(UserContext *uc);
                            
void TrapClock(UserContext *uc);

void TrapIllegal(UserContext *uc);
                            
void TrapMemory(UserContext *uc);

void TrapMath(UserContext *uc);

void TrapTtyReceive(UserContext *uc);

void TrapTtyTransmit(UserContext *uc);

void TrapDisk(UserContext *uc);

void TrapTemp(UserContext *uc);

