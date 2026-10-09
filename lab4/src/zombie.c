#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  pid_t pid = fork();

  if (pid < 0) {
    printf("Fork failed!\n");
    return 1;
  }

  if (pid == 0) {
    printf("Child pid=%d exiting immediately\n", getpid());
    _exit(0);
  }

  printf("Parent pid=%d, child pid=%d is zombie until wait\n", getpid(), pid);
  printf("Check with: ps -o pid,ppid,stat,cmd -p %d\n", pid);
  sleep(30);
  return 0;
}
