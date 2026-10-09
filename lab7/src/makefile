CC=gcc

all: tcpclient tcpserver udpclient udpserver

tcpclient: tcpclient.c
	$(CC) -o tcpclient tcpclient.c

tcpserver: tcpserver.c
	$(CC) -o tcpserver tcpserver.c

udpclient: udpclient.c
	$(CC) -o udpclient udpclient.c

udpserver: udpserver.c
	$(CC) -o udpserver udpserver.c

clean:
	rm -f tcpclient tcpserver udpclient udpserver
