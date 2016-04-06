//Garrett Schwartz 102076684
//CSCI whatever
//Created 4/3/16
//server.h

/*Description:
 *  This program communicates with its companion
 * client program to receive and send files
 * to the machine that is running the server
 * program.
*/

#ifndef SFTP_SERV_H
#define SFTP_SERV_H


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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SFTP_PORT 6684      //Default port
#define BUFFER_SIZE 512     //Default buffer size
#define SHMEM_KEY 102076684 //Shared memory key
#define SHMEM_KILL 0x11
#define SHMEM_SIZE 32
#define BACKLOG 10

void sigchld_handler(int s);

void parse_input(char * input, char * command, char * argument);
/*
    This function takes a string of input, received from the client, and will split
    it into two strings, command, and argument. The command string will take the form
    "[command]\0" and the argument string will take the form "[argument]\0". The server
    will never receive a command with more than one argument from the client.
    Note that input must be null terminated, and command and argument will be overwritten.
    if there is no argument, argument will be set to "\0"
*/

void clean_string(char * string);

#endif // SFTP_SERV_H
