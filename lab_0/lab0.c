/*
NAME: Anup Kar
EMAIL: akar@g.ucla.edu
ID: 204419149
 */
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

void
sighandler_for_segfault(int signalNumber)
{
  fprintf(stderr, "SIGSEGV was generated, signal number: %d\n", signalNumber);
  exit(4);
}

void
write_fail(void)
{
  fprintf(stderr, "Error making sys call write(). Exiting Immediately /n.");
  exit(1); 
}

void
segfault(char *something)
{
  something = NULL;
  something[0] = 12;
}

int
main(int argc, char **argv)
{
  int c;
  int ifd = 0;
  int ofd = 1;
  char* something;

  // --segfault and --catch options: 0, no segfault or catch, 1: segFault only, 2: catch only, 3: segfault and catch the SEGSEV generated exiting with return code 4
  int s_and_c = 0;
  static struct option long_options[] =
	{
	  {"input", required_argument, 0, 'i'},
	  {"output",  required_argument, 0, 'o'},
	  {"segfault",  no_argument, 0, 's'},
	  {"catch",  no_argument, 0, 'c'},
	  {0, 0, 0, 0}
	};

  while (1)
    {
      
      /* getopt_long stores the option index here. */
      int option_index = 0;
      c = getopt_long(argc, argv, "i:o:sc", long_options, &option_index);
      /* Detect the end of the options. */
      if (c == -1) break;
      
      switch (c)
	{
	case 'i':
	  // Input redirection
	  ifd = open(optarg, O_RDONLY);
  	  if (ifd==-1)
	    {
	      fprintf(stderr, "Error: There was an error opening the input file: %s.  Reason: %s \n" , optarg, strerror(errno));
	      exit(2);
	    }
	  else if (ifd>=0)
	    {
	      close(0);
	      if(dup2(ifd, 0)==-1)
		{
		  fprintf(stderr, "Error: The process of setting the file descriptor and redirecting input file failed. Reason: %s\n", strerror(errno));
		  exit(2);
		}
	    }
	  break;

	case 'o':

	  // Output redirection
	  ofd = creat(optarg, 0666);
	  if (ofd == -1)
	    {
	       fprintf(stderr, "Error: There was an error opening output file : %s. Reason: %s\n" , optarg, strerror(errno));
	      exit(3); 
	    }
	  else if (ofd >= 0)
	    {
	      close(1);
	      if(dup2(ofd, 1)==-1)
		{
		  fprintf(stderr, "Error: The process of setting the file descriptor and redirecting input file failed. Reason: %s\n", strerror(errno));
		  exit(3);
		}
	    }
	  break;

	case 'c':
	  s_and_c+=2;
	  break;

	case 's':
	  s_and_c+=1;
	  break;

	default:
	  fprintf(stderr, "Error: Unrecognized option: %s entered \n", argv[optind-1]);
	  fprintf(stderr, "Usage: ./lab0 --input=inputFile.txt --output=OutputFile.txt --segfault --catch\n");
	  return(1); 
	}
    }

  // Handle s and c options
  switch (s_and_c)
    {
    case 1:
      //--segfault: Force segmentation fault, should generate a Segmentation Fault
      segfault(something); // as recommended by the spec we use a separate function call to generate our Seg Fault for a more interesting backtrace.  
      break;
    case 2:
      // --catch:  continues running program with no segfault generated (should be exact same output as simply running program) exits with return code 0  
      signal(SIGSEGV, sighandler_for_segfault);
      break;
    case 3:
      // --segfault --catch : catch segFault SIGSEGV from forcing Segmentation Fault, stops running program reporting what signal was caught with the signal handler and exits with return code 4 
      signal(SIGSEGV, sighandler_for_segfault);
      segfault(something); // as recommended by the spec we use a separate function call to generate our Seg Fault for a more interesting backtrace. 
      break;
    }

  
  char *buff = (char*) malloc(sizeof(char));
  ssize_t byteCount; 
  byteCount = read(ifd, buff, 1);
  int ret; 
  while(byteCount>0)
    {
      ret=write(ofd, buff, 1);
      if (ret < 0) write_fail(); 
      byteCount = read(ifd, buff, 1); 
    }
  free(buff); 
  exit(0);
}


