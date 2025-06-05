#ifndef MANAGER_FUNCTIONS
#define MANAGER_FUNCTIONS

#include "../resource/constantTerms.h"
#include "../resource/employeeNeeds.h"
#include "../recordStruct/structs.h"
#include "../recordStruct/client_data.h"
#include "../resource/commanFun.h"
#include "../resource/set.h"
#include "../resource/shFile.h"
#include <sys/stat.h>  // For file mode constants like S_IRWXU
#include <crypt.h>
#include <stdbool.h>

// Semaphores are necessary joint account due the design choice I've made
#include <sys/ipc.h>
#include <sys/sem.h>


// Function Prototypes =================================
// void assingLoanHandler(int connFD,struct Loanapply);
bool manager_operation_handler(int connFD);
bool add_account(int connFD);
int add_customer(int connFD);
int add_employee(int connFD);
bool view_feedback(int connFD);
bool assignLoans(int connFD,struct clientData);
// bool logout(int connFD)
// bool login_handler(bool isAdmin, int connFD, struct Account *ptrToCustomer);

// =====================================================

// Function Definition =================================

// =====================================================
int semIdentifier;
bool manager_operation_handler(int connFD)
{    
    struct clientData clientData;
    struct Employee employee;
     
    if (employeee_login_handler(connFD,&employee,&clientData,0))
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
            strcat(writeBuffer, MANAGER_MENU);
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
                delete_account(connFD,false);
                break;
            case 2: 
                 assignLoans(connFD,clientData);
                break;
            case 3:
                view_feedback(connFD);
                break;
            case 4:
                change_password(connFD,EMPLY_TYPE,semIdentifier,clientData);            
                break;
            case 5:
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
bool view_feedback(int connFD){
    struct CustomerFeedback feed;
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    int fileDesp = open(FEEDBACK_FILE,O_RDONLY);
    struct flock lock = {F_RDLCK, SEEK_SET, 0, 0, getpid()};

    if (fileDesp == -1)
    {
        // Customer File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "No Feedback");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing NO Feedback message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return false;
    }
    off_t fileSize = lseek(fileDesp, 0, SEEK_END);
     if(fileSize<=0){
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO FeedBack");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing NO FeedBack message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
        return true;
    }
       int offset = lseek(fileDesp, 0, SEEK_SET);

      if (errno == EINVAL)
    {
        // Customer record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "NO FeedBack");
        strcat(writeBuffer, "^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing FEEDBACK_DOESNT_EXIT message to client!");
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

    int lockingStatus = fcntl(fileDesp, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Customer file!");
        return false;
    }
    char printstr[10000];
    // struct  Loanapply loanapps;
    offset = lseek(fileDesp, 0, SEEK_END); 

    lseek(fileDesp, 0, SEEK_SET); 
    bzero(writeBuffer, sizeof(writeBuffer));
    for (off_t i = 0; i < offset; i += sizeof(struct CustomerFeedback)) {
        
        ssize_t readBytes = read(fileDesp, &feed, sizeof(struct CustomerFeedback));
        if (readBytes == sizeof(struct CustomerFeedback)) {
    
            if (readBytes == 0) {
                break;
            } 
            else if(readBytes>0){
            sprintf(writeBuffer, "FeedBack\n\tAccount No : %d\n\tName : %s\n\tCustomer ID : %s\n\tFeedBack : %s\n\tTime : %s\n",feed.accountNumber,feed.name,feed.login,feed.feedback,feed.transactionTime);
            strcat(printstr, writeBuffer);}
           
            }  else {
                break;
            }
        }
       lock.l_type = F_UNLCK;
    fcntl(fileDesp, F_SETLK, &lock);
   
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



bool  assignLoans(int connFD,struct clientData clientData) {
     ssize_t readBytes, writeBytes;            // Number of bytes written to / read from the socket
    char readBuffer[50000], writeBuffer[50000]; // Buffer for reading from / writing to the client
    char tempBuffer[50000];
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
    bzero(printstr,sizeof(printstr));
    struct Loanapply  loanarray[100];
    int loanpointer=0;
    for (off_t i = 0; i < offset; i += sizeof(struct Loanapply)) {
        
        ssize_t readBytes = read(customerFileDescriptor, &loanapps, sizeof(struct Loanapply));
        if (readBytes == sizeof(struct Loanapply)) {
            // write(STDOUT_FILENO,loanapps.custLogID,strlen(loanapps.custLogID));
            // write(STDOUT_FILENO,"\n",strlen("\n"));
            // write(STDOUT_FILENO,str,strlen(str));
            loanarray[loanpointer++]=loanapps;

            if(loanapps.handleByEmpID==-1){
             char loanDetails[1000];
              snprintf(loanDetails, sizeof(loanDetails),  "Loan Details-\n\tLoan ID : %d\n\tAccount No : %d\n\tCustomer Name : %s\n\tCustomer ID : %s\n\tRequested Amount: %ld\n\tEmployee ID (handling by) : %d\n\tEmployee name : %s\n\tStatus : %s\n\tApproved by EMP ID : %d\n\tProcessed Time : %s\n\tApplied Time : %s\n\n",loanapps.loanid, loanapps.accountNumber,loanapps.custName,loanapps.custLogID,loanapps.newBalance,loanapps.handleByEmpID,loanapps.nameEmployee,(loanapps.status==0?"Applied":loanapps.status==1?"Assigned":loanapps.status==2?"Approved":"Declined"),loanapps.approvedByEMP,loanapps.processedTime,loanapps.appliedTime);
        //    *printstr+=writeBuffer;
            strcat(printstr, loanDetails);
           
            } 
            
            }
            else if (readBytes == 0) {
                break;
            } else {
                write(connFD,"Threre is an error while reading",strlen("Threre is an error while reading"));
                perror("Error While Reading from file");
                // break;
                return true;
            }
        
     
    }
    writeBytes = write(connFD,printstr, strlen(printstr));
    
    if (writeBytes == -1)
    {
        perror("Error writing Loan info to client!");
        return false;
    }
    // bzero(writeBuffer, sizeof(writeBuffer));
    // bzero(printstr,sizeof(printstr));
    writeBytes = write(connFD,"Enter the Loan ID your want to Assign to an Employee\n", strlen("Enter the Loan ID your want to Assign to an Employee\n"));
    
    if (writeBytes == -1)
    {
        perror("Error writing Loan info to client!");
        return false;
    }
      bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD,&readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
            perror("Error while reading from client");
        else if (readBytes == 0){
          
            printf("No data was sent by the client ");
            }
   
    int targetLoanID = atoi(readBuffer);

  bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer,sizeof(writeBuffer));
     struct Employee emp[100];
     bzero(emp, sizeof(emp));  // Set entire emp array to zero (nil)
     int count=0;
     view_employee_account(connFD,1,-1,"",&emp,1,&count);
    //  sprintf(writeBuffer,"---%d---",count);
    //  write(connFD,"Enter the Employee ID you want to Assign the loan",strlen("Enter the Employee ID you want to Assign the loan"));
     for (int i=0;i<count;i++) {
        struct Employee employee = emp[i];
        bzero(writeBuffer,sizeof(writeBuffer));
        sprintf(writeBuffer, "\nEmployee No *- %d\n\tBank-EMP-ID : %d\n\tName : %s\n\tGender : %c\n\tAge: %d\n\tLoginID : %s\n\tRole : %s\n\n", i+1,employee.empID, employee.name, employee.gender, employee.age,employee.login,getRole(employee.role));
        write(connFD,writeBuffer,strlen(writeBuffer));
     }
    bzero(readBuffer, sizeof(readBuffer));
    bzero(writeBuffer,sizeof(writeBuffer));
    writeBytes = write(connFD,"Enter the Employee no * your want to Assign the Loan\n->", strlen("Enter the Employee no * your want to Assign the Loan\n->"));
    
    if (writeBytes == -1)
    {
        perror("Error writing Loan info to client!");
        return false;
    }
       bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD,&readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
            perror("Error while reading from client");
        else if (readBytes == 0){
          
            printf("No data was sent by the client ");
            }
    if(atoi(readBuffer)==0){
        write(connFD,"INVALID INPUT",strlen("INVALID INPUT"));
        return true;
    }

    int targetEMPID= atoi(readBuffer)-1;
    
    // loanarray[targetLoanID].handleByEmpID=emp[targetEMPID].empID;
    // strcpy(loanarray[targetLoanID].nameEmployee,emp[targetEMPID].login);
    lock.l_type = F_UNLCK;
    fcntl(customerFileDescriptor, F_SETLK, &lock);
    ////////////////////////
    close(customerFileDescriptor);

    customerFileDescriptor = open(LOAN_FILE, O_RDWR);
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
    offset=lseek(customerFileDescriptor, targetLoanID*sizeof(struct Loanapply), SEEK_SET);
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
    lockingStatus = fcntl(customerFileDescriptor, F_SETLKW, &lock1);
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
    loantemp.handleByEmpID=emp[targetEMPID].empID;
    loantemp.status=1;
    strcpy(loantemp.nameEmployee,emp[targetEMPID].name);

    customerFileDescriptor = open(LOAN_FILE,O_WRONLY);
       if (customerFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return false;
    }
    offset = lseek(customerFileDescriptor, targetLoanID*sizeof(struct Loanapply), SEEK_SET);
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
    writeBytes = write(connFD, "Loan Assigned Succesfully!^", sizeof("Loan Assigned Succesfully!^"));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_CUSTOMER_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read
    return true;

    
}


#endif