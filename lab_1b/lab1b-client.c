/*
NAME: Anup Kar
EMAIL: akar@g.ucla.edu
UID: 204419149
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h> /* needed for polling */
#include <sys/stat.h> /* used for open call */
#include <fcntl.h>
#include <signal.h>
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
struct termios saved; 
/*
typedef struct z_stream_s {
z_const Bytef *next_in;      next input byte
uInt     avail_in;   number of bytes available at next_in
uLong    total_in;  total number of input bytes read so far

Bytef    *next_out;  next output byte will go here
uInt     avail_out;  remaining free space at next_out
uLong    total_out;  total number of bytes output so far

z_const char *msg;   last error message, NULL if no error
struct internal_state FAR *state;  not visible by applications

alloc_func zalloc;   used to allocate the internal state
free_func  zfree;    used to free the internal state
voidpf     opaque;   private data object passed to zalloc and zfree

int     data_type;   best guess about the data type: binary or text
                     for deflate, or the decoding state for inflate
uLong   adler;       Adler-32 or CRC-32 value of the uncompressed data
uLong   reserved;    reserved for future use
} z_stream;
 USER IMPLEMENTATION MUSTS!
1. The application must initialize zalloc, zfree and opaque before calling the init function
2. The application must update next_in and avail_in when avail_in has dropped to zero
3. The application must update next_out and avail_out when avail_out has dropped to zero
4. The fields total_in and total_out can be used for statistics or progress reports. After compression, total_in holds the total size of the uncompressed data and may be saved for use by the decompressor (particularly if the decompressor wants to decompress everything in a single step INSPIRATION FROM int def(FILE *source, FILE *dest, int level) from the zlib tutorial provided by Professor:
 Compress from file source to file dest until EOF on source.
 defWrapper() returns byes compressed on success, -1 if deflate() does not properly reach Z_STREAM_END in one pass (Z_STREAM_FINISH) Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files.
 */
int 
defWrapper(void *in, int inlen, void *out, int outlen, int level)
{
  z_stream strm;
  int ret = -1;
  int z_err = -1;
  strm.total_in = strm.avail_in = inlen;
  strm.total_out = strm.avail_out = outlen;
  strm.next_in = (unsigned char *) in; /* next_in byte is of type Byte *f so we need to cast the void * to Bytef*/
  strm.next_out = (unsigned char *) out; /* same for next_out */
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
    strm.next_in = (unsigned char *) in; /* next_in byte is of type Byte *f so we need to cast the void * to Bytef*/
    strm.next_out = (unsigned char *) out; /* same for next_out */
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

    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do
      {
	strm.avail_out = CHUNK;
	strm.next_out = out;
	ret = deflate(&strm, flush);
	assert( ret != Z_STREAM_ERROR);
	have = CHUNK - strm.avail_out;
	if (fwrite(out, 1, have, dest) != have || ferror(dest))
	  {
	    (void)deflateEnd(&strm);
	    return Z_ERRNO;
	  }
      }while (strm.avail_out ==0);
    assert(strm.avail_in == 0);
  }while(flush != Z_FINISH);
  assert(ret == Z_STREAM_END); 
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

/* END OF ZLIB FUNCTIONS */

void
pipe_error(int i_o)
{
  switch (i_o)
    {
    case 1:
      fprintf(stderr, "pipe() failed piping STDOUT. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1);
    case 0:
      fprintf(stderr, "pipe() failed piping STDIN. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1);
    default:
      fprintf(stderr, "pipe() failed. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1);
    }
 }



/* END OF ZLIB FUNCTION USE */
void
exec_error(void)
{
  fprintf(stderr, "exe() failed. Exit.\n");
  tcsetattr(0, TCSANOW, &saved);
  exit(1); 
}
void 
write_socket_fail(void)
{
  fprintf(stderr, "Error writing to socket. Exiting immediately. \n");
  tcsetattr(0, TCSANOW, &saved);
  exit(1); 
}
void
dup2_error(int i_o)
{
  switch (i_o)
    {
    case 1:
      fprintf(stderr, "dup2() failed copying STDOUT to pipe. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1);
    case 0:
      fprintf(stderr, "dup2() failed copying STDIN to pipe. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1);
    case 2:
      fprintf(stderr, "dup2() failed copying STDERR to pipe. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1); 
    default:
      fprintf(stderr, "dup2() failed. Exit. \n");
      tcsetattr(0, TCSANOW, &saved);
      exit(1);
    }
}
void 
poll_error(void)
{
  fprintf(stderr, "Error polling! Exiting Immediately."); 
  tcsetattr(0, TCSANOW, &saved); 
  exit(1); 
} 

void
exit_msg(void)
{
  fprintf(stderr, "EXIT SUCCESS!");
  tcsetattr(0, TCSANOW, &saved); 
  exit(0); 
}

void
read_fail(void)
{
  fprintf(stderr,"Not able to read from stdin. Reason: %s \n", strerror(errno));
  tcsetattr(0, TCSANOW, &saved); 
  exit(1); 
}

void 
close_error(void)
{
  fprintf(stderr, "Error closing"); 
  tcsetattr(0, TCSANOW, &saved); 
  exit(1); 
}

void
hostname_error(void)
{
  fprintf(stderr, "Error getting hostname. Exiting immediately.");
  tcsetattr(0, TCSANOW, &saved); 
  exit(1); 
}

void
read_socket_error(void)
{
  fprintf(stderr, "Error reading from socket. Exiting Immediately.");
  tcsetattr(0,TCSANOW,&saved); 
  exit(1); 
}

void 
connect_error(void)
{
  fprintf(stderr, "Error connecting to Port. Exiting Immediately.");
  tcsetattr(0, TCSANOW, &saved); 
  exit(1); 
}

void
socket_error(void)
{
  fprintf(stderr, "Error creating socket. Exiting Immediately");
  tcsetattr(0, TCSANOW, &saved); 
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
  fprintf(stderr, "No Port # Provided. Exiting Immediately. \n"); 
  exit(1); 
}
void 
sighandler(int sigNum)
{
  fprintf(stderr,"Program terminated with Signal #: %d\n", sigNum);
  exit(1); 
}

int main(int argc, char **argv)
{
  char c;

  int sockfd, read_count;  
  struct sockaddr_in serv_addr; 
  struct hostent *server;
  int portno = 0; 
  int port = 0; 
  int compress = 0; 
  int log = 0; 
  char *logfile;
  signal(SIGPIPE, sighandler); 
    /*using getopt_long to parse commond line options and check for --bogus args */
     static struct option long_options[] =
       {
	 {"port", required_argument, 0,'p'},
	 {"log", required_argument, 0, 'l'}, 
	 {"compress", no_argument, 0, 'c'}, 
	 {0,          0,         0, 0}
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
	   case'l':
	     log = 1; 
	     logfile = optarg; 
	     break; 
	   case 'c':
	     compress = 1; 
	     break; 
	   default:
	     fprintf(stderr,"Usage: ./lab1a --port [port #], optional flags --log [logfilename] --compress"); 
	     exit(1);
	     break;
	   }
       }
     int logFD = -1;
     if (port != 1) user_error(); 
     if (log != 0) 
       {
	 logFD = open(logfile, O_RDWR | O_APPEND | O_CREAT, 0666);
	 if (logFD < 0)
	   {
	     fprintf(stderr, "Error opening file\n");
	     tcsetattr(0, TCSANOW, &saved); 
	     exit(1); 
	   }
       }
     /*2 termios variables, one to hold old terminal settings, one to set new terminal settings*/
     struct termios new;

     /*get old terminal settings, edit them to make them non-canonical input mode with no echo */
     tcgetattr(0, &saved); 
     /*copy old to new - allows easily restoring terminal attributes upon exit */  
     new = saved;  
     
     /* FROM SPEC :enables no-echo, character-at-a-time, for terminal attributes */
     /* e.g. NON-CANONICAL INPUT MODE WITH NO ECHO */
     new.c_iflag = ISTRIP;/* only lower 7 bits */ 
     new.c_oflag = 0;/*no processing*/
     new.c_lflag = 0;/*no processing*/
     
     new.c_cc[VTIME] = 0; /*no waiting*/
     new.c_cc[VMIN] = 1; /*1 char at a time*/

     /*set terminal attributes of STDIN_FILENO NOW (TCSANOW), to non-canonical input mode with no echo (&new) */
     tcsetattr(0, TCSANOW, &new);
     /*sockfd is FD to communicate w/ Server */     
     sockfd = socket(AF_INET, SOCK_STREAM, 0); 
 
     if (sockfd < 0) socket_error();
      
     /* save hostname in struct honstnet */
      server = gethostbyname("localhost");
      
      if (server == NULL) hostname_error();
      /*initialize the struct serv_addr_in to attempt to connect to the Server at the IP address specified and Port # */
      memset((char*) &serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET; 
      memcpy(server->h_addr,
	    (char *)&serv_addr.sin_addr.s_addr,
	    server->h_length); 
      serv_addr.sin_port = htons(portno);
      /* ensure succesful connection to server */
      if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) connect_error(); 

      char buff[255]; /*store intial reads in buff*/
      read_count = 0; 
      char mod_buff[510];
      int mod_count = 0;
      /* Poll between input from keyboard and shell output that is sent back from server over sockfd */
      struct pollfd pollfds[2]; 
      pollfds[0].fd = STDIN_FILENO; 
      pollfds[0].events = POLLIN | POLLHUP | POLLERR;

      pollfds[1].fd = sockfd; 
      pollfds[1].events = POLLIN | POLLHUP | POLLERR;
      unsigned char dest[CHUNK];
      
      while(1)
	{
	  int polled = poll(pollfds, 2, 0);
	  if (polled < 0) poll_error();
	  /* user passed input to STD_IN to be passed to server through sockfd */
	  if (pollfds[0].revents & POLLIN)
	    {
	      memset(buff, 0, sizeof(buff)); 
	      read_count = read(0, buff, sizeof(buff)); 
	      if (read_count < 0) read_fail(); 
	      /* Log for data sent between Client and Server w/o Compression */
	      if (log != 0)
		{
		  if (compress==0)
		    {
		      char log_msg[14] = "SENT x BYTES: "; 
		      log_msg[5] = '0' + read_count;
		      write(logFD,log_msg,sizeof(log_msg));
		      write(logFD, buff, read_count); 
		      write(logFD, "\n", 1); 
		    }
		}
	      if (compress != 0)
		{
		  /*User typed to keyboard so compress data to send to server */
		  int ret = defWrapper(buff, 255, dest, CHUNK, Z_DEFAULT_COMPRESSION); 
		  if (ret < 0) 
		    {
		      fprintf(stderr, "issue decompressing. Exiting Immediately \n. "); 
		      tcsetattr(0, TCSANOW, &saved);
		      exit(1);
		    }
		  /* write compressed data to Server */
		  write(sockfd, dest, ret);
		  /* Log for data sent between Client and Server w Compression */
		  if(log != 0)
		    {
		      char log_msg[14] = "SENT x BYTES: "; 
		      log_msg[5] = '0' + ret;
		      write(logFD, log_msg, 14);
		      write(logFD, dest, sizeof(log_msg)); 
		      write(logFD, "\n", 1); 
		    }
		}
	      
	      mod_count = 0; 
	      int i;
	      for (i = 0; i < read_count; i++)
		{
		  if((buff[i] == '\n') | (buff[i] == '\r'))
		    {
		      mod_buff[mod_count] = '\r';
		      mod_count++;
		      mod_buff[mod_count] = '\n'; 
		      mod_count++; 
		    }
		  else
		    {
		      mod_buff[mod_count] = buff[i]; 
		      mod_count++; 
		    }
		}
	      /* Still must display what Client is typing with <cr> or <lf> mapping to <r><n> so just write that to STDOUT*/
	      write(1, mod_buff, mod_count);
	      /*if we do not need to compress simply send the modified keyboard input to Server for shell processing */
	      if(compress == 0) write(sockfd, mod_buff, mod_count); 

	    }
	  /* Their is pending input becuase Server processed client input and has produced output */
	  if (pollfds[1].revents & POLLIN)
	    {
	      read_count = read(sockfd, buff, sizeof(buff)); 
	      if (read_count < 0) read_fail(); 
	      if (read_count==0) exit_msg();
	      /* Logging whats received by Client from Server w/o Compression */
	      if (log != 0)
		{
		  char log_msg[20];
		  sprintf(log_msg, "RECEIVED %d BYTES: ",(int)read_count);
		  write(logFD, log_msg, sizeof(log_msg));
		  write(logFD, buff, read_count); 
		  write(logFD, "\n", 1); 
		}
	      /* Decompress Shell Output sent from Server */
	      if (compress !=0)
		{
		  int ret = infWrapper(buff, 255, dest, CHUNK); 
		  mod_count = 0;
		  int i;
		  /* after decompression do \n conversion to \r\n */
		  for (i = 0; i < ret; i++)
		    {
		      if(dest[i] == '\n')
			{
			  mod_buff[mod_count] = '\r';
			  mod_count++;
			  mod_buff[mod_count] = '\n';
			  mod_count++;
			}
		      else
			{
			  mod_buff[mod_count] = dest[i]; 
			  mod_count++;
			}
		    }
		  if(write(1, mod_buff, mod_count) <0) fprintf(stderr, "writing mod_buf to buffer \n");
		}
	      else
		{
		  /* simmply write to STDOUT the Server Shell output with correct \n to \r\n mapping */
		  mod_count = 0; 
		  int i; 
		  for (i = 0; i < read_count; i++)
		    {
		      if(buff[i] == '\n')
			{
			  mod_buff[mod_count] = '\r'; 
			  mod_count++; 
			  mod_buff[mod_count] = '\n'; 
			  mod_count++; 
			}
		      else 
			{
			  mod_buff[mod_count] = buff[i]; 
			  mod_count++; 
			}
		    }
		  write(1, mod_buff, mod_count); 
		}
	    }
	  /* DEALING WITH A SIGNAL INTERRUPTING SHELL e.g. broken pipe, etc... so harvest shell output and do correct conversion of \n to \r\n. write it to display reset terminal attributes and exit(1)*/ 
	  if((pollfds[1].revents & POLLHUP) || (pollfds[1].revents & POLLERR))
	    {
	      read_count = read(sockfd, buff, sizeof(buff)); 
	      if (read_count < 0) read_fail(); 
	      if (read_count == 0) exit_msg(); 
	      mod_count = 0; 
	      int i;
              for (i = 0; i < read_count; i++)
                {
                  if(buff[i] == '\n')
                    {
                      mod_buff[mod_count] = '\r';
                      mod_count++;
                      mod_buff[mod_count] = '\n';
                      mod_count++;
                    }
                  else
                    {
                      mod_buff[mod_count] = buff[i];
                      mod_count++;
                    }
                }
              write(1, mod_buff, mod_count);
	      tcsetattr(0, TCSANOW, &saved); 
	      exit(1); 
	    }
	}
}
     
	         
	    
