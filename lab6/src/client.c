#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

struct Server {
  char ip[255];
  int port;
};

struct ClientThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  errno = 0;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0 || end == str)
    return false;

  *val = i;
  return true;
}

void *ClientThread(void *args) {
  struct ClientThreadArgs *targs = (struct ClientThreadArgs *)args;

  struct hostent *hostname = gethostbyname(targs->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", targs->server.ip);
    exit(1);
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(targs->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    exit(1);
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", targs->server.ip,
            targs->server.port);
    exit(1);
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &targs->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &targs->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &targs->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    exit(1);
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Recieve failed\n");
    exit(1);
  }

  memcpy(&targs->result, response, sizeof(uint64_t));
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint64_t mod = 0;
  char servers[255] = {'\0'};
  int k_set = 0;
  int mod_set = 0;

  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k) || k == 0) {
          fprintf(stderr, "bad k\n");
          return 1;
        }
        k_set = 1;
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod) || mod == 0) {
          fprintf(stderr, "bad mod\n");
          return 1;
        }
        mod_set = 1;
        break;
      case 2:
        if (strlen(optarg) >= sizeof(servers)) {
          fprintf(stderr, "servers path too long\n");
          return 1;
        }
        memcpy(servers, optarg, strlen(optarg) + 1);
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (!k_set || !mod_set || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE *f = fopen(servers, "r");
  if (f == NULL) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers);
    return 1;
  }

  unsigned int servers_num = 0;
  char line[300];
  while (fgets(line, sizeof(line), f) != NULL) {
    if (line[0] == '\n' || line[0] == '#')
      continue;
    servers_num++;
  }
  if (servers_num == 0) {
    fprintf(stderr, "No servers in file\n");
    fclose(f);
    return 1;
  }

  struct Server *to = malloc(sizeof(struct Server) * servers_num);
  rewind(f);
  unsigned int idx = 0;
  while (fgets(line, sizeof(line), f) != NULL && idx < servers_num) {
    if (line[0] == '\n' || line[0] == '#')
      continue;
    char *colon = strchr(line, ':');
    if (colon == NULL) {
      fprintf(stderr, "Bad server line: %s\n", line);
      fclose(f);
      free(to);
      return 1;
    }
    *colon = '\0';
    strncpy(to[idx].ip, line, sizeof(to[idx].ip) - 1);
    to[idx].ip[sizeof(to[idx].ip) - 1] = '\0';
    to[idx].port = atoi(colon + 1);
    idx++;
  }
  fclose(f);
  servers_num = idx;

  pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);
  struct ClientThreadArgs *targs =
      malloc(sizeof(struct ClientThreadArgs) * servers_num);

  for (unsigned int i = 0; i < servers_num; i++) {
    targs[i].server = to[i];
    targs[i].begin = i * k / servers_num + 1;
    targs[i].end = (i + 1) * k / servers_num;
    targs[i].mod = mod;
    targs[i].result = 1;
    if (pthread_create(&threads[i], NULL, ClientThread, &targs[i]) != 0) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
  }

  uint64_t answer = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    answer = MultModulo(answer, targs[i].result, mod);
  }

  printf("answer: %llu\n", (unsigned long long)answer);

  free(threads);
  free(targs);
  free(to);

  return 0;
}
