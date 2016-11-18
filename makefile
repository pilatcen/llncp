CC=gcc
CFLAGS=-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -Wall -Wno-long-long -O2 -pedantic -finline-functions -c

#only for linux
#LDFLAGS=-lpthread

#only for solaris
LDFLAGS = -mt -lnsl -lsocket -lresolv -lpthread




TARGET=llncp-server llncp llncp-clientd


all: $(TARGET)


llncp-server: libs/libllncp.o server.o
	$(CC) $(LDFLAGS) libs/libllncp.o server.o -o llncp-server

llncp: libs/libllncp.o client.o
	$(CC) $(LDFLAGS) libs/libllncp.o client.o -o llncp

llncp-clientd: libs/libllncp.o client_daemon.o
	$(CC) $(LDFLAGS) libs/libllncp.o client_daemon.o -o llncp-clientd


libs/libllncp.o:
	$(CC) $(CFLAGS) libs/libllncp.c -o libs/libllncp.o
server.o:
	$(CC) $(CFLAGS) server.c libs/libllncp.o
client.o:
	$(CC) $(CFLAGS) client.c libs/libllncp.o
client_daemon.o:
	$(CC) $(CFLAGS) client_daemon.c libs/libllncp.o


clean:
	rm -rf *.o $(TARGET)
	rm -rf libs/libllncp.o
