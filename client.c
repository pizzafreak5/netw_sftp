//Garrett Schwartz 102076684
//CSCI whatever
//Created 4/3/16
//client.c

#include "client.h"

int main( int argc, char * argv[])
{
    //DATA SECTION
    int socket_descriptor;
    int numbytes;


    struct hostent *he;
    struct sockaddr_in server_address;

    char * read_stat;

    char file_read_buffer[BUFFER_SIZE];
    char file_write_buffer[BUFFER_SIZE];
    char netw_read_buffer[BUFFER_SIZE];
    char netw_write_buffer[BUFFER_SIZE];
    char input_buffer[BUFFER_SIZE];

    char command[BUFFER_SIZE];
    char argument_1[BUFFER_SIZE];
    char argument_2[BUFFER_SIZE];

    //CHECK ARGUMENTS

    if (argc != 2)
    {
        printf("Usage: ./client [hostname]");
        exit(0);
    }

    printf("%s", argv[0]);

    //CONNECT TO SERVER

    //Resolve host
    if ((he = gethostbyname(argv[1])) == NULL)
    {
        perror("Could not get host by name");
        exit(1);
    }

    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SFTP_PORT);
    server_address.sin_addr = * ((struct in_addr *) he->h_addr);
    memset(&(server_address.sin_zero), '\0', 8);

    if (connect (socket_descriptor, (struct sockaddr *) &server_address, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }
    printf("\nConnected to server, you may now enter commands.\n");

    //command loop
    while (1)
    {
        do
        {
            read_stat = fgets(input_buffer, 512, stdin) != NULL;
        }
        while (read_stat == NULL);

        clean_string(input_buffer);

        parse_input(input_buffer, command, argument_1, argument_2);

        //Check for local only commands

        //ls
        if (!strcmp("ls", command))
        {
            if (!fork())    //if child
            {
                execl("/bin/ls", "ls", (char *) 0);
                exit(1);
            }
        }

        //pwd
        if (!strcmp("pwd", command))
        {
            if (!fork())    //if child
            {
                execl("/bin/pwd", "pwd", (char *) 0);
                exit(1);
            }
        }

        //Commands that must be sent to server

        //catalog
        if (!strcmp("catalog", command))
        {
            strcpy(netw_write_buffer, command);

            if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
            {
                perror("send");
                close(socket_descriptor);
                exit(1);
            }

            printf("\nMessage sent!");

            if ((numbytes = recv(socket_descriptor, netw_read_buffer, (BUFFER_SIZE - 1), 0)) == -1)
            {
                perror("recv");
                exit(1);
            }

            printf("%s\n",netw_read_buffer);
        }

        //spwd
        if (!strcmp("spwd", command))
        {
            strcpy(netw_write_buffer, command);

            if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
            {
                perror("send");
                close(socket_descriptor);
                exit(1);
            }

            printf("\nMessage sent!");

            if ((numbytes = recv(socket_descriptor, netw_read_buffer, (BUFFER_SIZE - 1), 0)) == -1)
            {
                perror("recv");
                exit(1);
            }

            printf("%s\n",netw_read_buffer);
        }

        //download
        if (!strcmp("download", command))
        {
            //Copy command and src file to command to send
            strcpy(netw_write_buffer, "download");
            netw_write_buffer[8] = ' ';

            int i = 0;

            while(argument_1[i] != '\0') //copy
            {
                netw_write_buffer[(i + 9)] = argument_1[i];
                ++i;
            }

            argument_1[i] = '\0';

            //Open file to write to

            //check if dest file exists
            if (access(argument_2, F_OK) != -1)
            {
                printf("\nDestination file exists, will be overwritten.");
            }

            FILE * write_file;

            write_file = fopen(argument_2, "w");

            //Send command to server

            if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
            {
                perror("send");
                close(socket_descriptor);
                exit(1);
            }

            if (numbytes = recv(socket_descriptor, netw_read_buffer, (BUFFER_SIZE - 1), 0) == -1)
            {
                perror("recv");
                close(socket_descriptor);
                exit(1);
            }

            if (!strcmp(netw_read_buffer, "error"))
            {
                printf("received error from server");
            }
            else
            {
                while(!strcmp(netw_read_buffer, "102076684::SERV::CEASEREAD")) //Read until cease-code
                {
                    fputs(netw_read_buffer, write_file);
                    if (numbytes = recv(socket_descriptor, netw_read_buffer, (BUFFER_SIZE - 1), 0) == -1)
                    {
                        perror("recv failure while transfering");
                        continue;
                    }
                }

                //Done writing, close the file
                fclose(write_file);
            }
        }

        //upload
        if (!strcmp("upload", command))
        {
            //Check if file exists
            if (access(argument_1, F_OK) < 0)
            {
                printf("\nSource file does not exists");
            }
            else
            {

                strcpy(netw_write_buffer, "upload");

                //append dest file to upload
                netw_write_buffer[6] = ' ';

                int i = 0;

                while(argument_1[i] != '\0') //copy
                {
                    netw_write_buffer[(i + 7)] = argument_1[i];
                    ++i;
                }

                //Send server upload
                if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
                {
                    perror("Could not send upload code");
                    close(socket_descriptor);
                    exit(1);
                }

                //Open file
                FILE * read_file;

                read_file = fopen(argument_1, "r");

                //Send file to server
                while( fgets(netw_write_buffer, BUFFER_SIZE, read_file) != NULL)
                {
                    if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
                    {
                        perror("Could not send upload data");
                        continue;
                    }
                }

                //Send cease-code
                strcpy(netw_write_buffer, "102076684::CLIENT::CEASEREAD");
                if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
                {
                    perror("Could not send upload cease code");
                    close(socket_descriptor);
                    exit(1);
                }

                //check for file overwrite message
                if (numbytes = recv(socket_descriptor, netw_read_buffer, (BUFFER_SIZE - 1), 0) == -1)
                {
                    perror("recv failure while getting overwrite notice");
                    close(socket_descriptor);
                    exit(1);
                }

                if(!strcmp(netw_read_buffer, "102076684::SERV::OVERWRITE"))
                {
                    printf("\nUpload has overwritten a file on the server");
                }
            }
        }

        //bye
        if (!strcmp("bye", command))
        {
            strcpy(netw_write_buffer, "bye");

            if ((numbytes = send(socket_descriptor, netw_write_buffer, sizeof(netw_write_buffer), 0)) == -1)
            {
                perror("send");
                close(socket_descriptor);
                exit(1);
            }

            printf("Internet copy client is down!\n");

            close(socket_descriptor);
            exit(0);
        }
    }

    return 0;
}

void parse_input(char * input, char * command, char * argument_1, char * argument_2)
{
    int cursor = 0;
    int argcounter = 0;

    while (input[cursor] != '\0' && input[cursor] != ' ' )
    {
        command[cursor] = input[cursor];
        ++cursor;
    }

    command[cursor] = '\0' ;

    //iterate past whitespace
    while (input[cursor] == ' ')
    {
        ++cursor;
    }

    while (input[cursor] != '\0' && input[cursor] != ' ')
    {
        argument_1[argcounter] = input[cursor];

        ++cursor;
        ++argcounter;
    }

    argument_1[argcounter] = '\0';
    argcounter = 0;

    //iterate past whitespace
    while (input[cursor] == ' ')
    {
        ++cursor;
    }

    while (input[cursor] != '\0' && input[cursor] != ' ')
    {
        argument_2[argcounter] = input[cursor];

        ++cursor;
        ++argcounter;
    }
    argument_2[argcounter] = '\0';


    //for test
    printf("\nPARSE_C: %s\nPARSE_A1:%s\nPARSE_A2:%s:", command, argument_1, argument_2);

}

void user_help()
{
    char helpstring[] = "Usage: client [arguments] [hostname]\nArguments:\n -h, -H, -help: Display format and arguments";
    printf("%s",helpstring);
}

void clean_string(char * string)
{
	int i = 0;
	while (string[i] != '\n')
		++i;

	string[i] = '\0';

}
