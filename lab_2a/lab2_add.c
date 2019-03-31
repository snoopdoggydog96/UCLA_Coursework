#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/* GLOBAL VARS SHARED/ACESSIBLE BY EACH THREAD (has the same & space) but LOCAL ALLOCATION OF VARS for each THREAD!*/

struct Accumulate
{
  long long *counterPtr; 
  int iterations; 
}; 

pthread_mutex_t lock;

int opt_yield = 0; 
int num_threads = 1; 
int iterations = 1;

int yield = 0; 
int mutex = 0; 
int spinlock = 0; 
int compswap = 0; 


volatile int cond_var = 0; 
volatile int comp_swap = 0; 

void 
spinLock(void)
{
  while(__sync_lock_test_and_set(&cond_var, 1))
    while(cond_var)
      ; 
}

void
releaseSpinLock(void)
{ __sync_lock_release(&cond_var); }


void 
compswaplock(void)
{
  while(__sync_bool_compare_and_swap(&comp_swap, 0, 1))
    ; 
}

void
compswaprelease(void)
{ comp_swap = 0; } 


void
unprotected_add(long long *pointer, long long value)
 {
  long long sum = *pointer + value;
  if(opt_yield == 1) sched_yield(); 
  *pointer = sum;
 }
void
add(long long *pointer, long long value)
{
  if(mutex)
    {
      pthread_mutex_lock(&lock); 
      unprotected_add(pointer, value); 
      pthread_mutex_unlock(&lock); 
    }
  else if(spinlock)
    {
      spinLock(); 
      unprotected_add(pointer, value); 
      releaseSpinLock(); 
    }
  else if(compswap)
    {
      long long sum, exp; 
      do{
	exp = *pointer; 
	sum = exp + value; 
	if(opt_yield == 1) sched_yield(); 
      } while (exp != __sync_val_compare_and_swap(pointer, exp, sum)); 
    }
  else 
    unprotected_add(pointer, value); 
}
void*
add_wrapper(void *arg)
{
  long long *counterPtr = (long long *) arg; 
  int i, j; 
  for (i = 0; i < iterations; i++) add(counterPtr, 1); 
  for (j = 0; j < iterations; j++) add(counterPtr, -1);    
  return NULL; 
}
void 
user_error(void)
{
  fprintf(stderr,"Usage: ./lab2, with flags --iterations=# --threads=#(default 1) \n"); 
  exit(1);
}

void
Pthread_mutex_init_fail(void)
{
  fprintf(stderr, "Error initializing mutex. Exiting immediately\n");  
  exit(1);
 }
void
Pthread_create_fail(void)
{
  fprintf(stderr, "Error allocating thread. Exiting Immediately.\n.");
  exit(1); 
}
void
Pthread_join_fail(void)
{
  fprintf(stderr, "Error: %s , was not able to join threads. Exiting Immediately.\n", strerror(errno)); 
  exit(1);
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
	 {"yield", no_argument, 0, 'y'},
         {"sync", required_argument, 0, 's'}, 
	 {0,           0,       0, 0}
       };
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
	     opt_yield = 1; 
	     yield = 1; 
	     break;
	   case 's':
	     if (optarg[0] == 'm') mutex = 1; 
	     else if(optarg[0] =='c') compswap = 1; 
	     else if (optarg[0] == 's') spinlock = 1; 
	     break; 
           default:
             user_error(); 
             break;
           }
       }
     long long counter = 0;     

     pthread_t threads[num_threads];

     if(mutex==1)
       {
	 int rc = pthread_mutex_init(&lock, NULL);
	 if(rc != 0) Pthread_mutex_init_fail(); 
       }
     
     struct timespec start;
     long long timer; 
     clock_gettime(CLOCK_MONOTONIC, &start);  
     
     int i; 
     for (i = 0; i < num_threads; i++)
       {
	 int rc = pthread_create(&threads[i], NULL, add_wrapper, &counter);
	 if (rc) Pthread_create_fail();
       }
     
     for(i = 0; i < num_threads; i++)
       {
	 int rc = pthread_join(threads[i], NULL);
	 if (rc != 0 ) Pthread_join_fail(); 
       }
     struct timespec end;    
     clock_gettime(CLOCK_MONOTONIC, &end); 
     timer = (end.tv_sec - start.tv_sec) * 1000000000; 
     timer += end.tv_nsec; 
     timer -= start.tv_nsec; 
     
     char csv[60];
     memset(csv, 0, 60*sizeof(csv[0])); 
     /* our output needs to be in the format add-none,10,10000,200000,6574000,32,374 */
     int ops = num_threads * iterations * 2; 
     long long avg_time = timer / ops; 
     /*
      add-m no yield, mutex synch
      add-s no yield, spinlock synch
      add-c no yield , compswap synch
      add-yield-none yield, no synch
      add-yield-m yield, mutex synch
      add-yield-s yield, spinlock synch
      add-yield-c yield, compswap synch
     */
     if(!yield && mutex)
       {
	 sprintf(csv, "add-m,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter)
;
       }
     else if(!yield && spinlock)
       {
	 sprintf(csv, "add-s,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     else if(!yield && compswap)
       {
       sprintf(csv, "add-c,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     else if(yield && mutex)
       {
       sprintf(csv, "add-yield-m,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     else if(yield && spinlock)
       {
       sprintf(csv, "add-yield-s,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     else if(yield && compswap)
       {
       sprintf(csv, "add-yield-c,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     else if(yield)
       {
	 sprintf(csv, "add-yield-none,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     else
       {
       sprintf(csv, "add-none,%d,%d,%d,%lld,%lld,%lld", num_threads, iterations, ops, timer, avg_time, counter);
       }
     printf("%s\n", csv); 
     if (mutex) pthread_mutex_destroy(&lock); 
     return 0; 
  }
