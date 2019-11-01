#NAME: Anup Kar
#EMAIL: akar@g.ucla.edu
#ID: 204419149
CC = gcc
CFLAGS = -std=gnu11 -g -O2 -Wall -Wextra 
FILES   = lab0.c Makefile README breakpoint.png backtrace.png ./P0_check.sh

.SILENT:

lab0: lab0.c
	$(CC) $(CFLAGS) -o lab0 lab0.c

check: lab0 smoketests

dist: 
	tar -czvf lab0-204419149.tar.gz $(FILES)
clean:
	rm -f lab0 lab0-204419149.tar.gz 

smoketests: checkSuccess checkBadArg checkCreat checkOpen checkCatchSegFault

checkSuccess:
	echo "Hello World!" > inputFile.txt;\
	./lab0 --input=inputFile.txt --output=outputFile.txt ; \
	if [ $$? -ne 0 ] ; then	echo "checkSuccess failed" ; fi;\
	rm *.txt
checkBadArg:
	./lab0 --badargument ;\
	if [ $$? -ne 1 ]; then echo "There was an error handling the unrecognized argument, exited"; fi ;\

checkCreat:
	echo "Hello World!" > test_in.txt;\
	touch test_out.txt;\
	chmod u-w test_out.txt ;\
	./lab0 --input=test_in.txt --output=test_out.txt;\
	if [ $$? -ne 3 ] ; then	echo "checkCreat failed with exit $$?"; fi;\
	chmod u+w test_out.txt
	rm *.txt
checkOpen:
	echo "Hello World" > test_in.txt;\
	chmod ugo-r test_in.txt;\
	./lab0 --input=test_in.txt --output=test_out.txt;\
	if [ $$? -ne 2 ]; then echo "checkOpen failed with exit $$?"; fi;\
	chmod 777 test_in.txt;\
	rm *.txt;
checkCatchSegFault:
	./lab0 --segfault --catch;\
	if [ $$? -ne 4 ]; then echo "checkSegFaultAndCatch with exit $$?"; fi;\

