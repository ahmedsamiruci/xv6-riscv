#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//extern uint64 timer_scratch[1][5];

int
main(int argc, char *argv[])
{
  printf("[pid-%d]--> start test4 <--\n",getpid());
  volatile unsigned long y,x,x2;
  
  for(y=0; y < 15000; y++)
  {
    x = x2 + (356 * 34.1) * (356.86 * 356)/ 7149.08;
    x2 = x;
    printf("44");
  }
  printf("\n[pid-%d] done test4\n", getpid());
  pcb();
  exit(0);
}
