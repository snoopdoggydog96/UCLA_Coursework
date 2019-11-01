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


int period = 1;
int motion = 0; 
char * logFile;
int LOG = 0;
FILE * logPtr = NULL;
int logfd;
int stop = 0;
int off = 0;
int temp = 0; 
double
convert (int analogSig, char unit)
{
    double R = 1023.0/(double)(analogSig)-1.0;
    R = R0*R;
    double conversion = 1.0/(log((R/R0))/B + 1 / 298.15) - 273.15; // convert to temperature via datasheet
    return unit == 'C'? conversion : conversion * 9/5 + 32;
}

void
get_opt_wrapper(int argc, char ** argv){
    static struct option long_options[] = {
        {"period", required_argument, 0, 'p'},
        {"motion", no_argument, 0, 'm'},
        {"log", required_argument, 0,'l'},
        {0,         0,                 0,  0 }
    };
    int c;
    while (1) {
        c = getopt_long(argc, argv, "", long_options, 0);
        if (c == -1)            //if no input is given
	  break;
	switch (c) {
	case 'p'://set the shellflag
	  period = atoi(optarg);
	  break;
	case 'm':
	  motion = 1; 
	  break;
	case 'l':
                LOG = 1;
                logFile = optarg;
                logPtr = fopen(logFile, "w");
                if (logPtr == NULL)
		  {
                    perror("problem opening the file\n");
		    exit(1); 
		  }
                break;
            case '?':
            default:
                //this means that the user input unrecognized argument to the program
                fprintf(stdout,"Bogus argument, exiting\n");
                exit(1);
                break;
        }
    }
    
}

void
change_scale(char temp){
    if ((temp == 'C') || (temp == 'c'))
      {
        default_temp = 'C';
        fprintf(stdout, "SCALE=C\n");
        if (LOG && stop == 0){
            fprintf(logPtr, "SCALE=C\n");
        }
    }
    else if ((temp == 'F') || (temp == 'f')){
        default_temp = 'F';
        fprintf(stdout, "SCALE=F\n");
        if (LOG && stop == 0){
            fprintf(logPtr, "SCALE=F\n");
        }
    }
    else{
        fprintf(stderr, "Problem in _change_\n");
    }
}

void
_change_period (int givenPeriod)
{
    period = givenPeriod;
    fprintf(stdout, "PERIOD=%d\n", givenPeriod);
    if (LOG && stop == 0)
      fprintf(logPtr, "PERIOD=%d\n", givenPeriod);
}

#define START 1
#define STOP 0
void _stop_start(int flag)
{
    if (flag == START)
      {
        stop = 0; //so continue
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

void _log(char * str, int numcharacters){
    char mString [numcharacters];
    int i;
    for ( i = 0; i < numcharacters; i++)
      mString[i] = str[i];
    
    if(LOG)
      fprintf(logPtr, "LOG %s\n", mString);
    
    fprintf(stdout, "LOG %s\n", mString);
}

void _shutdown()
{
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime ( &rawtime );
    fprintf(stdout, "OFF\n");
    //fprintf(stdout, "%d:%d:%d SHUTDOWN\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec);
    fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec);

    if (LOG)
      {
        fprintf(logPtr, "OFF\n");
        fprintf(logPtr, "%.2d:%.2d:%.2d SHUTDOWN\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec);
      }
    exit(0); 
}
void
_bogus_command()
{
    fprintf(stdout, "Bogus command, check usage\n");
}

void Command(char * str){ //assume that str is null terminated
    //    fprintf(stderr, "Received str: %s\n", str);
    if (str == NULL)
      {
	fprintf(stderr, "Error with NULL in command\n");
	exit(1); 
      }
    //check if the command is \n new-line terminated
    //check the strings
    int isLOG= 1;
    int isPeriod = 1;
    if (strcmp(str, "OFF\n") == 0) _shutdown();
    //start and stop
    else if (strcmp(str, "SCALE=c\n")) change_scale('C');
    else if (strcmp(str, "SCALE=f\n")) change_scale('F');
    else if (strcmp(str, "STOP\n") == 0) _stop_start(STOP);
    else if (strcmp(str, "START\n") == 0) _stop_start(START);
    else{
        //now log, or priod, or bogus
        char * simpleLogStr = "LOG ";
        if (strlen(str) <= strlen(simpleLogStr))
	  {
            _bogus_command();
            return;
	  }
        int i ;
        for (i = 0; i < 4 && isLOG == 1; i++)
	  {
            if (str[i] != simpleLogStr[i])
	      {
                isLOG = 0;
                break;
	      }
	  }
        if (isLOG)
	  {
            int characterCount = 0;
            char textStr [40];
            //read characters until the null char and size of the string
            unsigned int j = 0;
            while ( (str[j+4] != '\0' && str[j+4] != '\n') && j + 4 < strlen(str))
	      {
                textStr [j] = str[j+4];
                characterCount ++;
                j++;
	      }
            _log(textStr, characterCount);//butnow has to also get the string and the number of characters
            return;
	  }
        //now check if it is a period
        char * periodTestStr = "PERIOD=";
        if (strlen(str) <= strlen(periodTestStr))
	  {
            isPeriod = 0;
            _bogus_command();
            return;
	  }
        //now make sure that it has the same spelling as PERIOD
        int j = 0;
        for (; j < 7; j++)
	  {
	    if (periodTestStr[j] != str[j])
	      {
	    _bogus_command();
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
                _bogus_command();
                return;
	      }
            j++;
            charCount ++;
        }
        char buffer [charCount];
        int k;
        for (k = 7; k < charCount + 7; k++)
	  buffer[k-7]= str[k]; //7 is length of max size_command

        _change_period(atoi(buffer));
        if ((!isLOG) && (!isPeriod)){
            _bogus_command();
            return;
        }
        //fprintf(stderr, "Error with  command\n");
    }
    //for period we have to read in
}

void
buttonPressed()
{    
}

int
main(int argc, char ** argv)
{
    
    get_opt_wrapper(argc, argv); //to set the flags and initialize as we want
    //now setup the pir motion sensor and the solid state relay
    mraa_gpio_context button; 
    button = mraa_gpio_init (62); //acctually connected to GPIO = 51 ==> maps to ==> 62
    mraa_gpio_read(button); 
    //but GPIO pin read is a non-blocking operation, so you can simply read the ssr status once per second.
    ////IMPORTANT,so read the value of GPIO instead of interupting
    
    //mraa_gpio_isr(ssr,MRAA_GPIO_EDGE_RISING, &ssrPressed, NULL);
    
    //also setup the temperature sensor
    double sensorValue;
    mraa_aio_context tempSensor;
    tempSensor = mraa_aio_init(1);
    
    //setting up polling
    struct pollfd pollfds [1];
    pollfds[0].fd= STDIN_FILENO;
    pollfds[0].events = POLLIN | POLLHUP | POLLERR; //waiting for this event to occur
    while (1)
      {//read the values and output the values that are read
        sensorValue = mraa_aio_read(tempSensor);
	double temp = convert(sensorValue, unit); 
        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
	time_t beginTime, endTime;
        time(&beginTime);
        time(&endTime);
        while (difftime(endTime, beginTime) < (period))
	  {
	    //unsigned int delay_time = period * 1000; 
	    if(mraa_gpio_read(button)) _shutdown(); 
	    	   	   	    
            int polled = poll(pollfds, 1, 0);
            if (polled == -1)
	      {
                fprintf(stderr, "Polling error\n");
                exit(1); 
	      }
            if (pollfds[0].revents & POLLIN)
	      {
                //my main problem is to make sure that make sure that the string is null terminated
                //get the string
                //compare with possible values
                //check if it is null terminated ==> if not return immediately
                char charBuffer [35];
                memset(charBuffer, 0, 35*sizeof(char));//zero them out
                //scanf("%s",charBuffer);
                //fgets(<#char *restrict#>, <#int#>, <#FILE *#>)
                fgets(charBuffer, 35, stdin);
                Command(charBuffer);
	      }
            time(&endTime);
        }
        
    }
     //dont forget to close the the sensors,
    
    if ((int rc = mraa_gpio_close(button) < 0) exit(1); 
    if ((int rc2 = mraa_aio_close(tempSensor) <0)) exit(1); 
      
    close (logfd);
    exit(0); 
    
}

