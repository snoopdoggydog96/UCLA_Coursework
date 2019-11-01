/*
NAME: Anup Kar
EMAIL: akar@g.ucla.edu
UID: 204419149
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <poll.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

/* USER IMPLEMENTATION MUSTS! 
1. The application must initialize zalloc, zfree and opaque before calling the init function
2. The application must update next_in and avail_in when avail_in has dropped to zero
3. The application must update next_out and avail_out when avail_out has dropped to zero
4. The fields total_in and total_out can be used for statistics or progress reports. After compression, total_in holds the total size of the uncompressed data and may be saved for use by the decompressor (particularly if the decompressor wants to decompress everything in a single step
*/
/*INSPIRATION FROM int def(FILE *source, FILE *dest, int level) from the zlib tutorial provided by Professor:
 Compress from file source to file dest until EOF on source.
 defWrapper() returns byes compressed on success, -1 if deflate() does not properly reach Z_STREAM_END in one pass (Z_STREAM_FINISH)*/
/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int
defWrapper(void *in, int inlen, void *out, int outlen, int level)
{
  z_stream strm;
  int ret = -1;
  int z_err = -1;
  strm.total_in = strm.avail_in = inlen;
  strm.total_out = strm.avail_out = outlen;
  strm.next_in = (unsigned char*) in; /* next_in byte is of type Byte *f so we need to cast the void * to Bytef*/
  strm.next_out = (unsigned char*) out; /* same for next_out */
  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, level);
  if (ret == Z_OK)
    {
      ret = deflate(&strm, Z_FINISH); /* Z_FINISH, deflate only be fed one char <= sizeof(buf), so deflate in one pass*/
      if (ret == Z_STREAM_END) z_err = (int)strm.total_out; /*remember total_out is of type ulong */
      else
	{
	  deflateEnd(&strm);
	  return ret;/* there was an issue during deflation process */
	}
    }
  else
    {
      deflateEnd(&strm);
      return ret;
    }
  deflateEnd(&strm);
  return z_err;
}
/*INSPIRATION FROM int int inf(FILE *source, FILE *dest) from the zlib tutorial provided by Professor:
 Decompress from file source to file dest until stream ends or EOF.
 inf() returns WHAT? on successful decompression or -1 when encountering an error e.g. not properly reaching Z_STREAM_END in one pass.*/
int
infWrapper(void *in, int inlen, void *out, int outlen)
{
  z_stream strm;
  int ret = -1;
  int z_err = -1;
  strm.total_in = strm.avail_in = inlen;
  strm.total_out = strm.avail_out = outlen;
  strm.next_in = (unsigned char*) in; /* next_in byte is of type Byte *f so we need to cast the void * to Bytef*/
  strm.next_out = (unsigned char*) out; /* same for next_out */
  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  z_err = inflateInit(&strm);
  if (z_err == Z_OK)
    {
      z_err = inflate(&strm, Z_FINISH); /* shell processing char at a time, but doing large reads use Z_FINISH, deflate only be fed one char <= sizeof(buf*/
      if (z_err == Z_STREAM_END) ret = (int)strm.total_out; /*remember total_out is of type ulong */
      else
	{
	  inflateEnd(&strm);
	  return z_err; /* there was an issue during deflation process */
	}
    }
  else
    {
      inflateEnd(&strm);
      return z_err;
    }
  inflateEnd(&strm);
  return ret; 
}

int
def(FILE *source, FILE *dest, int level)
{
  int ret, flush;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, level);
  if (ret != Z_OK)
    return ret;

  /* compress until end of file */
  do {
    strm.avail_in = (uint)fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      return Z_ERRNO;
    }
    flush = feof(source) ? Z_FINISH : Z_SYNC_FLUSH; /* THIS IS THE APPRORIATE FLUSH TYPE */
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, flush);    /* no bad return value */
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
	(void)deflateEnd(&strm);
	return Z_ERRNO;
      }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);     /* all input will be used */

    /* done when last data in file processed */
  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END);        /* stream will be complete */

  /* clean up and return */
  (void)deflateEnd(&strm);
  return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int
inf(FILE *source, FILE *dest)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK)
    return ret;

  /* decompress until deflate stream ends or end of file */
  do {
    strm.avail_in = (uint)fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)inflateEnd(&strm);
      return Z_ERRNO;
    }
    if (strm.avail_in == 0)
      break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      switch (ret) {
      case Z_NEED_DICT:
	ret = Z_DATA_ERROR;     /* and fall through */
	(void)inflateEnd(&strm);
	return ret;
	break; 
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
	(void)inflateEnd(&strm);
	return ret;
      }
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
	(void)inflateEnd(&strm);
	return Z_ERRNO;
      }
    } while (strm.avail_out == 0);

    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  /* clean up and return */
  (void)inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void
zerr(int ret)
{
  fputs("zpipe: ", stderr);
  switch (ret) {
  case Z_ERRNO:
    if (ferror(stdin))
      fputs("error reading stdin\n", stderr);
    if (ferror(stdout))
      fputs("error writing stdout\n", stderr);
    break;
  case Z_STREAM_ERROR:
    fputs("invalid compression level\n", stderr);
    break;
  case Z_DATA_ERROR:
    fputs("invalid or incomplete deflate data\n", stderr);
    break;
  case Z_MEM_ERROR:
    fputs("out of memory\n", stderr);
    break;
  case Z_VERSION_ERROR:
    fputs("zlib version mismatch!\n", stderr);
  }
}


/* error handler functions after we have detected a system call has returned incorrectly in our main program */
void 
socket_error(void)
{
  fprintf(stderr,"Error opening socket. Exiting Immediately");
  exit(1); 
}
void 
accept_error(void)
{
  fprintf(stderr,"Error accepting connection. Exiting Immediately");
  exit(1); 
}
void 
bind_error(void)
{
  fprintf(stderr,"Error binding socket to port #. Exiting Immediately"); 
  exit(1); 
}
void
pipe_error(int i_o)
{
  switch (i_o)
    {
    case 1:
      fprintf(stderr, "pipe() failed piping STDOUT. Exit. \n");
      exit(1);
    case 0:
      fprintf(stderr, "pipe() failed piping STDIN. Exit. \n");
      exit(1);
    default:
      fprintf(stderr, "pipe() failed. Exit. \n");
      exit(1);
    }
}

void
exec_error(void)
{
  fprintf(stderr, "exe() failed. Exit.\n");
  exit(1); 
}

void
dup2_error(int i_o)
{
  switch (i_o)
    {
    case 1:
      fprintf(stderr, "dup2() failed copying STDOUT to pipe. Exit. \n");
      exit(1);
    case 0:
      fprintf(stderr, "dup2() failed copying STDIN to pipe. Exit. \n");
      exit(1);
    case 2:
      fprintf(stderr, "dup2() failed copying STDERR to pipe. Exit. \n");
      exit(1); 
    default:
      fprintf(stderr, "dup2() failed. Exit. \n");
      exit(1);
    }
}

void
exit_msg(void)
{
  fprintf(stderr, "EXIT SUCCESS!");
  exit(0); 
}
void
read_fail(void)
{
  fprintf(stderr,"Not able to read from stdin. Reason: %s \n", strerror(errno));
  exit(1); 
}

void 
close_error(void)
{
  fprintf(stderr, "Error closing specified FD. Reason: %s \n", strerror(errno)); 
  exit(1); 
}

void 
wait_error(void)
{
  fprintf(stderr, "Child process failed returning. Exiting immediately.\n");
  exit(1); 
}


/* function to see if ^C was input by user in any of the char buffs[] from our read */
int
ctrl_c(char *buff, int rc)
{
  if (buff == NULL || rc <= 0) return 0; 
  else
    {
      char *ctrl_c = buff; 
      while (rc > 0)
	{
	  if (*(ctrl_c) == 3) return 1; 
	  else
	    ctrl_c++; 
	  rc --;
	}
    }
  return 0; 
}

/* function to see if ^D was input by user in any of the char buffs[] from our read */
int
ctrl_d(char *buff, int rc)
{
  if (buff == NULL || rc <= 0) return 0; 
  else
    {
      char *ctrl_d = buff; 
      while (rc > 0)
	{
	  if (*(ctrl_d) == 4) return 1; 
	  else
	    ctrl_d++;
	  rc--;
	}
    }
  return 0; 
}
void 
user_error(void)
{
  fprintf(stderr, "Usage: ./lab1a-server --port [port #], optional --compress\n"); 
  exit(1); 
}
void 
poll_error(void)
{
  fprintf(stderr, "Error polling FD's, exiting immediately \n"); 
  exit(1); 
}
void
sighandler(int sigNum)
{
  fprintf(stderr, "Broken Pipe! Attempting to read or write to a closed pipe, exiting with signal #: %d \n", sigNum); 
  exit(1); 
}

int 
main(int argc, char **argv)
{
  char c;
  int port = 0;
  int sockfd, newsockfd, portno, compress;
  socklen_t clilen; 
  signal(SIGPIPE, sighandler); 
  portno = 0; 
  compress = 0; 
  struct sockaddr_in serv_addr, cli_addr;  
    /*using getopt_long to parse commond line options and check for --bogus args */
     static struct option long_options[] =
       {
	 {"port", required_argument, 0,'p'},
	 {"compress", no_argument, 0, 'c'},
	 {0        , 0              , 0, 0}
       };
     while (1)
       {
	 c = getopt_long(argc, argv, "", long_options, 0);
	 if (c==-1) break;
	 switch(c)
	   {
	   case 'p':
	     port = 1;
	     portno = atoi(optarg); 
	     break;
	   case 'c':
	     compress = 1; 
	     break; 
	   default:
	     fprintf(stderr,"Usage: ./lab1a-server --port [port #], optional --compress"); 
	     exit(1);
	     break;
	   }
       }
     
     if (port != 1) user_error(); 
    
     /* initializing socket for accepting connections to client to read data. */ 
     sockfd = socket(AF_INET, SOCK_STREAM, 0); 
     if (sockfd < 0) socket_error(); 
    
     bzero((char *) &serv_addr, sizeof(serv_addr)); /* use newer function */ 

     /* setting serv_addr struct members to make sure TCP/IP is the protocol used and using htons(portno) to make the host byte # into a network byte # */
     serv_addr.sin_family = AF_INET; 
     serv_addr.sin_addr.s_addr = INADDR_ANY; 
     serv_addr.sin_port = htons(portno);

     /* bind sockfd to port # (allows socket to listen to any inc. requests after above initialization*/

     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) bind_error();
     /* listen to the socket for any incoming requests, maximum of 5 at a time*/
     listen(sockfd, 5);
     clilen = sizeof(cli_addr);

     /* accept returns a new FD THAT ALLOWS COMMUNICATION BETWEEN THE CLIENT AND SERVER! */ 
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); 
     /*newsockfd is now our "stdin". Close current stdin and dup2(newsockfd, 0) so stdin and newsockfd refer to same FD to communicate */ 
     close(0); 
     if (dup2(newsockfd, 0) == -1) dup2_error(2); 
     /*2 pipes for bidirectional I/O, client to Server, Server to shell processs */
     int to_shell[2];
     int from_shell[2];
     if (newsockfd < 0) accept_error(); 

     if (pipe(to_shell) == -1) pipe_error(1); /*check bad pipe() sys call*/
     if (pipe(from_shell) == -1) pipe_error(0); /* check bad pipe() sys call */ 
     
    
     int child = fork();
	 /* checking return value of fork() to process identify whether it is parent or child */ 
     if (child == 0) /*CHILD PROCESS */
       {
	 /*close FDS not necessary: 6 open fds. Close parent process STDIN and close Write end of pipe to child process */
	 close(to_shell[1]);
	 close(STDIN_FILENO);

	 /*read_end of pipe should be copy of STDIN which is duped to newsockfd so Client and Server can pass data*/
	 if(dup2(to_shell[0], 0) == -1) dup2_error(0); 

	 /*Close any fds not used i.e. read end of child pipe and child process STDOUT */
	 close(from_shell[0]);
	 close(STDOUT_FILENO);

	 /*write end of child pipe (shell) should be new STDOUT */
	 if(dup2(from_shell[1], STDOUT_FILENO) == -1) dup2_error(1);

	 /*write end of child pipe (shell error outputs that go to stderr) should be new stderr*/
	 close(STDERR_FILENO);
	 if(dup2(from_shell[1], STDERR_FILENO) == -1) dup2_error(2);/*check bad dup2() sys call attempting to copy pipe STDERR to from_child[1] */
  
	 
	 /* child process executes the shell after closes/dups all pipes for IPC*/
	 char *execvp_argv[2];
	 char execvp_filename[] = "/bin/bash"; 
	 execvp_argv[0] = execvp_filename; 
	 execvp_argv[1] = NULL;
	 if (execvp(execvp_filename, execvp_argv) == -1) exec_error();  /*check bad execvp() sys call */
       }	     
     else
       {
	 /*similarly to project 1a, poll file descriptors back and forth
	   
	   b. anything in read end of child pipe to be processed by Shell */
	 struct pollfd pollfds[2];
	 /* a. Poll newsockfd for anything sent by client to the server */
	 pollfds[0].fd = newsockfd; 
	 pollfds[0].events = POLLIN | POLLHUP | POLLERR; 
	 /* b. Poll Read end of Child Process to see if any data must be proecessed by Child PRocess */  
	 pollfds[1].fd = from_shell[0]; 
	 pollfds[1].events = POLLIN | POLLHUP | POLLERR;
	 /* BECAUSE THIS IS THE PARENT PROCESS! we need to re-close unneccesary FDS not closed by child proces */
	 if (close(to_shell[0])== -1) close_error(); /*Close Parent Process STDIN, was already dup2ed to newsockfd and to_shell[1] */
	 if (close(from_shell[1]) == -1) close_error();  /*write end of from_shell[1], Parent Process STDOUT/STDERR not needed becuase child ddup2ed STDOUT/STDERR is what we want to send to client*/ 
	 int read_count = 0; 
	 char buff[255]; /*store intial reads in buff*/ 
	 char shellBuff[510]; 
	 int shell_count = 0;
	 unsigned char in[CHUNK]; 
	 unsigned char dest[CHUNK]; 
	 while(1)
	   {
		 int polled = poll(pollfds, 2, 0);
		 if (polled < 0) poll_error();
		 
		 if (pollfds[0].revents & POLLIN)
		   {
		     fprintf(stderr, "client sent data/server received.\n");
		     memset(buff, 0, 255*sizeof(char)); 
		     read_count = read(newsockfd, buff, sizeof(buff)); 
		     
		     if (read_count < 0) read_fail(); 
		     /* if no compress flag, do same thing done in Project1a */
		     if (compress != 1)
		       {
			 int c = ctrl_c(buff, read_count);  
			 int d = ctrl_d(buff, read_count); 
			 if (d==1) close(to_shell[1]); /* if ^D received, close write end of pipe to shell and begin Shutdown Processing */
			 if (c==1) kill(child, SIGINT); 
			 if (read_count == 0) close(to_shell[1]); /*no bytes to read, so shutdown Processing will be triggered */
			 shell_count = 0; 
			 int j;
			 /* mapping \r to \n */
			 for (j = 0; j < read_count; j++)
			   {
			     if ((buff[j] == '\r') && (buff[j+1] == '\n')) 
			       {
				 shellBuff[shell_count] = '\n'; 
				 shell_count++; 
			       }
			     else 
			       {
				 shellBuff[shell_count] = buff[j];
				 shell_count++; 
			       }
			   }
			 /* write from shellbuff to write end of pipe to shell (contains sent client data)*/
			 write(to_shell[1], shellBuff, shell_count); 
		       }
		     else
		       {
			 int ret = infWrapper(buff, (int)read_count, dest, CHUNK); 
			 /*  NONCANON mode, received data was sent 1 byte at a time. Mmanually check for ^D or ^C and translate \r to \n to write to shell */ 			
			 if (dest[0] == 0x04) close(to_shell[1]); 
			 if (dest[0] == 0x03) kill(child, SIGKILL);
			 if (dest[0] == '\r') dest[0] = '\n';
			 if (ret==0)  close(to_shell[1]); /* No bytes to read. Close Pipe to trigger shutdown processing */
			 /* else write whatever decompressed data sent fron client (in dest) to write end of Parent Process duped to Child Process stdin*/
			 write(to_shell[1], dest, 1); 
		       }
		   }
		   
		 if (pollfds[1].revents & POLLIN)
		   {
		     read_count = read(from_shell[0], buff, sizeof(buff)); 
		     if (read_count < 0) read_fail(); 
		     if (read_count==0)
		       {
			 pid_t childRet; 
			 int status; 
			 childRet = waitpid(child, &status, WUNTRACED | WCONTINUED); 
			 if (childRet == -1) wait_error();
			 /* SHUT DOWN PROCESSING from Child receiving ^C or ^D*/
			 if (WIFEXITED(status))
			   {
			     int exitStatus = WEXITSTATUS(status);
			     fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n\r", exitStatus, exitStatus);
			     close(sockfd); /* close connection between client and server */
			     close(newsockfd);/* close file descriptor that allows communication b/w client and server */
			     kill(child, SIGINT); 
			     exit(0); 
			   }
			 /* SHUT DOWN PROCESSING from external signal*/
			 else if (WIFSIGNALED(status))
			   {
			     fprintf(stderr, "Program terminted with Signal=%d \n", WTERMSIG(status));
			     close(sockfd); /* close connection between client and server */
			     close(newsockfd);/* close file descriptor that allows communication b/w client and server */ 
			     exit(1); 
			   }
			 fprintf(stderr, "Child Process never exited or signaled, exit failure");
			 close(sockfd); /* close connection between client and server */
			 close(newsockfd); /* close file descriptor that allows communication b/w client and server */
			 exit(1); 
		       }
		     /* no compression just write client data to be sent to shell */ 
		     if (compress != 1) write(newsockfd, buff, read_count); 
		     else 
		       {
			 
			 int ret = defWrapper(buff, 255, in, CHUNK, Z_DEFAULT_COMPRESSION); 
			 if (ret < 0) 
			   {
			     fprintf(stderr, "defBuffer failed compressing Data. Exiting Immediately.\n"); 
			     exit(1); 
			   }
			 write(newsockfd, in, ret); 
		       }
		   }
		 /* THIS IS HOW WE DEAL WITH BROKEN PIPE! Child closed to_shell[1] when ^D is encountered but Parent process never closed it so we receive a SIGPIPE error*/ 
		 if((pollfds[0].revents & POLLHUP) || (pollfds[0].revents & POLLERR)) close(to_shell[1]); 

		 /* SHUTDOWN PROCESSING AFTER ^D/^C sent to harvest any remaining output in read end of pipe from_shell (output of shell) */
		 if((pollfds[1].revents & POLLHUP) || (pollfds[1].revents & POLLERR))
		   {
		     read_count = read(from_shell[0], buff, sizeof(buff));
		     if (read_count < 0) read_fail();
		     write(newsockfd, buff, read_count); 
		     pid_t childRet; 
		     int status; 
		     childRet = waitpid(child, &status, WUNTRACED | WCONTINUED); 
		     if (childRet == -1) wait_error(); 
		     if (WIFEXITED(status))
		       {
			 int exitStatus = WEXITSTATUS(status);
			 fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n\r", exitStatus, exitStatus);
			 
			 exit(0); 
		       }
		     else if(WIFSIGNALED(status))
		       {
			 fprintf(stderr, "Program terminted with Signal #: %d \n", WTERMSIG(status));
			 exit(1); 
		       }
		     fprintf(stderr, "Child Process never exited or signaled, exit failure"); 
		     exit(1);  
		   }
		 
	   }	       
	 
       }
     
}

