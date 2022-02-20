#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

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

  int i;
  int pid;
  volatile unsigned int y,x;
  for (i =0 ; i < n ; i++){
    printf("\n");
    pid = fork();
    if(pid < 0 ){
      printf("Fork failed, pid = %d\n", pid);
    }
    else if (pid > 0){ // parent process
      printf("[Parent-%d] creating child process -> {%d}\n", getpid(), pid);
      
      for(y=0; y < 80000000; y+=s)
        x = x + (356 * 34.1) * (356.86 * 356)/ 7149.08;

      printf("[Parent-%d] done with calculations!\n", getpid());
      pcb();
      printf("[Parent-%d] enter waiting state....\n", getpid());
      wait(0);
    } 
    else{ // child process
      printf("[child-%d] child process created\n", getpid());
      for(y=0; y < 80000000; y+=s)
        x = x + (356 * 34.1) * (356.86 * 356)/ 7149.08;

      printf("[child-%d] done with Calculations!\n", getpid());
      break; // break the child loop to go to parent
    }

  }
  printf("print pcb then exit process {%d}!!\n", getpid());
  pcb();
  exit(0);
}
