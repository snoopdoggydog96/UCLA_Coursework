# UCLA_Coursework

Computer Science Courses and Associated Projects: 

CS111 - Operating Systems
Course Description: http://web.cs.ucla.edu/classes/spring18/cs111/

lab_0 throgh lab_4 - the specifications for each project can be found within the folder - or alternatively one can view the CS111 spring page to verify the rigorousness of this course.
Lab Synopsis:

lab0 - Wrote a program that copied its standard input to its standard output by reading from file descriptor 0 (until EOF) and writing to file descriptor 1. If no errors (other than EOF) are encountered, the program should exit with a return code of 0. Created a makefile that compiles and creates an executable labeled lab0. Used GDB to debug and test functionality of program with various different Command Line arguments. 

lab1a - Wrote a program to read input from the keyboard character-at-a-time, no-echo mode using previous terminal mode attributes. Wrote the received characters back out to the display as they were processed. Extended and refactored program to support a --shell argument to pass input/output between the terminal and a shell. Utilized the fork() system call to create a new process, and then exec()d a shell (/bin/bash, with no arguments other than its name), whose standard input was a pipe from the terminal process, and whose standard output and standard error were (dups of) a pipe to the terminal process. Read (ASCII) input from the keyboard, echod it to stdout, and forwarded it to the shell. Read input from the shell pipe and wrote it to stdout. Used the poll() call (for IPC and Aysnchronous RPC) for strict alternation between input from the keyboard and input from the shell.

lab1b - Wrote a program to pass input & output over a TCP socket(Socket Programming). Created a client program that opened a connection to a server and sends input from the keyboard to the socket (while echoing to display) and input from the socket to the display with non-canonical terminal behavior. The server program connected to the client by listening to a network socket, received the clients commands and sent them to the shell to "serve" the client the output of commands. Extended and refactored program to utilize the standard compression library ZLIB to improve the efficiency of socket communications. 

lab2a - Wrote a multithreaded application (using pthreads) that performed parallel updates to a shared variable. Demonstrated the race condition in the provided add routine, and addressed it with different synchronization mechanisms such as mutexes and spin-locks. Did performance instrumentation and measurement.

lab2b - Wrote a multithreaded application (using pthreads) that performed parallel updates to a shared variable. Demonstrated the race condition in the provided add routine, and addressed it with different synchronization mechanisms such as mutexes and spin-locks. Did performance instrumentation and measurement. Implemented a new way to perform parallel updates by dividing the linked list into sublists and supported synchroniztion on the sublists, which allowed parallel access to the original list. Did new performance measurements to confirm the problem with synchronization mechanisms on a large list having high overhead is solved.

lab3a- Wrote a program that Reads a file system image, whose name is specified as a command line argument, analyzes the provided file system image and produces to stdout CSV summaries of what is found. 

CS117 - Computer Communications - Networks: The Physical Layer

