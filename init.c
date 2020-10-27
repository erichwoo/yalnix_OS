#include "yuser.h"

int main(int argc, char* argv[]) {
  if (strcmp(argv[1], "0")) {
    while(1) {
      TracePrintf(1,"DoInit\n");
      Pause();
    }
  } else if (strcmp(*argv + 1, "1")) {
    while(1) {
      TracePrintf(1,"DoInit\n");
      Delay(3);
    }
  } else {
    while(1) {
      TracePrintf(1,"My pid: %d\n", GetPid());
      Pause();
    }
  }
  
}
