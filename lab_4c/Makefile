CC = gcc
CFLAGS =-Wall -Wextra -g -std=gnu99

default:
	$(CC) $(CFLAGS) lab4c_tcp.c -o lab4c_tcp -lm -lmraa
	$(CC) $(CFLAGS) lab4c_tls.c -o lab4c_tls -lm -lmraa -lssl -lcrypto

check:
	./lab4c_tcp --id=135791357 --host=131.179.192.136 --log=tcp.txt 18000
	./lab4c_tcp --id=135791357 --host=131.179.192.136 --log=tls_log.txt 19000
dist:
	tar -czvf lab4c-135791357.tar.gz README Makefile lab4c_tcp.c lab4c_tls.c
clean:
	rm -rf *.o *~ lab4c_tls lab4c_tcp lab4c-135791357.tar.gz
