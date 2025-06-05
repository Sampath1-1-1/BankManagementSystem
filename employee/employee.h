#ifndef EMPLOYEE_FUNCTIONS
#define EMPLOYEE_FUNCTIONS

#include "../resource/commanFun.h"
#include "../admin/admin.h"
#include "../resource/constantTerms.h"
#include "../resource/employeeNeeds.h"
#include "../recordStruct/structs.h"
#include "../recordStruct/client_data.h"
#include "../resource/set.h"
#include "../resource/shFile.h"
#include <sys/stat.h>  // For file mode constants like S_IRWXU
#include <crypt.h>
#include <stdbool.h>

// Semaphores are necessary joint account due the design choice I've made
#include <sys/ipc.h>
#include <sys/sem.h>


// Function Prototypes =================================

bool employee_operation_handler(int connFD);
// bool view_employee_account(int connFD,int type,int range,char *str);
bool View_account_trans(int connFD);
bool view_assigned_loans(int connFD,int ID,int reqType,struct Loanapply (*loan)[100],int *count);
bool approve_reject(int connFD,struct clientData clientdata);
bool process_loan(int connFD,struct clientData clientdata);
// bool logout(int connFD)
// bool login_handler(bool isAdmin, int connFD, struct Account *ptrToCustomer);

// =====================================================

// Function Definition =================================

// =====================================================
int semIdentifier;

bool employee_operation_handler(int connFD)
{    
    struct clientData clientData;
    struct Employee employee;
     
    if (employeee_login_handler(connFD,&employee,&clientData,1))
    {
        ssize_t writeBytes, readBytes;            // Number of bytes read from / written to the client
        char readBuffer[1000], writeBuffer[1000]; // A buffer used for reading & writing to the client
       
        key_t semKey = ftok(ACCOUNT_FILE, clientData.userid); 
        union semun
        {
            int val; 
        } semSet;

        int semctlStatus;
        semIdentifier = semget(semKey, 1, 0); 
        if (semIdentifier == -1)
        {
            semIdentifier = semget(semKey, 1, IPC_CREAT | 0700);
            if (semIdentifier == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            semSet.val = 1; 
            semctlStatus = semctl(semIdentifier, 0, SETVAL, semSet);
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }


        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, EMPLOYEE_LOGIN_WELCOME);
        while (1)
        {
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, EMPLOYEE_MENU);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(readBuffer);
            switch (choice)
            {
            
            case 1:
                add_customer(connFD);
                break;
            case 2: 
                updateDetails(connFD, false);
                break;
            case 3:
                process_loan(connFD,clientData);
                break;
            case 4:
                //Approve and reject;
                approve_reject(connFD,clientData);
                break;
            case 5:
                view_assigned_loans(connFD,clientData.userid,0,NULL,0);
                break;
            case 6:
                View_account_trans(connFD);
                break;
            case 7:
                change_password(connFD,EMPLY_TYPE,semIdentifier,clientData);            
                break;
            case 8:
                logout(connFD,clientData.username);
                break;
            default:
                write(connFD,"Enter Valid Input\n",strlen("Enter Valid Input\n"));
                break;
            }
        }
    }
    else
    {
        return false;
    }
    return true;
}

bool approve_reject(int connFD,struct clientData clientdata){
         struct Loanapply loan[100];
        ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
        char readBuffer[50000], writeBuffer[50000]; // Buffer for reading from / writing to the client
        char tempBuffer[50000];
         int count=0;
         bzero(loan, sizeof(loan));  // Set entire array to zero (nil)/
         view_assigned_loans(connFD,clientdata.userid,1,&loan,&count);
        bool flagexist=true;
        for (int i=0;i<count;i++) {
        // struct Employee employee = loan[i];
             bzero(writeBuffer,sizeof(writeBuffer));
            if(loan[i].status <2)
            {   flagexist=false;
                sprintf(writeBuffer, "\nLoan No *- %d\n\tCustomer Name : %s\n\tAmount : %ld\n\tApplied Time : %s\n\n", loan[i].loanid,loan[i].custName,loan[i].newBalance,loan[i].appliedTime);
                write(connFD,writeBuffer,strlen(writeBuffer));
            }
        }

        if(flagexist){
            write(connFD,"No Loan application are there for loan Verification process!",strlen("No Loan application are there for loan Verification process!"));
            return true;
        }
        writeBytes = write(connFD,"Enter The Loan ID Which you want to approve or reject",strlen("Enter The Loan ID Which you want to approve or reject"));
          if (writeBytes == -1)
    {
        perror("Error writing Loan info to client!");
        return false;
    }
        readBytes =  read(connFD,readBuffer,sizeof(readBuffer));
         if (readBytes == -1)
    {
        perror("Error while reading Employee from the Employee!");
        return false;
    }
    int targetLoan = atoi(readBuffer);
    bzero(readBuffer,sizeof(readBuffer));
    writeBytes = write(connFD,"You want to Approve it or  Reject it \n\t1. Approve Press 1\n\t2. Reject Press 2",strlen("You want to Approve it or  Reject it \n\t1. Approve Press 1\n\t2. Reject Press 2"));
          if (writeBytes == -1)
    {
        perror("Error writing Loan info to client!");
        return false;
    }
        readBytes =  read(connFD,readBuffer,sizeof(readBuffer));
         if (readBytes == -1)
    {
        perror("Error while reading Employee from the Employee!");
        return false;
    }
    int choice = atoi(readBuffer);
    if(choice!=1&&choice!=2){
        write(connFD,"Invalid Input",strlen("Invalid Input"));
        return true;
    }
    int customerFileDescriptor = open(LOAN_FILE, O_RDWR);
     if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO LOAN EXIST");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    
    // struct Loanapply temp= loanarray[targetLoanID];
    int offset=lseek(customerFileDescriptor, targetLoan*sizeof(struct Loanapply), SEEK_SET);
     if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "No Loan Exist");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }
    struct flock lock1 = {F_RDLCK, SEEK_SET, offset, sizeof(struct Loanapply), getpid()};
    int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock1);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on the Account file!");
                return false;
            }
    if (fcntl(customerFileDescriptor, F_SETLKW, &lock1) == -1) {
        perror("Error obtaining write lock");
        close(customerFileDescriptor);
        return false;
    }
  
    struct Loanapply loantemp;
    readBytes = read(customerFileDescriptor, &loantemp, sizeof(struct Loanapply));
    
    if (readBytes == -1)
    {
        perror("Error while reading customer record from the file!");
        return false;
    }
    // Unlock the record
    lock1.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock1);
    close(customerFileDescriptor);
    loantemp.approvedByEMP=clientdata.userid;
    loantemp.status=choice==1?2:3;

    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(loantemp.processedTime, sizeof(loantemp.processedTime), "%Y-%m-%d %H:%M:%S", timeInfo);

    customerFileDescriptor = open(LOAN_FILE,O_WRONLY);
       if (customerFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return false;
    }
    offset = lseek(customerFileDescriptor, loantemp.loanid*sizeof(struct Loanapply), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }

    lock1.l_type = F_WRLCK;
    lock1.l_start = offset;
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock1);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on customer record!");
        return false;
    }
     writeBytes = write(customerFileDescriptor,  &loantemp, sizeof(struct Loanapply));
    if (writeBytes == -1)
    {
        perror("Error while writing update customer info into file");
    }

    lock1.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLKW, &lock1);

    close(customerFileDescriptor);
    writeBytes = write(connFD, "Loan Verification Succesful!^", sizeof("Loan Verification Succesful!^"));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_CUSTOMER_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;
return true;
}


bool process_loan(int connFD,struct clientData clientdata){
         struct Loanapply loan[100];
        ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
        char readBuffer[50000], writeBuffer[50000]; // Buffer for reading from / writing to the client
        char tempBuffer[50000];
         int count=0;
         bzero(loan, sizeof(loan));  // Set entire array to zero (nil)/
         view_assigned_loans(connFD,clientdata.userid,1,&loan,&count);
        bool flagexist=true;
        for (int i=0;i<count;i++) {
        // struct Employee employee = loan[i];
             bzero(writeBuffer,sizeof(writeBuffer));
            if(loan[i].status >1&&loan[i].status<3)
            {   flagexist=false;
                sprintf(writeBuffer, "\nLoan No *- %d\n\tCustomer Name : %s\n\tAmount : %ld\n\tApplied Time : %s\n\n", loan[i].loanid,loan[i].custName,loan[i].newBalance,loan[i].appliedTime);
                write(connFD,writeBuffer,strlen(writeBuffer));
            }
        }

        if(flagexist){
            write(connFD,"No Loan application are there for loan Verification process!",strlen("No Loan application are there for loan Verification process!"));
            return true;
        }
        writeBytes = write(connFD,"Enter The Loan ID Which you want to Process",strlen("Enter The Loan ID Which you want to Process"));
          if (writeBytes == -1)
    {
        perror("Error writing Loan info to client!");
        return false;
    }
        readBytes =  read(connFD,readBuffer,sizeof(readBuffer));
         if (readBytes == -1)
    {
        perror("Error while reading Employee from the Employee!");
        return false;
    }
    int targetLoan = atoi(readBuffer);
    bzero(readBuffer,sizeof(readBuffer));
    int customerFileDescriptor = open(LOAN_FILE, O_RDWR);
     if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO LOAN EXIST");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    
    // struct Loanapply temp= loanarray[targetLoanID];
    int offset=lseek(customerFileDescriptor, targetLoan*sizeof(struct Loanapply), SEEK_SET);
     if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "No Loan Exist");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing CUSTOMER_ID_DOESNT_EXIT message to client!");
            return false;
        }
        // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }
    struct flock lock1 = {F_RDLCK, SEEK_SET, offset, sizeof(struct Loanapply), getpid()};
    int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock1);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on the Account file!");
                return false;
            }
    if (fcntl(customerFileDescriptor, F_SETLKW, &lock1) == -1) {
        perror("Error obtaining write lock");
        close(customerFileDescriptor);
        return false;
    }
  
    struct Loanapply loantemp;
    readBytes = read(customerFileDescriptor, &loantemp, sizeof(struct Loanapply));
    
    if (readBytes == -1)
    {
        perror("Error while reading customer record from the file!");
        return false;
    }
    // Unlock the record
    lock1.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock1);
    close(customerFileDescriptor);
    // loantemp.approvedByEMP=clientdata.userid;
    loantemp.status=4;

    // time_t currentTime;
    // struct tm *timeInfo;
    // time(&currentTime);
    // timeInfo = localtime(&currentTime);
    // strftime(loantemp.processedTime, sizeof(loantemp.processedTime), "%Y-%m-%d %H:%M:%S", timeInfo);

    //
     customerFileDescriptor = open(LOAN_FILE,O_WRONLY);
       if (customerFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return false;
    }
    offset = lseek(customerFileDescriptor, loantemp.loanid*sizeof(struct Loanapply), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required customer record!");
        return false;
    }

    lock1.l_type = F_WRLCK;
    lock1.l_start = offset;
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock1);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on customer record!");
        return false;
    }
     writeBytes = write(customerFileDescriptor,  &loantemp, sizeof(struct Loanapply));
    if (writeBytes == -1)
    {
        perror("Error while writing update customer info into file");
    }

    lock1.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLKW, &lock1);

    close(customerFileDescriptor);
    writeBytes = write(connFD, "Loan Verification Succesful!^", sizeof("Loan Verification Succesful!^"));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_CUSTOMER_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    // return true;
    deposit(connFD,loantemp.custLogID,loantemp.newBalance,TRANSACTION_TYPE_LOANCREDIT);
return true;
}


bool view_assigned_loans(int connFD,int ID,int reqType,struct Loanapply (*loans)[100],int *count){
        ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
    char readBuffer[1000], writeBuffer[10000]; // Buffer for reading from / writing to the client
    char tempBuffer[1000];
    struct Loanapply loan;
    
    struct flock lock = {F_RDLCK, SEEK_SET, 0, 0, getpid()};
    int toSeek=0;
    int customerFileDescriptor = open(LOAN_FILE, O_RDONLY);
    if (customerFileDescriptor == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO LOANS APPLICATION");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing LOANS_APPLICATION_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return true;
    }
    
    off_t fileSize = lseek(customerFileDescriptor, 0, SEEK_END);
     if(fileSize<=toSeek*sizeof(struct Employee)){
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO LOANS APPLICATION");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing NO LOANS APPLICATION_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return true;
    }
    int offset = lseek(customerFileDescriptor, 0, SEEK_SET);

      if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO Employee");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing EMPLOYEE_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required Employee record!");
        return false;
    }

     lock.l_start = offset;

    int lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Customer file!");
        return false;
    }
    char printstr[50000];
    struct  Loanapply loanapps;
    offset = lseek(customerFileDescriptor, 0, SEEK_END); // Get the total size of the file

    lseek(customerFileDescriptor, 0, SEEK_SET); // Reset file pointer to the start of the file
    bzero(writeBuffer, sizeof(writeBuffer));
 strcat(printstr, "Loan Details as Follows : \n");
    for (off_t i = 0; i < offset; i += sizeof(struct Loanapply)) {
        
        ssize_t readBytes = read(customerFileDescriptor, &loanapps, sizeof(struct Loanapply));

        if (readBytes == sizeof(struct Loanapply)) {
            // write(STDOUT_FILENO,loanapps.custLogID,strlen(loanapps.custLogID));
            // write(STDOUT_FILENO,"\n",strlen("\n"));
            // write(STDOUT_FILENO,str,strlen(str));

            if(loanapps.handleByEmpID==ID){
                // loan[(*count)++]=loanapps;

                 if(reqType==1){
                    if(loans!=NULL)(*loans)[*count]=loanapps;
                 (*count)++;}
                bzero(writeBuffer,sizeof(writeBuffer));
            sprintf(writeBuffer, "Loan ID : %d\n\tAccount No : %d\n\tCustomer Name : %s\n\tCustomer ID : %s\n\tRequested Amount: %ld\n\tEmployee ID (handling by) : %d\n\tEmployee name : %s\n\tStatus : %s\n\tApproved by EMP ID : %d\n\tProcessed Time : %s\n\tApplied Time : %s \n",loanapps.loanid, loanapps.accountNumber,loanapps.custName,loanapps.custLogID,loanapps.newBalance,loanapps.handleByEmpID,loanapps.nameEmployee,(loanapps.status==0?"Applied":loanapps.status==1?"Assigned":loanapps.status==2?"Approved":loanapps.status==3?"Declined":"Processed"),loanapps.approvedByEMP,loanapps.processedTime,loanapps.appliedTime);
        //    *printstr+=writeBuffer;
            strcat(printstr, writeBuffer);
            }
           
            } else if (readBytes == 0) {
                break;
            } else {
                break;
            }
        }
       lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);
    if(reqType==1){
        return true;
    }
    writeBytes = write(connFD,printstr, strlen(printstr));
    
    if (writeBytes == -1)
    {
        perror("Error writing Employee info to client!");
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));
    bzero(printstr,sizeof(printstr));
    
    // readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read



return true;
}

bool View_account_trans(int connFD){
    char readBuffer[1000],writeBuffer[1000];
    write(connFD,"Enter the Customer ID to print the Pass Book",strlen("Enter the Customer ID to print the Pass Book"));
    int readBytes=read(connFD,readBuffer,sizeof(readBuffer));
    if(readBytes==-1){
        perror("Error while reading Input : ");
        return false;
    }
    view_transections(connFD,readBuffer);
    return true;
}

#endif