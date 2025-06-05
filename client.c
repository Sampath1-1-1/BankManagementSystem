#include <stdio.h>      
#include <errno.h>      
#include <fcntl.h>     
#include <unistd.h>     
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/ip.h> 
#include <string.h>    
#include <stdbool.h>
#include <arpa/inet.h>

//================================ RESOURCES ================================
// #include "./resource/set.h"
// #include "./resource/shFile.h"
// #include "./resource/commanFun.h"
// #include "./resource/constantTerms.h"
// //================================ STRUCT__FUN ================================
#include "./recordStruct/employee.h"
#include "./recordStruct/account.h"
// #include "./recordStruct/loanapply.h"
// #include "./recordStruct/structs.h"
// #include "./recordStruct/transection.h"
void connection_handler(int sockFD); // Handles the read & write operations to the server
void printErrorMessage();
void main()
{
    int socketFileDescriptor, connectStatus;
    struct sockaddr_in serverAddress;
    struct sockaddr server;

    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == -1)
    {
        perror("Error while creating server socket!");
        _exit(0);
    }

   serverAddress.sin_family = AF_INET;
serverAddress.sin_port = htons(8081); // Match with the server port
serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost
   connectStatus = connect(socketFileDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectStatus == -1)
    {
        perror("Error while connecting to server!");
        close(socketFileDescriptor);
        _exit(0);
    }

    connection_handler(socketFileDescriptor);

    close(socketFileDescriptor);
}
// Handles the read & write operations w the server
void connection_handler(int sockFD)
{
    char readBuffer[50000], writeBuffer[1000]; // A buffer used for reading from / writting to the server
    ssize_t readBytes, writeBytes;            // Number of bytes read from / written to the socket
    struct Employee employee;
    struct Account account;
   
    char tempBuffer[1000];

    do{
         bzero(readBuffer, sizeof(readBuffer)); // Empty the read buffer
        bzero(tempBuffer, sizeof(tempBuffer));
        readBytes = read(sockFD, readBuffer, sizeof(readBuffer));
       
        if (readBytes == -1)
            perror("Error while reading from client socket!");
        else if (readBytes == 0)
            printf("No error received from server! Closing the connection to the server now!\n");
        else if (strchr(readBuffer, '^') != NULL)
        {
            // Skip read from client
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
            printf("%s\n", tempBuffer);
            writeBytes = write(sockFD, "^", strlen("^"));
            if (writeBytes == -1)
            {
                perror("Error while writing to client socket!");
                break;
            }
        }
        else if (strchr(readBuffer, '$') != NULL)
        {
            // Server sent an error message and is now closing it's end of the connection
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 2);
            printf("%s\n", tempBuffer);
            printf("Closing the connection to the server now!\n");
            break;
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer)); // Empty the write buffer

            if (strchr(readBuffer, '#') != NULL)
                strcpy(writeBuffer, getpass(readBuffer));
            else
            {
                  printf("%s\n", readBuffer);
                // fflush(stdin);
                // do{
              
               int inputResult = scanf("%[^\n]%*c", writeBuffer); // Take user input!
                // if (inputResult == 0) {
            // Clear the newline or any garbage left in stdin when user presses Enter without typing anything
        //     int ch;
        //     while ((ch = getchar()) != '\n' && ch != EOF);
        //     printf("\n\n");
        //     printErrorMessage();
        //     printf("\n\n");
        // }  
                //  } while(strlen(writeBuffer)==0);

                        bzero(readBuffer, sizeof(readBuffer)); // Empty the read buffer


              
            }

            writeBytes = write(sockFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing to client socket!");
                printf("Closing the connection to the server now!\n");
                break;
            }
        }
    } while (readBytes > 0);

    close(sockFD);
}


void printErrorMessage() {
    printf("#############################################################\n");
    printf("##                                                         ##\n");
    printf("##       Input cannot be empty! Please try again.          ##\n");
    printf("##                                                         ##\n");
    printf("#############################################################\n");
}