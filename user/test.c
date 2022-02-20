#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

//extern uint64 timer_scratch[1][5];

int
main(int argc, char *argv[])
{
  int n,s;

  if(argc < 2){
    n = 1;
  }
  else{
    n = atoi( argv[1] );
  }

  if(argc < 3)
    s = 1;
  else
    s = atoi ( argv[2] );


  printf("start test process with %d children with alg. step %d\n", n, s);
 /* uint64 *scratch = &timer_scratch[0][0];
  printf("current timeInter = %d\n", scratch[4]);
*/
  int i;
  int pid;
  volatile unsigned int y,x,x2;
  x2 = 1;
  for (i =0 ; i < n ; i++){
    printf("\n");
    pid = fork();
    if(pid < 0 ){
      printf("Fork failed, pid = %d\n", pid);
    }
    else if (pid > 0){ // parent process
      printf("[Parent-%d] creating child process -> {%d}\n", getpid(), pid);
      int starttime = uptime();
      for(y=0; y < 80000000; y+=s)
      {
        x = x2 + (356 * 34.1) * (356.86 * 356)/ 7149.08;
      }
      int donetime = uptime();

      printf("[Parent-%d] done with calculations, with %d ticks!\n", getpid(), donetime - starttime);
      pcb();
      printf("[Parent-%d] enter waiting state....\n", getpid());
      wait(0);
    } 
    else{ // child process
      printf("[child-%d] child process created\n", getpid());
      int starttime = uptime();
      for(y=0; y < 80000000; y+=s)
      {
        x = x + (356 * 34.1) * (356.86 * 356)/ 7149.08;
        x2 = x2 + (356 * 34.1) * (356.86 * 356)/ 7149.08;
      }
      int donetime = uptime();

      printf("[child-%d] done with Calculations, with %d ticks!\n", getpid(), donetime - starttime);
      break; // break the child loop to go to parent
    }

  }
  printf("print pcb then exit process {%d}!!\n", getpid());
  pcb();

  exit(0);
}
