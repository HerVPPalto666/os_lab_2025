#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    printf("Fork failed!\n");
    return 1;
  }

  if (pid == 0) {
    execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2],
          (char *)NULL);
    perror("execl");
    exit(1);
  }

  wait(NULL);
  return 0;
}
