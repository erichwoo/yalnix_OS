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
    TtyPrintf(0, "And the collected write-len of the above block^ was: %d\n", write);
    
    TtyPrintf(0, "\n --------- Testing Complete ---------\n");

    free(input);
    Exit(0);
  }
  // KERNEL PIPE
  else if (strcmp(argv[1], "6") == 0) {
    int pipe_id;
    PipeInit(&pipe_id);

    int pid = Fork();
    // Child
    if (pid == 0) {
      char* s1 = "child_write";
      TtyPrintf(0, "I am Child, I will write 'child_write' to Pipe now, then delay\n");
      PipeWrite(pipe_id, s1, 12);
      Delay(1);

      char s2[5] = "";
      TtyPrintf(0, "Child will now read 5 bytes from Pipe\n");
      PipeRead(pipe_id, s2, 5);
      TtyPrintf(0, "Child got '%s'\n", s2);

      TtyPrintf(0, "Child Exiting\n");
      Exit(0);
    }
    // Parent
    char s1[100] = "";
    TtyPrintf(0, "I am Parent, I will read 100 bytes from Pipe now, waiting for someone to write\n");
    PipeRead(pipe_id, s1, 100);
    TtyPrintf(1, "Parent got '%s'\n", s1);

    char* s2 = "longer_string";
    TtyPrintf(0, "Parent will write 'longer_string' to Pipe now, then delay\n");
    PipeWrite(pipe_id, s2, 14);
    Delay(1);

    char s3[100] = "";
    TtyPrintf(0, "Parent reading remaining bytes from Pipe\n");
    PipeRead(pipe_id, s3, 100);
    TtyPrintf(0, "Parent got '%s'\n", s3);
    
    TtyPrintf(0, "Parent exiting\n");

    Wait(NULL);
    Exit(0);
  }
  // Kernel Lock/Cvar 
  else if (strcmp(argv[1], "7") == 0) {
    int lock_id;
    int want_empty;
    int want_filled;
    int pipe_id;
    LockInit(&lock_id);
    CvarInit(&want_empty);
    CvarInit(&want_filled);
    PipeInit(&pipe_id);

    // testing with CVAR is a little janky bc PipeRead == 0 will block
    // so checking if pipe is empty/filled requires writing chars
    // and checking if those chars are the ONLY chars in pipe
    TtyPrintf(0, "Pipe will be the SHARED critical data\n");
    
    // Three children, all identical
    for (int i = 1; i < 4; i++) {
      int pid = Fork();
      if (pid == 0) {
	char read[17] = "";
	int start = 1;

	TtyPrintf(0, "Child %d trying to acquire lock...\n", i);
	Acquire(lock_id);
	TtyPrintf(0, "\nChild %d acquired lock, checking if pipe is filled\n", i);
	
	//wait while pipe is empty
	while (PipeWrite(pipe_id, "12345", 6) && PipeRead(pipe_id, read, 17) == 6) {
	  if (start) {
	    start--;
	    TtyPrintf(0, "Child %d waiting for condition of filled pipe\n", i);
	  }
	  else
	    TtyPrintf(0, "Child %d REwaiting for condition of filled pipe\n", i);
	  CvarWait(want_filled, lock_id);
	  TtyPrintf(0, "\nChild %d was signaled/broadcasted to unblock/reacquire lock\n", i);
	}
	TtyPrintf(0, "Child %d passed condition check, got '%s'\n", i, read);
	TtyPrintf(0, "      and will SIGNAL parent\n");
	PipeRead(pipe_id, read, 17); // clear the "12345" at end
	CvarSignal(want_empty);
      
	TtyPrintf(0, "Child %d done doing CRITICAL WORK. Releasing lock\n", i);
	Release(lock_id);

	Exit(0);
      }
    }
    char* write1 = "parent's phrase 1";
    char* write2 = "parent's phrase 2";
    char* write3 = "parent's phrase 3";
    char read[100] = "";
    int num;

    // do 3 writes
    for (int i = 1; i < 4; i++) {
      // Acquire
      TtyPrintf(0, "\nParent delaying, then will try to acquire lock and do work\n");
      Delay(10);
      Acquire(lock_id);
      TtyPrintf(0, "\nParent acquired lock");
      TtyPrintf(0, "\nParent trying to write Phrase %d\n", i);

      // wait for pipe to be empty, so wait while pipe NOT empty
      while (PipeWrite(pipe_id, "12345", 6) && (num = PipeRead(pipe_id, read, 100)) != 6) {
	PipeWrite(pipe_id, read, num - 6); // write what you read back in
	TtyPrintf(0, "Parent waiting for condition of empty pipe\n");
	CvarWait(want_empty, lock_id);
	TtyPrintf(0, "\nParent was signaled to unblock/reacquire lock\n");
      }
      TtyPrintf(0, "Parent passed condition check. Now writing Phrase %d to pipe, ", i);
      
      // write the next phrase to pipe, then broadcast/signal
      if (i == 1) {
	TtyPrintf(0, "then BROADCASTING children\n");
	PipeWrite(pipe_id, write1, 17);
	CvarBroadcast(want_filled); // test Broadcast on first write, signal on others
      }
      else {
	TtyPrintf(0, "then SIGNALING children\n");
	if (i == 2)
	  PipeWrite(pipe_id, write2, 17);
	else if (i == 3)
	  PipeWrite(pipe_id, write3, 17);
	CvarSignal(want_filled);
      }

      // RELEASE
      TtyPrintf(0, "Parent done doing CRITICAL WORK for now. Releasing lock and delaying until my next write\n");
      Release(lock_id);
    }


    // wait for all children before exiting
    for (int i = 0; i < 3; i++)
      Wait(NULL);

    TtyPrintf(0, "Test complete. Parent exiting now\n");
    Exit(0);
  }
  //DEFAULT
  else {
    while(1) {
      TracePrintf(1,"DoInit\n");
      Pause();
    }
  }
}
