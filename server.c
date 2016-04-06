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

#include "server.h"

int main (int argc, char * argv[])
{
    //Data for sockets
    int socket_descriptor;
    int new_socket;             //For client connections
    int sin_size;
    int yes = 1;
    int numbytes;

    struct sockaddr_in my_address;
    struct sockaddr_in client_address;
    struct sigaction signal_action;

    //Data for file operations

    //Data for Shared Memory
    key_t key = 0x1020766;
    int shm_flag = IPC_CREAT | 0666;
    int shm_id;
    int size = SHMEM_SIZE;
    char * shared_mem;

    //Buffers
    char file_read_buffer[BUFFER_SIZE];
    char file_write_buffer[BUFFER_SIZE];
    char netw_read_buffer[BUFFER_SIZE];
    char netw_write_buffer[BUFFER_SIZE];
    char general_buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char argument[BUFFER_SIZE];

    //Set up shared memory
    if ((shm_id = shmget(key,size,shm_flag)) == -1)
    {
        perror("shmem failed");
        exit(1);
    }

    if ((shared_mem = shmat(shm_id, NULL, 0)) == (char *) -1)
    {
        perror("shmat()");

        //Remove the shared memory and exit
        shmctl(shm_id,IPC_RMID, NULL);
        exit(1);
    }

    //Set up Socket
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket():Socket not bound");
        exit(1);
    }

    printf("\nSocket Descriptor: %i", socket_descriptor);

    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt(): Returned failure");
        exit(1);
    }

    my_address.sin_family = AF_INET;         //host byte order
    my_address.sin_port = htons(SFTP_PORT);  //short, network byte order
    my_address.sin_addr.s_addr = INADDR_ANY; //Autofill ip
    memset(&(my_address.sin_zero), '\0', 0);  //Set the rest of the struct to 0

    if (bind(socket_descriptor, (struct sockaddr *) &my_address, sizeof(struct sockaddr)) == -1)
    {
        perror("bind(): Socket not bound");
        exit(1);
    }

    if (listen(socket_descriptor, BACKLOG) == -1)
    {
        perror("listen(): Not listening");
        exit(1);
    }

    signal_action.sa_handler = sigchld_handler;   //reap all dead processes
    sigemptyset(&signal_action.sa_mask);
    signal_action.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &signal_action, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    printf("\nServer setup complete...\n");

    //Server loop
    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);

        //check for bye
        if (*shared_mem == SHMEM_KILL)
        {
            printf("\nFile Copy Server is Down!");

            //remove shared memory
            shmctl(shm_id,IPC_RMID, NULL);
            exit(0);
        }

        if ((new_socket = accept(socket_descriptor, (struct sockaddr*) &client_address, &sin_size)) == -1)
        {
            perror("Connection not excepted");
            continue;
        }

        printf("\nserver: Got connection");

        //Fork
        if (!fork()) //run if child
        {
            close(socket_descriptor);

            int i = 0;
            for (i = 0; i < BUFFER_SIZE; ++i)
            {
                netw_read_buffer[i] = 0;
            }

            //Command Loop
            while (1)
            {
                //Get data from socket
                numbytes = recv(new_socket, (char *) netw_read_buffer, BUFFER_SIZE, 0);

                if (numbytes < 0)
                {
                    perror("recv(): Failed");
                    close(new_socket);
                    exit(1);
                }
                else if (numbytes == 0)
                {
                    printf("Client disconnected\n");
                    close(new_socket);
                    exit(1);
                }


                //COMMANDS

                //Parse input
                parse_input(netw_read_buffer, command, argument);

                //catalog
                if (!strcmp("catalog", command))
                {
                    FILE * file_pointer;
                    int error;
                    int buffer_counter = 0;
                    int path_counter = 0;
                    char path[BUFFER_SIZE];


                    //run ls
                    if ((file_pointer = popen("ls", "r")) == NULL)
                    {
                        perror("popen");
                        close(new_socket);
                        exit(1);
                    }

                    //read ls results into general buffer
                    while (fgets(path, BUFFER_SIZE, file_pointer) != NULL)
                    {
                        while (path[path_counter] != '\0' && buffer_counter < BUFFER_SIZE)
                        {
                            netw_write_buffer[buffer_counter] = path[path_counter];
                            ++path_counter;
                            ++buffer_counter;
                        }

                        netw_write_buffer[buffer_counter] = '\0';
                        path_counter = 0;
                    }

                    //Send general buffer to client
                    if (send(new_socket, netw_write_buffer, BUFFER_SIZE, 0) == -1)
                    {
                        perror("send()");
                        close(new_socket);
                        exit(1);
                    }

                }


                //download
                if (!strcmp("download", command))
                {
                    //check for source file
                    if (access(argument, F_OK) < 0)
                    {
                        printf("\nClient requested file that does not exist");
                        strcpy(netw_write_buffer, "error");

                        //send error message
                        if (send(new_socket, netw_write_buffer, BUFFER_SIZE, 0) == -1)
                        {
                            perror("send()");
                            close(new_socket);
                            exit(1);
                        }
                    }
                    else
                    {
                        //Open file
                        FILE * read_file;

                        read_file = fopen(argument, "r");

                        //Send file
                        while (fgets(netw_write_buffer, BUFFER_SIZE, read_file) != NULL)
                        {
                            if (send(new_socket, netw_write_buffer, BUFFER_SIZE, 0) == -1)
                            {
                                perror("failed to send data to client");
                                continue;
                            }
                        }

                        //Send cease-code
                        strcpy(netw_write_buffer, "102076684::SERV::CEASEREAD");
                        if (send(new_socket, netw_write_buffer, BUFFER_SIZE, 0) == -1)
                        {
                            perror("failed to send cease code");
                            close(new_socket);
                            exit(1);
                        }
                    }
                }

                //upload
                if (!strcmp("upload", command))
                {
                    char overwrite = 0;


                    //Send message for overwrite
                    if (overwrite)
                    {
                        strcpy(netw_write_buffer,"102076684::SERV::OVERWRITE");
                        if (send(new_socket, netw_write_buffer, BUFFER_SIZE, 0) == -1)
                        {
                            perror("failed to send overwrite code");
                            close(new_socket);
                            exit(1);
                        }
                    }
                }

                //spwd
                if (!strcmp("spwd", command))
                {
                    FILE * file_pointer;
                    int error;
                    int buffer_counter = 0;
                    int path_counter = 0;
                    char path[BUFFER_SIZE];


                    //run pwd
                    if ((file_pointer = popen("pwd", "r")) == NULL)
                    {
                        perror("popen");
                        close(new_socket);
                        exit(1);
                    }

                    //read pwd results into netw write buffer
                    while (fgets(path, BUFFER_SIZE, file_pointer) != NULL)
                    {
                        while (path[path_counter] != '\0' && buffer_counter < BUFFER_SIZE)
                        {
                            netw_write_buffer[buffer_counter] = path[path_counter];
                            ++path_counter;
                            ++buffer_counter;
                        }

                        netw_write_buffer[buffer_counter] = '\0';
                        path_counter = 0;
                    }

                    //Send general buffer to client
                    if (send(new_socket, netw_write_buffer, BUFFER_SIZE, 0) == -1)
                    {
                        perror("send()");
                        close(new_socket);
                        exit(1);
                    }
                }

                //bye
                if (!strcmp("bye", command))
                {
                    //Communicate to parent to shut down
                    *shared_mem = SHMEM_KILL;

                    printf("\nServer got bye. Bye.");

                    //Close socket
                    close(new_socket);

                    exit(0);
                }

            }

        }


    }

    return 0;
}

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

void parse_input(char * input, char * command, char * argument)
{
    int cursor = 0;
    int argcounter = 0;

    while (input[cursor] != '\0' && input[cursor] != ' ' )
    {
        command[cursor] = input[cursor];
        ++cursor;
    }

    command[cursor] = '\0' ;

    while (input[cursor] != '\0')
    {
        if (input[cursor] != ' ')
        {
            argument[argcounter] = input[cursor];
        }

        ++cursor;
        ++argcounter;
    }

    argument[argcounter] = '\0';

    //for test
    printf("\nPARSE_C:%s:\nPARSE_A1:%s:", command, argument);

}

void clean_string(char * string)
{
	int i = 0;
	while (string[i] != '\n')
		++i;

	string[i] = '\0';

}
