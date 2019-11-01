#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <math.h>
#include "fcntl.h"
#include <stdlib.h>
#include <ctype.h>
#include <mraa.h>
#include <aio.h>
#include <mraa/aio.h> //this is included in the beaglebone

//GLOBALS
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k

int period = 1;
char default_temp = 'f';
char * logFile;
int LOG = 0;
FILE * logPtr = NULL;
int logfd;
int stop = 0;
int off = 0;


double
convert(int analogSig, char f_c)
{
    double R = 1023.0/(double)(analogSig)-1.0;
    R = R0*R;
    double conversion = 1.0/(log((R/R0))/B + 1 / 298.15) - 273.15; // convert to temperature via datasheet
    return f_c == 'C'? conversion : conversion * 9/5 + 32;
}

void
input_commands(int argc, char ** argv)
{
    static struct option long_options[] = {
        {"period", required_argument, 0, 'p'},
        {"scale", required_argument, 0, 's'},
        {"log", required_argument, 0,'l'},
        {0,         0,                 0,  0 }
    };
    int c;
    while (1)
      {
        c = getopt_long(argc, argv, "", long_options, 0);
        if (c == -1)
	  break;
	switch (c)
	  {
	  case 'p'://set the shellflag
	    period = atoi(optarg);
	    break;
	  case 's':
	    if (optarg[0] == 'c' || optarg[0] == 'C')
	      default_temp = 'C';
	    else if (optarg[0] == 'f' || optarg[0] == 'F')
	      default_temp = 'F';
	    else
	      {
		fprintf(stderr, "Must enter F, f, C or c for correct units. Exiting Immediately.\n"); 
		exit(1);
	      }
	    break;
	  case 'l':
	    LOG = 1;
	    logFile = optarg;
	    logPtr = fopen(logFile, "w");
	    if (logPtr == NULL)
	      {
		perror("Not able to open log file. Exiting immediately. \n"); 
		exit(2);
	      }
	    break;
	  case '?':
	  default:
	    //this means that the user input unrecognized argument to the program
	    fprintf(stdout,"Bogus argument. Exiting immediately.\n");
	    exit(1);
	    break;
	  }         
      }
}

void
change_scale(char unit)
{
  if ((unit == 'C') || (unit == 'c'))
      {
	default_temp = 'C';
        fprintf(stdout, "SCALE=C\n");
        if (LOG && stop == 0)
	  fprintf(logPtr, "SCALE=C\n");
      }
  else if ((unit == 'F') || (unit == 'f'))
    {
      default_temp = 'F';
      fprintf(stdout, "SCALE=F\n)");
      if (LOG && stop == 0)
	fprintf(logPtr, "SCALE=F\n");
    }
  else
    fprintf(stderr, "Problem in change_scale\n");
}
void
change_period (int given)
{
  period = given;
  fprintf(stdout, "PERIOD=%d\n", given); 
  if (LOG && stop == 0)
    fprintf(logPtr, "PERIOD=%d\n", given);
}

#define START 1
#define STOP 0
void
stop_start(int flag)
{
  if (flag == START)
    {
      stop = 0; 
      fprintf(stdout, "START\n");
      if (LOG)
	fprintf(logPtr, "START\n");
    }
  else
    {
      stop = 1; //stop
      fprintf(stdout, "STOP\n");
      if (LOG)
	fprintf(logPtr, "STOP\n");
    }
}

void
bb_log(char *str, int num_chars)
{
  char log_string[num_chars];
  int i;
  for ( i = 0; i < num_chars; i++)
    log_string[i] = str[i]; 
  
  if (LOG)
    fprintf(logPtr, "LOG %s\n", log_string); 
  
  fprintf(stdout, "LOG %s\n", log_string); 
}

void
shutdown()
{
  time_t raw; 
  struct tm *get_time; 
  time(&raw); 
  get_time = localtime(&raw); 
  fprintf(stdout, "OFF\n");
  //fprintf(stdout, "%d:%d:%d SHUTDOWN\n",get_time -> tm_hour, get_time ->tm_min, get_time ->tm_sec);
  fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n",get_time -> tm_hour, get_time ->tm_min, get_time ->tm_sec);
  
  if (LOG)
    {
    fprintf(logPtr, "OFF\n");
    fprintf(logPtr, "%.2d:%.2d:%.2d SHUTDOWN\n",get_time -> tm_hour, get_time ->tm_min, get_time ->tm_sec);
    }
  exit(0);
}

void
user_error(void)
{
  fprintf(stdout, "User entered command other then START=f, START=c, OFF, or LOG=*log_file_name*. Exiting_Immediately.\n"); 
  exit(1); 
}

void
command(char * str)
{ //assume that str is null terminated
    //    fprintf(stderr, "Received str: %s\n", str);
    if (str == NULL)
      {
      fprintf(stderr, "Error, null char detected. Exiting Immediately.\n");
      exit(2);
      }
    //check if the command is \n new-line terminated
    //check the strings
    int isLOG= 1;
    int isPeriod = 1;
    if (strcmp(str, "OFF\n") == 0) shutdown();
    if (strcmp(str, "SCALE=F\n") == 0) change_scale('F');
    else if (strcmp(str, "SCALE=C\n") == 0 ) change_scale('C');
    //start and stop
    else if (strcmp(str, "STOP\n") == 0) stop_start(STOP);
    else if (strcmp(str, "START\n") == 0) stop_start(START);
    else{
        //now log, or priod, or bogus
        char * simpleLogStr = "LOG ";
        if (strlen(str) <= strlen(simpleLogStr)){
            user_error();
            return;
        }
        int i ;
        for (i = 0; i < 4 && isLOG == 1; i++){
            if (str[i] != simpleLogStr[i]){
                isLOG = 0;
                break;
            }
        }
        if (isLOG){
            int characterCount = 0;
            char textStr [40];
            //read characters until the null char and size of the string
            unsigned int j = 0;
            while ( (str[j+4] != '\0' && str[j+4] != '\n') && j + 4 < strlen(str)){
                textStr [j] = str[j+4];
                characterCount ++;
                j++;
            }
            bb_log(textStr, characterCount);//butnow has to also get the string and the number of characters
            return;
        }
        //now check if it is a period
        char * periodTestStr = "PERIOD=";
        if (strlen(str) <= strlen(periodTestStr)){
            isPeriod = 0;
            user_error();
            return;
        }
        //now make sure that it has the same spelling as PERIOD
        int j = 0;
        for (; j < 7; j++)
	  {
            if (periodTestStr[j] != str[j])
	      { //it means that neither period nor log
                user_error();
                return;
	      }
	  }
        //now keep we have to read the value of the PERIOD
        //j == 7 now
        int charCount = 0;
        //can read just one digit value
        while (str[j] != '\n')
	  {
            //read character by char to make sure we have digits
            if (!isdigit(str[j]))
	      { //if not digit
                user_error();
                return;
	      }
            j++;
            charCount ++;
        }
        char buffer[charCount];
        int k;
        for (k = 7; k < charCount + 7; k++) //7 because is the index right after =, PERIOD=
            buffer[k-7]= str[k];
        
        change_period(atoi(buffer));
        if ((!isLOG) && (!isPeriod))
	  user_error();
            
        //fprintf(stderr, "Error with  command\n");
    }
}

void
buttonPressed()
{  
}

int
main(int argc, char ** argv) {
    
    input_commands(argc, argv); //to set the flags and initialize as we want
    //now setup the temp. sensor and the button
    mraa_gpio_context button;
    button = mraa_gpio_init (62); //acctually connected to GPIO = 51 ==> maps to ==> 62
    mraa_gpio_dir(button,MRAA_GPIO_IN);
    //but GPIO pin read is a non-blocking operation, so you can simply read the button status once per second.
    ////IMPORTANT,so read the value of GPIO instead of interupting
    
    //mraa_gpio_isr(button,MRAA_GPIO_EDGE_RISING, &buttonPressed, NULL);
    
    //also setup the temperature sensor
    int sensor; 
    mraa_aio_context temp_sensor;
    temp_sensor = mraa_aio_init(1);
    
    //setting up polling
    struct pollfd pollfds [1];
    pollfds[0].fd= STDIN_FILENO;
    pollfds[0].events = POLLIN | POLLHUP | POLLERR; //waiting for this event to occur
    while (1)
      {//read the values and output the values that are read
        sensor = mraa_aio_read(temp_sensor);
        double temp = convert(sensor, default_temp);
        time_t raw;
        struct tm *get_time;
        time (&raw);
        get_time = localtime(&raw); 
        if (stop == 0)
	  fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n",get_time -> tm_hour, get_time ->tm_min, get_time ->tm_sec, temp);
	if (LOG)
	  fprintf(logPtr, "%.2d:%.2d:%.2d %.1f\n",get_time -> tm_hour, get_time ->tm_min, get_time ->tm_sec, temp);
        
	//usleep(period * 1000000);//since the number used is in microseconds
	time_t begin, end;
	time(&begin);
	time(&end);
	while (difftime(end, begin) < (period))
	  {
	    if (mraa_gpio_read (button))
	      shutdown();
	    int polled = poll(pollfds, 1, 0);
	    if (polled == -1)
	      {
		fprintf(stderr, "Error with poll. Exiting Immediately.\n"); 
		exit(1);
	      }
	    if (pollfds[0].revents & POLLIN)
	      {
		//my main problem is to make sure that make sure that the string is null terminated
		//get the string
		//compare with possible values
		//check if it is null terminated ==> if not return immediately
		char tempBuff[35]; 
		memset(tempBuff, 0, 35*sizeof(char));//zero them out
		//scanf("%s",tempBuff);
		//fgets(<#char *restrict#>, <#int#>, <#FILE *#>)
                fgets(tempBuff, 35, stdin);
                command(tempBuff);
	      }
	    time(&end);
	  }
      }
    //dont forget to close the the sensors,
    close (logfd);
    exit(0); 
}

