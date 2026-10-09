CC=gcc
CFLAGS=-I. -I../../lab3/src
LAB3=../../lab3/src

all: parallel_min_max process_memory zombie parallel_sum

parallel_min_max: parallel_min_max.c $(LAB3)/utils.c $(LAB3)/find_min_max.c
	$(CC) -o parallel_min_max parallel_min_max.c $(LAB3)/utils.c $(LAB3)/find_min_max.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c

zombie: zombie.c
	$(CC) -o zombie zombie.c

sum.o: sum.c sum.h
	$(CC) -c sum.c -o sum.o $(CFLAGS)

libsum.a: sum.o
	ar rcs libsum.a sum.o

parallel_sum: parallel_sum.c libsum.a $(LAB3)/utils.c
	$(CC) -o parallel_sum parallel_sum.c $(LAB3)/utils.c -L. -lsum -pthread $(CFLAGS)

clean:
	rm -f parallel_min_max process_memory zombie parallel_sum sum.o libsum.a
