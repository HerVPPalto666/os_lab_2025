#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct FactArgs {
  int begin;
  int end;
  int mod;
};

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
long long result = 1;

void *ThreadFact(void *args) {
  struct FactArgs *fargs = (struct FactArgs *)args;
  long long local = 1;

  for (int i = fargs->begin; i <= fargs->end; i++) {
    local = (local * i) % fargs->mod;
  }

  pthread_mutex_lock(&mut);
  result = (result * local) % fargs->mod;
  pthread_mutex_unlock(&mut);

  return NULL;
}

int main(int argc, char **argv) {
  int k = -1;
  int pnum = -1;
  int mod = -1;

  while (true) {
    static struct option options[] = {{"pnum", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "k:", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 'k':
        k = atoi(optarg);
        break;
      case 0:
        switch (option_index) {
          case 0:
            pnum = atoi(optarg);
            break;
          case 1:
            mod = atoi(optarg);
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (k <= 0 || pnum <= 0 || mod <= 0) {
    printf("Usage: %s -k \"num\" --pnum=\"num\" --mod=\"num\"\n", argv[0]);
    return 1;
  }

  if (pnum > k) {
    pnum = k;
  }

  pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
  struct FactArgs *args = malloc(sizeof(struct FactArgs) * pnum);

  for (int i = 0; i < pnum; i++) {
    args[i].begin = i * k / pnum + 1;
    args[i].end = (i + 1) * k / pnum;
    args[i].mod = mod;
    if (pthread_create(&threads[i], NULL, ThreadFact, &args[i]) != 0) {
      perror("pthread_create");
      return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("pthread_join");
      return 1;
    }
  }

  printf("%d! mod %d = %lld\n", k, mod, result);

  free(threads);
  free(args);
  return 0;
}
