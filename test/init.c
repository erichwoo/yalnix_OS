#include "yuser.h"

char* global_var = "abcdef";

int main(int argc, char* argv[]) {
  //KERNELGETPID
  if (strcmp(argv[1], "0") == 0) {
    TracePrintf(1, "I am a helper for Exec test!\n");
    TracePrintf(1, "I will exit with code 5\n");

    Exit(5);
  }
  else if (strcmp(argv[1], "1") == 0) {
    TracePrintf(1,"My pid: %d\n", GetPid());

    Exit(0);
  }
  // KERNELDELAY
  else if (strcmp(argv[1], "2") == 0) {
    TracePrintf(1,"Delaying 3 clock_ticks\n");
    Delay(3);

    Exit(0);
  }
  //KERNELBRK
  else if (strcmp(argv[1], "3") == 0) {
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

    //exiting
    Exit(0);
  }
  //KERNEL FORK, EXEC, and WAIT
  else if (strcmp(argv[1], "4") == 0) {
    // This child will Delay
    int cid1 = Fork();
    if (cid1 == 0) {
      TracePrintf(1, "I am child 1 and my pid is %d\n", GetPid());
      TracePrintf(1, "I will Delay for 3 clock ticks, then Exit with status = 42\n");
      Delay(3);
      Exit(42);
    }
    // This child will Exec
    int cid2 = Fork();
    if (cid2 == 0) {
      TracePrintf(1, "I am child 2 and my pid is %d\n", GetPid());
      TracePrintf(1, "I will Exec with args: 'test/init 0', which should Exit with status = 5\n");
      char* args[2] = {"test/init", "0"};
      Exec(args[0], args);
      TracePrintf(1, "If you get here, Exec'd child errrored!\n");
      Exit(-1);
    }
    int status1, status2;
    int first_defunct = Wait(&status1);
    int second_defunct = Wait(&status2);
    TracePrintf(1, "Child 1 pid is %d and Child 2 pid is %d\n", cid1, cid2);
    TracePrintf(1, "Child id %d exited first with status %d\n", first_defunct, status1);
    TracePrintf(1, "Child id %d exited second with status %d\n", second_defunct, status2);

    Exit(0);
  }
  else if (strcmp(argv[1], "5") == 0) {
    while(1) {
      TracePrintf(1,"Delaying 3 clock_ticks\n");
      //for (int i = 1; i < 100; i++) TracePrintf(1,"\n"); 
    }
  }
  //PIPE
  else if (strcmp(argv[1], "6") == 0) {
    int l;
    PipeInit(&l);
    int i = Fork();
    if (i == 0) {
      
      char s[10] = "";
      PipeRead(l, s, 100);
      TracePrintf(1, "CHild Got %s\n", s);
    } else {
      char s[] = "astring";
      Delay(5);
      PipeWrite(l, s, 8);
      TracePrintf(1, "PArent wrote\n");
      
      TracePrintf(1, "Pare rekease\n");
      while(1);
    }
  }
  //KERNELBRK
  else if (strcmp(argv[1], "7") == 0) {
    while (1) {
      char d[50000];
      TracePrintf(1, "ok\n");
      d[50000] = 'a';
      Pause();

    }
  }
  else if (strcmp(argv[1], "8") == 0) {
    char *input = malloc(100);
    TtyRead(0, input, 100);
    TtyPrintf(1, "%s", input);
  }
  //DEFAULT
  else {
    while(1) {
      TracePrintf(1,"DoInit\n");
      Pause();
    }
  }
}
