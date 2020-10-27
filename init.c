#include "yuser.h"

char* global_var = "abcdef";

int main(int argc, char* argv[]) {
  //DEFAULT
  if (argc == 1) {
    while(1) {
      TracePrintf(1,"DoInit\n");
      Pause();
    }
  }
  // KERNELDELAY
  else if (strcmp(argv[1], "1") == 0) {
    while(1) {
      TracePrintf(1,"Delaying 3 clock_ticks\n");
      Delay(3);
    }
  }
  //KERNELGETPID
  else if (strcmp(argv[1], "2") == 0) {
    while(1) {
      TracePrintf(1,"My pid: %d\n", GetPid());
      Pause();
    }
  }
  //KERNELBRK
  else if (strcmp(argv[1], "3") == 0) {
    while(1) {
      //void* heap = malloc(10000);
      //free(heap);
      void *text, *data, *heap, *stack;

      text = main;
      data = &global_var;
      stack = &data;
      heap = malloc(4096);

      TracePrintf(0, "an address on the stack       : 0x%08x\n", (int) stack);
      TracePrintf(0, "an address in the heap        : 0x%08x\n", (int) heap);
      TracePrintf(0, "an address in the data segment: 0x%08x\n", (int) data);

      TracePrintf(0, "\n\n");
      // brk below heap : invalid
      TracePrintf(0, "Brk-ing into data segment...\n");
      Brk(data);
      // brk below brk but in heap : valid
      TracePrintf(0, "Brk-ing inside heap...\n");
      Brk(heap);
      // brk above brk : valid, brk should increase
      TracePrintf(0, "Brk-ing above heap but below stack...\n");
      Brk(heap + 10000);
      // brk into stack : invalid
      TracePrintf(0, "Brk-ing into stack...\n");
      Brk(stack);
      Pause();
    }
  }
}
