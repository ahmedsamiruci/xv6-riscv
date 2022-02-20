#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//extern uint64 timer_scratch[1][5];

int
main(int argc, char *argv[])
{


  printf("start test process \n");
  volatile unsigned int y,x,x2;

    int starttime = uptime();
    for(y=0; y < 80000000; y++)
    {
      x = x2 + (356 * 34.1) * (356.86 * 356)/ 7149.08;
      x2 = x;
    }
    int donetime = uptime();

    printf("[Process-%d] done with calculations, with %d ticks!\n", getpid(), donetime - starttime);
 
  exit(0);
}
