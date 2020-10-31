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
      //for (int i = 1; i < 100; i++) TracePrintf(1,"\n");
      
    }
  }
  //KERNELGETPID
  else if (strcmp(argv[1], "2") == 0) {
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
  else if (strcmp(argv[1], "3") == 0) {
    while (1) {
      char d[50000];
      TracePrintf(1, "ok\n");
      d[50000] = 'a';
      Pause();

    }
  }
  else if (strcmp(argv[1], "4") == 0) {
    char *input = malloc(100);
    TtyRead(0, input, 100);
    TtyPrintf(1, "%s", input);
  }
}
