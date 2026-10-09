#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void *thread1_func(void *arg) {
  printf("Thread1: locking mutex1\n");
  pthread_mutex_lock(&mutex1);
  sleep(1);
  printf("Thread1: locking mutex2\n");
  pthread_mutex_lock(&mutex2);
  printf("Thread1: got both locks\n");
  pthread_mutex_unlock(&mutex2);
  pthread_mutex_unlock(&mutex1);
  return NULL;
}

void *thread2_func(void *arg) {
  printf("Thread2: locking mutex2\n");
  pthread_mutex_lock(&mutex2);
  sleep(1);
  printf("Thread2: locking mutex1\n");
  pthread_mutex_lock(&mutex1);
  printf("Thread2: got both locks\n");
  pthread_mutex_unlock(&mutex1);
  pthread_mutex_unlock(&mutex2);
  return NULL;
}

int main(void) {
  pthread_t t1, t2;

  pthread_create(&t1, NULL, thread1_func, NULL);
  pthread_create(&t2, NULL, thread2_func, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  printf("Finished\n");
  return 0;
}
