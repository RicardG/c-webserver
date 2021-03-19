#Random Make file
all : WebServer

CC=gcc
CFLAGS=-Wall -Werror -lm -ggdb

WebServer : webServer.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
webServer.o : webServer.c webServer.h
clean : 
	rm -f server client WebServer *.o
