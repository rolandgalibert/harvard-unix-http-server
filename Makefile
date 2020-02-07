#
# File:         Makefile for wsng
# Author:       Roland L. Galibert
#               Problem Set 6
#               For Harvard Extension course CSCI E-28 Unix Systems Programming
# Date:         May 2, 2015
# 
# This make file contains instructions for creating the wsng executable
#
# Cleanup rules # are also provided for removing executables and object files
# in order to faciliate debugging and recompilation.

# Compilation options
CC = gcc
CFLAGS += -Wall

all: wsng

# wsng
wsng: wsng.o conf.o flexstr.o httplib.o socklib.o splitline.o urllib.o
	$(CC) $(CFLAGS) -o wsng wsng.o conf.o flexstr.o httplib.o socklib.o splitline.o urllib.o -lcrypto -lssl

wsng.o: wsng.c
	$(CC) $(CFLAGS) -c wsng.c
	
# conf
conf.o: conf.c
	$(CC) $(CFLAGS) -c conf.c

# flexstr
flexstr.o: flexstr.c
	$(CC) $(CFLAGS) -c flexstr.c

# httplib
httplib.o: httplib.c
	$(CC) $(CFLAGS) -c httplib.c
	
# socklib
socklib.o: socklib.c
	$(CC) $(CFLAGS) -c socklib.c
	
# splitline
splitline.o: splitline.c
	$(CC) $(CFLAGS) -c splitline.c
	
# urllib
urllib.o: urllib.c
	$(CC) $(CFLAGS) -c urllib.c

clean:
	rm -f wsng *.o core
