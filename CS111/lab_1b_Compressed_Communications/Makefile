#NAME: Anup Kar
#UID: 204419149
#EMAIL: akar@g.ucla.edu
CC = gcc
CFLAGS = -std=gnu11 -g -Wall -Wextra -Wno-unused-parameter -Wpointer-sign
FILES   = lab1b-server.c lab1b-client.c Makefile README 

all:
	$(CC) $(CFLAGS) lab1b-server.c -lz -o lab1b-server
	$(CC) $(CFLAGS) lab1b-client.c -lz -o lab1b-client
dist: 
	tar -czvf lab1b-204419149.tar.gz $(FILES)
clean:
	rm -f lab1b-server lab1b-client lab1b-204419149.tar.gz 


