/*
NAME: Anup Kar
UID: 204419149
EMAIL: akar@g.ucla.edu
SLIPDAYS: 2
 */
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "SortedList.h"
#include <signal.h>

pthread_mutex_t myMutex;

int opt_yield = 0; 
int num_threads = 1; 
int iterations = 1;

int yield = 0; 
int mutex = 0; 
int spinlock = 0; 
int compswap = 0; 
SortedList_t *head;
SortedListElement_t *elements; 

volatile int spinlock_var = 0; 
void
populate_list(int count, SortedListElement_t *elements)
{
  srand((unsigned int)time(NULL));
  int wordLength = 5;
  for (int i = 0; i < count; i++)
    {
      char *str = malloc((wordLength + 1)*sizeof(char));
      for (int j = 0; j < wordLength; j++)
	{
	  int offset = rand() % 26;
	  str[j] = 'a' + offset; 
	}
      str[wordLength] = '\0';
      elements[i].key = str; 	
    }
}
void 
spinLock(void)
{
  while(__sync_lock_test_and_set(&spinlock_var, 1))
    ;
}

void
releaseSpinLock(void)
{
  __sync_lock_release(&spinlock_var);
}

void 
user_error(void)
{
  fprintf(stderr,"Usage: ./lab2, with flags --iterations=# --threads=#(default 1) \n"); 
  exit(1);
}

void
mutex_error(void)
{
  fprintf(stderr, "Error initiailziing mutex. Exiting Immediately.\n");
  exit(1);
}
void 
pthread_create_fail(void)
{
  fprintf(stderr, "Error creating thread. Exiting Immediately\n"); 
  exit(1);
}
void 
pthread_join_fail(void)
{
  fprintf(stderr, "Error: %s , was not able to join threads. Exiting Immediately.\n", strerror(errno)); 
  exit(1);
}

void*
worker(void* arg)
{
  long start = (long)(arg);
  long i = num_threads * start;
  for (; i < iterations; i++)
    {
      if(mutex)
	{
	  pthread_mutex_lock(&myMutex);
	  SortedList_insert(head, &elements[i]);
	  pthread_mutex_unlock(&myMutex); 
	}
      else if(spinlock)
	{
	  spinLock();
	  SortedList_insert(head, &elements[i]);
	  releaseSpinLock(); 
	}
      else
	SortedList_insert(head, &elements[i]); 
    }
  long length;
  if (mutex)
    {
      pthread_mutex_lock(&myMutex);
      length = SortedList_length(head);
      pthread_mutex_unlock(&myMutex);
    }
  else if (spinlock)
    {
      spinLock(); 
      length = SortedList_length(head);
      releaseSpinLock(); 
    }
  else
    length = SortedList_length(head); 

  i = num_threads*start + length - length;
  for (; i < iterations; i++)
    {
      if (mutex)
	{
	  pthread_mutex_lock(&myMutex);
	  SortedListElement_t *ptr = SortedList_lookup(head, elements[i].key);
	  if (ptr == NULL)
	    {
	      fprintf(stderr, "Error with search. Exiting Immediately\n");
	      exit(1); 
	    }
	  SortedList_delete(ptr);
	  pthread_mutex_unlock(&myMutex); 
	}
      else if (spinlock)
	{
	  spinLock();
	  SortedListElement_t *ptr = SortedList_lookup(head, elements[i].key);
	  if (ptr == NULL)
	    {
	      fprintf(stderr, "Error with search. Exiting Immediately\n");
	      exit(1); 
	    }
	  SortedList_delete(ptr);
	  releaseSpinLock(); 	 
	}
      else
	{
	  SortedListElement_t *ptr = SortedList_lookup(head, elements[i].key);
	  if (ptr == NULL)
	    {
	      fprintf(stderr, "Error with search. Exiting Immediately\n");
	      exit(1); 
	    }
	  SortedList_delete(ptr); 
	}
      
    }
  return NULL; 
}

char *
getOptYield()
{
  if ((opt_yield & INSERT_YIELD) == 0 && (opt_yield & DELETE_YIELD) == 0 && (opt_yield & LOOKUP_YIELD) == 0)
    return "none";
  else if ((opt_yield & INSERT_YIELD) != 0 && (opt_yield & DELETE_YIELD) == 0 && (opt_yield & LOOKUP_YIELD) == 0)
    return "i";
  else if ((opt_yield & INSERT_YIELD) == 0 && (opt_yield & DELETE_YIELD) == 0 && (opt_yield & LOOKUP_YIELD) != 0)
    return "l";
  else if ((opt_yield & INSERT_YIELD) == 0 && (opt_yield & DELETE_YIELD) != 0 && (opt_yield & LOOKUP_YIELD) == 0)
    return "d";
  else if ((opt_yield & INSERT_YIELD) != 0 && (opt_yield & DELETE_YIELD) == 0 && (opt_yield & LOOKUP_YIELD) != 0)
    return "il";
  else if((opt_yield & INSERT_YIELD) != 0 && (opt_yield & DELETE_YIELD) != 0 && (opt_yield & LOOKUP_YIELD) == 0)
    return "id";
  else if ((opt_yield & INSERT_YIELD) == 0 && (opt_yield & DELETE_YIELD) != 0 && (opt_yield & LOOKUP_YIELD) != 0)
    return "dl";
  else if ((opt_yield & INSERT_YIELD) != 0 && (opt_yield & DELETE_YIELD) != 0 && (opt_yield & LOOKUP_YIELD) != 0)
    return "idl";

  return NULL; 
}

char *
getOptSync()
{
  if (spinlock)
    return "s";
  else if (mutex)
    return "m";
  else
    return "none";
  return NULL;       
}

void
segfaultHandler(int SigNum)
{
  fprintf(stderr, "Segfault Detected, opt_yield=%d, num_threads=%d, iterations=%d, SigNum=%d\n", opt_yield, num_threads, iterations, SigNum);
  exit(2); 
}
int
main(int argc, char **argv)
  {
    char c;
    /*using getopt_long to parse commond line options and check for --bogus args */
     static struct option long_options[] =
       {
	 {"iterations", required_argument, 0, 'i'},
	 {"threads", required_argument, 0,'t'},
	 {"yield", required_argument, 0, 'y'},
         {"sync", required_argument, 0, 's'}, 
	 {0,           0,       0, 0}
       };
     signal(SIGSEGV, segfaultHandler);
     
     while (1)
       {
	 c = getopt_long(argc, argv, "", long_options, 0);
         if (c==-1) break;
         switch(c)
           {
           case 't':
	     num_threads = atoi(optarg); 
	     if (num_threads < 1) user_error(); 
             break;
	   case 'i':
	     iterations = atoi(optarg); 
	     if (iterations<1) user_error();
	     break; 
	   case 'y':
	     for (unsigned int i = 0; i < strlen(optarg); i++)
	       {
		 if(optarg[i] == 'i')
		   opt_yield = opt_yield | INSERT_YIELD;
		 else if (optarg[i] == 'd')
		   opt_yield = opt_yield | DELETE_YIELD;
		 else if(optarg[i] == 'l')
		   opt_yield = opt_yield | LOOKUP_YIELD;
		 else
		   user_error(); 
	       }
	     break;
	   case 's':
	     if (optarg[0] == 'm') mutex = 1; 
	     else if (optarg[0] == 's') spinlock = 1;
	     else
	       user_error(); 
	     break; 
           default:
             user_error(); 
             break;
           }
       }
          
     if(mutex==1)
       {
       	 int rc = pthread_mutex_init(&myMutex, NULL);
	 if (rc < 0) mutex_error(); 
       }
       
     head = (SortedList_t *)malloc(sizeof(SortedList_t));
     head->key = NULL;
     head->next = head;
     head->prev = head;
     
     elements = (SortedListElement_t *) malloc((iterations*num_threads) * sizeof(SortedListElement_t)); 

     int num_elements = num_threads * iterations;
     populate_list(num_elements, elements); 

     struct timespec start; 
     clock_gettime(CLOCK_MONOTONIC, &start);  
     pthread_t threads[num_threads];
     
     for (long i = 0; i < num_threads; i++)
       {
	 int rc = pthread_create(&threads[i], NULL, worker, (void*) i);
	 if (rc < 0) pthread_create_fail(); 
       }
     
     for(int i = 0; i < num_threads; i++) 
       {
	 int rc = pthread_join(threads[i], NULL); 
	 if (rc != 0) pthread_join_fail(); 
       }
     struct timespec end; 
     clock_gettime(CLOCK_MONOTONIC, &end); 
     long long time_elapsed = (end.tv_sec - start.tv_sec) * 1000000000; 
     time_elapsed += end.tv_nsec; 
     time_elapsed -= start.tv_nsec; 
     
     if (SortedList_length(head) != 0){
       fprintf(stderr,"There has been corruption in the size of the list; size != 0\n");
       exit(2);
     }
     char csv[60];
     memset(csv, 0, 60*sizeof(csv[0])); 
     char *yield = getOptYield();
     char *sync = getOptSync();
     int num_opts = num_threads * iterations * 3;
     long long avg_time_opt = time_elapsed / num_opts;
     sprintf(csv, "list-%s-%s,%d,%d,1,%d,%lld, %lld", yield, sync, num_threads, iterations, num_opts, time_elapsed, avg_time_opt);
     printf("%s\n", csv);

     if (mutex)
       pthread_mutex_destroy(&myMutex);

     for(int i = 0; i < num_threads*iterations; i++)
       {
	 free((void*)elements[i].key);
       }

     free(elements);
     free(head);
     return 0; 
    
  }
