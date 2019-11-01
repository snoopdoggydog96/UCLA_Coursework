
//NAME: Milad Nourian
//ID: 004854226

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

#include <signal.h> //for kill()
#include <sys/types.h> //for kill ()
#include <sys/types.h> //for waitpid
#include <sys/wait.h> //for waitpid
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//needed for openssl
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>



///UNCOMMENT THIS
#include <mraa/aio.h> //this is included in the beaglebone

//GLOBALS
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k
int period = 1;
char temperatureUnit = 'f';
char * logFile;
int logFlag = 0;
FILE * logPtr = NULL;
int logfd;
int stopFlag = 0;
int offFlag = 0;
//int idNumber = -1;
char * idNumber = NULL;
char * hostName = NULL;
int portNumber = -1;
int socketfd;
struct sockaddr_in server_address;
struct hostent *server;
FILE * socketPtr = NULL;


double calculateTemperature (int analogSig, char unit){
    double R = 1023.0/(double)(analogSig)-1.0;
    R = R0*R;
    double celciusTemp = 1.0/(log((R/R0))/B + 1 / 298.15) - 273.15; // convert to temperature via datasheet
    return unit == 'C'? celciusTemp : celciusTemp * 9/5 + 32;
}

void handleInputCommands(int argc, char ** argv){
    static struct option long_options[] = {
        
        {"period", required_argument, 0, 'p'},
        {"scale", required_argument, 0, 's'},
        {"log", required_argument, 0,'l'},
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
        {0,         0,                 0,  0 }
    };
    
    

    int c;
    while (1) {
        c = getopt_long(argc, argv, "", long_options, 0);
        if (c == -1){
            //if no input is given
            break;
        }
        switch (c) {
            case 'p'://set the shellflag
                period = atoi(optarg);
                break;
            case 's':
                if (optarg[0] == 'c' || optarg[0] == 'C'){
                    temperatureUnit = 'C';
                }
                else if (optarg[0] == 'f' || optarg[0] == 'F'){
                    temperatureUnit = 'F';
                }
                else {
                    fprintf(stderr, "Invalid tempature unit given, exiting\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                if (strlen(optarg)!= 9){
                    fprintf(stderr, "id is not 9 digit\n");
                    exit(EXIT_FAILURE);
                }
                idNumber = optarg;
                //idNumber = atoi(optarg);
                break;
            case 'h':
                hostName = optarg;
                break;
            case 'l':
                logFlag = 1;
                logFile = optarg;
                logPtr = fopen(logFile, "w");
                if (logPtr == NULL){
                    perror("problem opening the file\n");
                    exit(EXIT_FAILURE);
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
    //now get the port number that is specified as the last part of the command
    portNumber = atoi(argv[optind]); //turn into int
    if (portNumber == -1 || portNumber == 0){
        fprintf(stderr, "Port number invalid, %d\n",portNumber);
        exit(EXIT_FAILURE);
    }
    //make sure all of the required input are present, otherwise, error message and exit
    if (idNumber == NULL || hostName == NULL || portNumber == -1|| logFlag == 0){
        fprintf(stderr, "Mandatory arguments not provided, exiting\n");
        exit(EXIT_FAILURE);
    }
}

void handle_change_scale(char temp){
    if ((temp == 'C') || (temp == 'c')){
        temperatureUnit = 'C';
        fprintf(stdout, "SCALE=C\n");
        if (logFlag && stopFlag == 0){
            fprintf(logPtr, "SCALE=C\n");
        }
    }
    else if ((temp == 'F') || (temp == 'f')){
        temperatureUnit = 'F';
        fprintf(stdout, "SCALE=F\n");
        if (logFlag && stopFlag == 0){
            fprintf(logPtr, "SCALE=F\n");
        }
    }
    else{
        fprintf(stderr, "Problem in handle_change_scale\n");
    }
}
void handle_change_period (int givenPeriod){
    period = givenPeriod;
    fprintf(stdout, "PERIOD=%d\n", givenPeriod);
    if (logFlag && stopFlag == 0){
        fprintf(logPtr, "PERIOD=%d\n", givenPeriod);
    }
}
#define START 1
#define STOP 0
void handle_stop_start(int flag){
    if (flag == START){
        stopFlag = 0; //so continue
        fprintf(stdout, "START\n");
        if (logFlag){
            fprintf(logPtr, "START\n");
        }
    }
    else {
        stopFlag = 1; //stop
        fprintf(stdout, "STOP\n");
        if (logFlag){
            fprintf(logPtr, "STOP\n");
        }
    }
    
}
void handle_log(char * str, int numcharacters){
    char mString [numcharacters];
    int i;
    for ( i = 0; i < numcharacters; i++){
        mString [i] = str[i];
    }
    if (logFlag){
        fprintf(logPtr, "LOG %s\n", mString);
    }
    fprintf(stdout, "LOG %s\n", mString);
    
}
void handle_shutdown(){
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    fprintf(stdout, "OFF\n");
    //fprintf(stdout, "%d:%d:%d SHUTDOWN\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec);
    fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec);
    
    if (logFlag){
        fprintf(logPtr, "OFF\n");
        fprintf(logPtr, "%.2d:%.2d:%.2d SHUTDOWN\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec);
    }
    exit(EXIT_SUCCESS);
}
void handle_bogus_command(){
    fprintf(stdout, "Bogus command, check usage\n");
}

void handleCommand(char * str){ //assume that str is null terminated
    if (str == NULL){fprintf(stderr, "Error with NULL in handlecommand\n"); exit(EXIT_FAILURE);}
    //check if the command is \n new-line terminated
    //check the strings
    int isLOG= 1;
    int isPeriod = 1;
    if (strcmp(str, "OFF\n") == 0) handle_shutdown();
    if (strcmp(str, "SCALE=F\n") == 0) handle_change_scale('F');
    else if (strcmp(str, "SCALE=C\n") == 0 ) handle_change_scale('C');
    //start and stop
    else if (strcmp(str, "STOP\n") == 0) handle_stop_start(STOP);
    else if (strcmp(str, "START\n") == 0) handle_stop_start(START);
    else{
        //now log, or priod, or bogus
        char * simpleLogStr = "LOG ";
        if (strlen(str) <= strlen(simpleLogStr)){
            handle_bogus_command();
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
            handle_log(textStr, characterCount);//butnow has to also get the string and the number of characters
            return;
        }
        //now check if it is a period
        char * periodTestStr = "PERIOD=";
        if (strlen(str) <= strlen(periodTestStr)){
            isPeriod = 0;
            handle_bogus_command();
            return;
        }
        //now make sure that it has the same spelling as PERIOD
        int j = 0;
        for (; j < 7; j++){
            if (periodTestStr[j] != str[j]){ //it means that neither period nor log
                handle_bogus_command();
                return;
            }
        }
        //now keep we have to read the value of the PERIOD
        //j == 7 now
        int charCount = 0;
        //can read just one digit value
        while (str[j] != '\n'){
            //read character by char to make sure we have digits
            if (!isdigit(str[j])){ //if not digit
                handle_bogus_command();
                return;
            }
            j++;
            charCount ++;
        }
        char buffer [charCount];
        int k;
        for (k = 7; k < charCount + 7; k++){ //7 because is the index right after =, PERIOD=
            buffer[k-7]= str[k];
        }
        handle_change_period(atoi(buffer));
        if ((!isLOG) && (!isPeriod)){
            handle_bogus_command();
            return;
        }
        //fprintf(stderr, "Error with handle command\n");
    }
    //for period we have to read in
    
    
}

void buttonPressed(){
    
}


void setupTCPconnection(){
    socketfd =socket(AF_INET, SOCK_STREAM, 0); //connect the socket
    if (socketfd < 0){
        fprintf(stderr, "There was an error creating the socket for the client, exiting with failure \n");
        exit(EXIT_FAILURE);
    }
    socketPtr = fdopen(socketfd, "w");
    if (socketPtr == NULL){
        fprintf(stderr, "Error in getting socketPtr\n");
        exit(EXIT_FAILURE);
    }
    server = gethostbyname(hostName); //use the host name from the command line args
    if (server == NULL){
        fprintf(stderr, "Unable to get the host name of the server, exiting\n");
        exit(EXIT_FAILURE);
    }
    memset((char *)  &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&server_address.sin_addr.s_addr,
          server->h_length);
    server_address.sin_port = htons(portNumber);
    if (connect(socketfd,(struct sockaddr *)&server_address,sizeof(server_address)) < 0)
    {
        fprintf(stderr, "There was an error connecting on the client side.\n");
        exit(EXIT_FAILURE);
    }
}

void sendIDtoServer(){
     //ID=ID-number
    dprintf(socketfd, "ID=%s\n", idNumber);
    fprintf(logPtr, "ID=%s\n", idNumber);
}

int main(int argc, char ** argv) {
    
    handleInputCommands(argc, argv); //to set the flags and initialize as we want
    
    // setup the temperature sensor
    int sensorValue;
    mraa_aio_context tempSensor;
    tempSensor = mraa_aio_init(1);
    
    setupTCPconnection();
    sendIDtoServer();
    

    /*
     as before, send (and log) newline terminated temperature reports over the connection
     as before, process (and log) newline-terminated commands received over the connection
     If your temperature reports are mis-formatted, the server will return a LOG command with a description of the error.
     Having logged these commands will help you find and fix any problems with your reports.
     as before, the last command sent by the server will be an OFF.
     Unlike the previous project, the button will not be used as an alternative manual shutdown mechanism.
     */
    //input comes from the TCP connection
    close(STDIN_FILENO);//now we have closed the stdin
    if (dup2(socketfd, STDIN_FILENO) == -1){
        fprintf(stderr, "There was an error with dup2-STDIN in pipefdFromTerminalToShell\n");
        exit(EXIT_FAILURE);
    }
    
    //setting up polling
    struct pollfd pollfds [1];
    pollfds[0].fd= socketfd; //wait for input from TCP/socket
    pollfds[0].events = POLLIN | POLLHUP | POLLERR; //waiting for this event to occur
    while (1){//read the values and output the values that are read
        sensorValue = mraa_aio_read(tempSensor);
        double tempatureValue = calculateTemperature(sensorValue, temperatureUnit);
        time_t rawtime;
        struct tm * timeinfo;
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        if (stopFlag == 0){
//            fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec, tempatureValue);
            dprintf(socketfd, "%.2d:%.2d:%.2d %.1f\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec, tempatureValue);
            if (logFlag){
                fprintf(logPtr, "%.2d:%.2d:%.2d %.1f\n",timeinfo -> tm_hour, timeinfo ->tm_min, timeinfo ->tm_sec, tempatureValue);}
        }
        //usleep(period * 1000000);//since the number used is in microseconds
        time_t beginTime, endTime;
        time(&beginTime);
        time(&endTime);
        while (difftime(endTime, beginTime) < (period)){
            int polled = poll(pollfds, 1, 0);
            if (polled == -1){
                fprintf(stderr, "Polling error\n");
                exit(EXIT_FAILURE);
            }
            if (pollfds[0].revents & POLLIN){
                //my main problem is to make sure that make sure that the string is null terminated
                //get the string
                //compare with possible values
                //check if it is null terminated ==> if not return immediately
                char charBuffer [256];
                memset(charBuffer, 0, 256*sizeof(char));//zero them out
                //read(socketfd, charBuffer, 35); //read the input over the TCP connection
                if (read(socketfd, charBuffer, 256) <0){
                    fprintf(stderr, "read error with socketfd\n");
                    exit(EXIT_FAILURE);
                }
//                fgets(charBuffer, 35, socketPtr);
                handleCommand(charBuffer);
            }
            time(&endTime);
        }
        
    }
    
    
    //dont forget to close the the sensors,
    
    close (logfd);
    exit(EXIT_SUCCESS);
    
}

