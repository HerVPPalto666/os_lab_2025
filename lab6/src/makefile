CC=gcc
CFLAGS=-I.
LDFLAGS=-pthread

all: client server

common.o: common.c common.h
	$(CC) -c common.c -o common.o $(CFLAGS)

libcommon.a: common.o
	ar rcs libcommon.a common.o

client: client.c libcommon.a
	$(CC) -o client client.c -L. -lcommon $(CFLAGS) $(LDFLAGS)

server: server.c libcommon.a
	$(CC) -o server server.c -L. -lcommon $(CFLAGS) $(LDFLAGS)

clean:
	rm -f client server common.o libcommon.a
