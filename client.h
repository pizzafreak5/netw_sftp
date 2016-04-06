//Garrett Schwartz 102076684
//CSCI whatever
//Created 4/3/16
//client.h

/*Description:
 *  This program communicates with its companion
 * server program to upload and download files
 * from the machine that is running the server
 * program.
*/

#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H


#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>          //provides system error numbers
#include <netdb.h>          //provides gethostbyname()
#include <netinet/in.h>    //provides socket structs
#include <unistd.h>

#define SFTP_PORT 6684      //Default port
#define BUFFER_SIZE 512     //Default buffer size

void user_help();

void parse_input(char * input, char * command, char * argument_1, char * argument_2);

void clean_string(char * string);

#endif // SFTP_CLIENT_H
