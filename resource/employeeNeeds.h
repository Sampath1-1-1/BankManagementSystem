#ifndef EMPLOYEENEEDS_FUNCTIONS
#define EMPLOYEENEEDS_FUNCTIONS

#include <stdio.h>     // Import for `printf` & `perror`
#include <unistd.h>    // Import for `read`, `write & `lseek`
#include <string.h>    // Import for string functions
#include <stdbool.h>   // Import for `bool` data type
#include <sys/types.h> // Import for `open`, `lseek`
#include <sys/stat.h>  // Import for `open`
#include <fcntl.h>     // Import for `open`
#include <stdlib.h>    // Import for `atoi`
#include <errno.h>     // Import for `errno`
#include <crypt.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "./set.h"
#include "./shFile.h"
// #include "../customer/customer.h"
#include "../admin/admin_cred.h"
#include "./constantTerms.h"
#include "../recordStruct/account.h"
#include "../recordStruct/employee.h"
#include "../recordStruct/structs.h"
#include "../recordStruct/loanapply.h"
#include "../recordStruct/client_data.h"
#include "../resource/commanFun.h"
bool employee_login_handler(int connFD, struct Employee *ptrToEmpID,struct clientData *clientData,int isManager);
// char* getRole(int role);
// int get_last_number_of_loginID(char *input);



bool employeee_login_handler(int connFD, struct Employee *ptrToEmpID,struct clientData *clientData,int isManager)
{
    ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
    char readBuffer[1000], writeBuffer[1000]; // Buffer for reading from / writing to the client
    char tempBuffer[1000];
    struct Employee account;

    int ID;

    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer, sizeof(writeBuffer));

    // Get login message for respective user type
    
    strcpy(writeBuffer, EMPLOYEE_LOGIN_WELCOME);

    // Append the request for LOGIN ID message
    strcat(writeBuffer, "\n");
    strcat(writeBuffer, ENTER_LOGIN_ID);

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing WELCOME & LOGIN_ID message to the client!");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading login ID from client!");
        return false;
    }

    bool userFound = false;

   
    bzero(tempBuffer, sizeof(tempBuffer));
    strcpy(tempBuffer, readBuffer);

    ID  =  get_last_number_of_loginID(tempBuffer);
    int customerFileFD = open(EMPLOYEE_FILE, O_RDONLY);
    if (customerFileFD == -1)
    {
        perror("Error opening customer file in read mode!");
        return false;
    }

    off_t offset = lseek(customerFileFD, ID * sizeof(struct Employee), SEEK_SET);
    if (offset >= 0)
    {
        struct flock lock = {F_RDLCK, SEEK_SET, ID * sizeof(struct Employee), sizeof(struct Employee), getpid()};

        int lockingStatus = fcntl(customerFileFD, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on customer record!");
            return false;
        }

        readBytes = read(customerFileFD, &account, sizeof(struct Employee));
        if (readBytes == -1)
        {
            ;
            perror("Error reading customer record from file!");
        }

        lock.l_type = F_UNLCK;
        fcntl(customerFileFD, F_SETLK, &lock);
        char accountNumberStr[100];
        // Buffer to hold the string representation of the account number
        snprintf(accountNumberStr, sizeof(accountNumberStr), "%s", account.login); 
        // Convert int to string
        if (strcmp(accountNumberStr, readBuffer) == 0)
            userFound = true;
        
            else{
                writeBytes = write(connFD, INVALID_LOGIN, strlen(INVALID_LOGIN));
                    close(customerFileFD);
            return false;
        }
            
        close(customerFileFD);
    }
    else
    {
        writeBytes = write(connFD, EMPLOYEE_LOGIN_ID_DOESNT_EXIT, strlen(EMPLOYEE_LOGIN_ID_DOESNT_EXIT));
    }


    if (userFound)
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, ENTER_PASSWORD, strlen(ENTER_PASSWORD));
        if (writeBytes == -1)
        {
            perror("Error writing PASSWORD message to client!");
            return false;
        }

        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == 1)
        {
            perror("Error reading password from the client!");
            return false;
        }

        char hashedPassword[1000];
        strcpy(hashedPassword, crypt(readBuffer, SALT_BAE));
       
        if (strcmp(hashedPassword, account.password) == 0)
            {


              if(!account.active){
                  bzero(writeBuffer, sizeof(writeBuffer));
                write(connFD,"Account is currently Not Active Please contact your Bank admin!$", strlen("Account is currently Not Active Please contact your Bank admin!$"));
                 return false;
               }
               if((account.role)!=isManager){
                 bzero(writeBuffer, sizeof(writeBuffer));
                 write(connFD,"You are not a employee who you are pretending of!$", strlen("You are not a employee who you are pretending of!$"));
                 return false;
               }
               if(!add_to_shared_set(account.login))
               {
                bzero(writeBuffer, sizeof(writeBuffer));
                writeBytes = write(connFD,CUSTOMER_ALREADY_LOGGED_IN, strlen(CUSTOMER_ALREADY_LOGGED_IN));
                return false;
               }
               *ptrToEmpID=account;
                strcpy((*clientData).name ,account.name);
                strcpy((*clientData).username,account.login);
                (*clientData).userid=account.empID;
                   strcpy((*clientData).password,account.password);
               return true;
              
            }
        

        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, INVALID_PASSWORD, strlen(INVALID_PASSWORD));
    }
    else
    {
        bzero(writeBuffer, sizeof(writeBuffer));
        writeBytes = write(connFD, INVALID_LOGIN, strlen(INVALID_LOGIN));
    }

    return false;
}


// char* getRole(int role){
//     if(role==0) return "Manager";
//     return "Employee";
// }
// int get_last_number_of_loginID(char *input){
//     char *token;
//     token = strtok(input, "-");
//     token = strtok(NULL, "-");
//     return atoi(token);
// }
#endif