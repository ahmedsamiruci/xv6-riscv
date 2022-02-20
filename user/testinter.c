#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int interval;

  if(argc < 2){
    fprintf(2, "usage: scheduler interval...\n");
    exit(1);
  }
  interval = atoi( argv[1] );

  printf("change scheduler with interval = [%d]\n", interval);
  // syscall inter .. 
  inter(interval);

  exit(0);
}
