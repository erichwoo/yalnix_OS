#include "yuser.h"

char* global_var = "abcdef";

int main(int argc, char* argv[]) {
  // Helper for KERNEL EXEC
  if (strcmp(argv[1], "0") == 0) {
    TracePrintf(1, "I am a helper for Exec test!\n");
    TracePrintf(1, "I will exit with code 5\n");

    Exit(5);
  }
  //KERNELGETPID
  else if (strcmp(argv[1], "1") == 0) {
    TracePrintf(1,"My pid: %d\n", GetPid());

    Exit(0);
  }
  // KERNELDELAY
  else if (strcmp(argv[1], "2") == 0) {
    TracePrintf(1,"Testing Delay for 3 clock_ticks\n");
    Delay(3);

    Exit(0);
  }
  //KERNELBRK
  else if (strcmp(argv[1], "3") == 0) {
    TracePrintf(1, "Testing Brk-ing\n");
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
    TracePrintf(1, "Testing Fork/Exec/Wait/Exit\n");
    TracePrintf(1, "Waiting without a child to wait for...\n");
    Wait(NULL);
    TracePrintf(1, "Now testing standard cases\n");
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
    TracePrintf(1, "Child 1 pid is %d and should exit 42\nChild 2 pid is %d and should exit 5\n", cid1, cid2);
    TracePrintf(1, "Child id %d exited first with status %d\n", first_defunct, status1);
    TracePrintf(1, "Child id %d exited second with status %d\n", second_defunct, status2);

    Exit(0);
  }
  //KERNEL TTY
  else if (strcmp(argv[1], "5") == 0) {
    TracePrintf(1, "Testing Terminal functionality\n");
    char *input = malloc(100 * sizeof(char));

    // Test immediate return of TtyRead when there are unread bytes to be collected
    TtyPrintf(0, "------- Test 1: Delaying, then TtyRead -------\n");
    TtyPrintf(0, "Type something NOW in Console as we delay 20 clock-ticks...\n");
    Delay(20);
    TtyPrintf(0, "Calling TtyRead now...\n");
    TtyRead(0, input, 100);
    TtyPrintf(0, "===== Read: %s\n", input);
    memset(input, 0, 100);
	   
    // Test read len < buffer len
    TtyPrintf(0, "\n------ Test 2: TtyRead with len < available input ------\n");
    TtyPrintf(0, "Reading the next 5 bytes. Input something longer than 5...\n");
    TtyRead(0, input, 5);
    TtyPrintf(0, "===== The next 5 bytes were: %s\n", input);
    memset(input, 0, 100);
    TtyPrintf(0, "Now reading the rest (will block if you inputted < 5 bytes above)\n");
    TtyRead(0, input, 100);
    TtyPrintf(0, "===== The rest was: %s\n", input);
    memset(input, 0, 100);
    
    // Test read len > buffer len
    TtyPrintf(0, "\n------ Test 3: TtyRead with len > available input ------\n");
    TtyPrintf(0, "Calling TtyRead with len 100; type less than 100 characters\n");
    int read = TtyRead(0, input, 100);
    TtyPrintf(0, "==== Read %d bytes as: %s\n", read, input);
    memset(input, 0, 100);
	
    // Read from 0, Write to 1
    TtyPrintf(0, "\n------ Test 4: TtyRead from 0, TtyWrite to 1 ------\n");
    TtyPrintf(0, "Type something here....Then look at Terminal 1\n");
    TtyRead(0, input, 100);
    TtyPrintf(1, "===== Read from Terminal 0: %s\n", input);
    memset(input, 0, 100);
    
    // Read from 1, Write to 3
    TtyPrintf(1, "\n------ Test 5: TtyRead from 1, TtyWrite to 3 ------\n");
    TtyPrintf(1, "Type something here....Then look at Terminal 3\n");
    TtyRead(1, input, 100);
    TtyPrintf(3, "===== Read from Terminal 1: %s\n", input);
    TtyPrintf(3, "Go back to Terminal 0 for rest of tests...\n");
    memset(input, 0, 100);
    
    // Return Val of Write
    TtyPrintf(0, "\n------ Test 6: Return Val of TtyWrite ------\n");
    int write = TtyWrite(0, "This writing block is 40 bytes long...\n", 40);
    TtyPrintf(0, "And the collected write-len of the above block^ was: %d\n");
    
    TtyPrintf(0, "\n --------- Testing Complete ---------\n");

    free(input);
    Exit(0);
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
  }
  //DEFAULT
  else {
    while(1) {
      TracePrintf(1,"DoInit\n");
      Pause();
    }
  }
}
