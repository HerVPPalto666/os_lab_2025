#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t got_alarm = 0;

void alarm_handler(int signo) {
  got_alarm = 1;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed is a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size is a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum is a positive number\n");
              return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout is a positive number\n");
              return 1;
            }
            break;
          case 4:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;
  pid_t *child_pids = malloc(sizeof(pid_t) * pnum);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipefd[pnum][2];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipefd[i]) == -1) {
        printf("Pipe failed!\n");
        free(array);
        free(child_pids);
        return 1;
      }
    }
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        free(child_pids);
        unsigned int begin = i * array_size / pnum;
        unsigned int end = (i + 1) * array_size / pnum;
        struct MinMax local_minmax = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[256];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *fp = fopen(filename, "w");
          if (fp == NULL) {
            printf("Failed to open file\n");
            free(array);
            return 1;
          }
          fprintf(fp, "%d %d\n", local_minmax.min, local_minmax.max);
          fclose(fp);
        } else {
          for (int j = 0; j < pnum; j++) {
            close(pipefd[j][0]);
            if (j != i) {
              close(pipefd[j][1]);
            }
          }
          write(pipefd[i][1], &local_minmax.min, sizeof(int));
          write(pipefd[i][1], &local_minmax.max, sizeof(int));
          close(pipefd[i][1]);
        }
        free(array);
        return 0;
      } else {
        child_pids[i] = child_pid;
      }

    } else {
      printf("Fork failed!\n");
      free(array);
      free(child_pids);
      return 1;
    }
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipefd[i][1]);
    }
  }

  if (timeout > 0) {
    signal(SIGALRM, alarm_handler);
    alarm(timeout);
  }

  while (active_child_processes > 0) {
    int status;
    pid_t done = waitpid(-1, &status, timeout > 0 ? WNOHANG : 0);
    if (done > 0) {
      active_child_processes -= 1;
    } else if (timeout > 0 && got_alarm) {
      for (int i = 0; i < pnum; i++) {
        kill(child_pids[i], SIGKILL);
      }
      while (active_child_processes > 0) {
        if (waitpid(-1, &status, 0) > 0) {
          active_child_processes -= 1;
        }
      }
      break;
    } else if (timeout > 0 && done == 0) {
      usleep(1000);
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int got_min = 0;
    int got_max = 0;

    if (with_files) {
      char filename[256];
      sprintf(filename, "min_max_%d.txt", i);
      FILE *fp = fopen(filename, "r");
      if (fp != NULL) {
        if (fscanf(fp, "%d %d", &min, &max) == 2) {
          got_min = 1;
          got_max = 1;
        }
        fclose(fp);
        remove(filename);
      }
    } else {
      if (read(pipefd[i][0], &min, sizeof(int)) == (ssize_t)sizeof(int)) {
        got_min = 1;
      }
      if (read(pipefd[i][0], &max, sizeof(int)) == (ssize_t)sizeof(int)) {
        got_max = 1;
      }
      close(pipefd[i][0]);
    }

    if (got_min && min < min_max.min) min_max.min = min;
    if (got_max && max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
