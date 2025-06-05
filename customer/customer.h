#ifndef CUSTOMER_FUNCTIONS
#define CUSTOMER_FUNCTIONS

// #include "../resource/constantTerms.h"
// // #include "../recordStruct/structs.h"
// #include "../recordStruct/client_data.h"
// #include "../resource/commanFun.h"
// #include "../resource/set.h"
// #include "../resource/shFile.h"
#include <sys/stat.h>  // For file mode constants like S_IRWXU
#include <crypt.h>
#include <stdbool.h>

// Semaphores are necessary joint account due the design choice I've made
#include <sys/ipc.h>
#include <sys/sem.h>


bool customerDriver(int connFD);

void get_customer_NAME(int connFD,struct clientData clientData);
bool deposit(int connFD,char *uname,int amount,int typeofTrans);
bool withdraw(int connFD,char *uname,int amount,int typeofTrans);
bool get_balance(int connFD,struct clientData clientData);
bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *semOp);
void write_transaction_to_array(int *transactionArray, int ID);
int write_transaction_to_file(char *name,int accountNumber, long int oldBalance, long int newBalance, int operation);
bool addFeedback(int connFD, struct clientData clientData);
bool applyLoan(int connFD,struct clientData clientData);
bool transferFund(int connFD,char *username,int type);
int semIdentifier;


bool customerDriver(int connFD)
{

    struct clientData clientData;
    if (login_handler(false, connFD, NULL, &clientData))
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
        strcpy(writeBuffer, CUSTOMER_LOGIN_WELCOME);
        while (1)
        {
            bzero(writeBuffer, sizeof(writeBuffer));
            bzero(readBuffer, sizeof(readBuffer));
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, CUSTOMER_MENU);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
          

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                get_customer_NAME(connFD, clientData);
                break;
            case 2:
                deposit(connFD,clientData.username,0,TRANSACTION_TYPE_DEPOSIT);
                break;
            case 3:
                withdraw(connFD,clientData.username,0,TRANSACTION_TYPE_WITHDRAW);
                break;
            case 4:
                get_balance(connFD,clientData);
                break;
            case 5:

                view_transections(connFD,clientData.username);
                break;
            case 6:
                change_password(connFD,CUSTR_TYPE,semIdentifier,clientData);
                break;
            case 7:
                transferFund(connFD,clientData.username,3);
                break;
            case 8:
                addFeedback(connFD,clientData);
                break;
            case 9:
                applyLoan(connFD,clientData);
                break;
            case 10:
                logout(connFD,clientData.username);
                break;
            default:
                write(connFD,"Invalid Input! Please enter Valid choice\n",strlen("Invalid Input! Please enter Valid choice\n"));
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
bool transferFund(int connFD,char *username,int type)
{
    char readBuffer[1000], writeBuffer[1000],tempCustID[1000],tempCustnameToTransfer[1000];
    ssize_t writeBytes,readBytes;
    writeBytes= write(connFD,"Enter the Customer ID To Transfer the Fund\n",strlen("Enter the Customer ID To Transfer the Fund\n"));
    if(writeBytes==-1){
        perror("Error while writing the Get ID message to Client");
        return false;
    }
    bzero(readBuffer,sizeof(readBuffer));
    readBytes=read(connFD,readBuffer,sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading LOAM MESSAGE from client!");
        return false;
    }
      strcpy(tempCustID, readBuffer);
    if(!get_customer_details(connFD,get_last_number_of_loginID(tempCustID),readBuffer,1)) return false;
     writeBytes= write(connFD,"\nEnter the amount you want to transfer\n",strlen("\nEnter the amount you want to transfer\n"));
    if(writeBytes==-1){
        perror("Error while writing the Get ID message to Client");
        return false;
    }
    strcpy(tempCustnameToTransfer,readBuffer);
    bzero(readBuffer,sizeof(readBuffer));
    readBytes=read(connFD,readBuffer,sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading LOAM MESSAGE from client!");
        return false;
    }
    if(atoi(readBuffer)==0){
          writeBytes= write(connFD,"Invalid Amount\n",strlen("Invalid Amount\n"));
            if(writeBytes==-1){
                 perror("Error while writing the Get ID message to Client");
             return false;
             }
             return true;
    }
    int amount = atoi(readBuffer);
    withdraw(connFD,username,amount,TRANSACTION_TYPE_TRANSFER);
    deposit(connFD,tempCustnameToTransfer,amount,TRANSACTION_TYPE_TRANSFER);
    return true;

}

bool applyLoan(int connFD,struct clientData clientData){
    struct Loanapply loan,prevLoan;
     char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    writeBytes = write(connFD,CUSTOMER_LOAN_MENU,strlen(CUSTOMER_LOAN_MENU));
    if (writeBytes == -1)
            {
                perror("Error writing LOAN MESSAGE to client");
                return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading LOAM MESSAGE from client!");
        return false;
    }

    /////////
    if(atoi(readBuffer)!=1&&atoi(readBuffer)!=2){
        write(connFD,"Invalid input",strlen("Invalid input"));
        return true;
    }

    /////////////  Print loan apps
    if(atoi(readBuffer)==2){
        // write(connFD,"LIST of Loan",strlen("LIST of Loan"));
        printLoanListofUser(connFD,clientData.username);
        return true;
    }
    writeBytes = write(connFD,CUSTOMER_LOAN_APPLY,strlen(CUSTOMER_LOAN_APPLY));
    if (writeBytes == -1)
            {
                perror("Error writing LOAN MESSAGE to client");
                return false;
    }
    bzero(readBuffer,sizeof(readBuffer));
    readBytes=read(connFD,readBuffer,sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading ammount from client!");
        return false;
    }
    int ammountToApply = atoi(readBuffer);
    loan.accountNumber=clientData.userid;
    loan.newBalance=ammountToApply;
    loan.handleByEmpID=-1;
    loan.approvedByEMP=-1;
    strcpy(loan.nameEmployee,"No Assigned");
    strcpy(loan.custName,clientData.name);
    strcpy(loan.custLogID,clientData.username);
    loan.status=0;
    strftime(loan.appliedTime, sizeof(loan.appliedTime), "%Y-%m-%d %H:%M:%S", timeInfo);

    //#########################3 FILE LOCKING

    // int customerFileDescriptor = open(LOAN_FILE, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    // struct flock lock;
    // lock.l_type = F_WRLCK;  // Write lock
    // lock.l_whence = SEEK_SET;
    // lock.l_start = 0;       // Start of the file
    // lock.l_len = 0;         // 0 means "until EOF" (entire file)
    // lock.l_pid = getpid(); 
    
    
    // ssize_t bytesWritten = write(customerFileDescriptor, &loan, sizeof(loan));
    // if (bytesWritten == -1) {
    //     perror("Error writing to file");
    //     close(customerFileDescriptor);
    //     return 1;
    // }
    // lock.l_type = F_UNLCK;
    // if (fcntl(customerFileDescriptor, F_SETLK, &lock) == -1) {
    //     perror("Error unlocking file");
    // }
    // close(customerFileDescriptor);
    // write(connFD,"Loan Request is Sent to the bank!",strlen("Loan Request is Sent to the bank!"));
    // return true;
    int loanFileDescriptor = open(LOAN_FILE, O_RDONLY);
    if (loanFileDescriptor == -1 && errno == ENOENT)
    {
        // Customer file was never created
        loan.loanid=0;
    }
    else if (loanFileDescriptor == -1)
    {
        perror("Error while opening customer file");
        return -1;
    }else{
        int offset = lseek(loanFileDescriptor, -sizeof(struct Loanapply), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Customer record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Loanapply), getpid()};
        int lockingStatus = fcntl(loanFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Customer record!");
            return false;
        }
        struct Loanapply previousloan;
        readBytes = read(loanFileDescriptor, &previousloan, sizeof(struct Loanapply));
        if (readBytes == -1)
        {
            perror("Error while reading Customer record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(loanFileDescriptor, F_SETLK, &lock);

        close(loanFileDescriptor);
        loan.loanid=previousloan.loanid+1;
    }
    loanFileDescriptor = open(LOAN_FILE, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (loanFileDescriptor == -1)
    {
        perror("Error while creating / opening customer file!");
        return false;
    }
    writeBytes = write(loanFileDescriptor, &loan, sizeof(loan));
    if (writeBytes == -1)
    {
        perror("Error while writing Customer record to file!");
        return false;
    }

    close(loanFileDescriptor);
    
    return true;
}
bool addFeedback(int connFD, struct clientData clientData){
    struct CustomerFeedback CustomerFeedback,prevCustomerFeedback;
    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    writeBytes = write(connFD,CUSTOMER_FEEDBACK,strlen(CUSTOMER_FEEDBACK));
    if (writeBytes == -1)
            {
                perror("Error writing FEEDBACK MESSAGE to client");
                return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading FEEDBACK MESSAGE from client!");
        return false;
    }
    strcpy(CustomerFeedback.feedback,readBuffer);
    CustomerFeedback.accountNumber=clientData.userid;
    strcpy(CustomerFeedback.login,clientData.username);
    strcpy(CustomerFeedback.name,clientData.name);
    strftime(CustomerFeedback.transactionTime, sizeof(CustomerFeedback.transactionTime), "%Y-%m-%d %H:%M:%S", timeInfo);
    
    //#########################3 FILE LOCKING

    int customerFileDescriptor = open(FEEDBACK_FILE, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    struct flock lock;
    lock.l_type = F_WRLCK;  // Write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;       // Start of the file
    lock.l_len = 0;         // 0 means "until EOF" (entire file)
    lock.l_pid = getpid(); 
    
   
    ssize_t bytesWritten = write(customerFileDescriptor, &CustomerFeedback, sizeof(CustomerFeedback));
    if (bytesWritten == -1) {
        perror("Error writing to file");
        close(customerFileDescriptor);
        return 1;
    }
    lock.l_type = F_UNLCK;
    if (fcntl(customerFileDescriptor, F_SETLK, &lock) == -1) {
        perror("Error unlocking file");
    }
    close(customerFileDescriptor);
    write(connFD,"Feedback sent to company!",strlen("Feedback sent to company!"));
    return true;
    


}
bool deposit(int connFD,char *uname,int amount,int typeofTrans)
{
    char readBuffer[1000], writeBuffer[1000],tempBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Account account;
    strcpy(tempBuffer, uname);
    account.accountNumber = get_last_number_of_loginID(tempBuffer);

    int depositAmount = 0;

    // Lock the critical section
    struct sembuf semOp;
    lock_critical_section(&semOp);

    if (get_account_details(connFD, &account))
    {
        // write(STDOUT_FILENO,account.name,strlen(account.name));
        if (account.active)
        {
            if(amount==0){
            writeBytes = write(connFD, DEPOSIT_AMOUNT, strlen(DEPOSIT_AMOUNT));
            if (writeBytes == -1)
            {
                perror("Error writing DEPOSIT_AMOUNT to client!");
                unlock_critical_section(&semOp);
                return false;
            }

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error reading deposit money from client!");
                unlock_critical_section(&semOp);
                return false;
            }
            depositAmount = atoi(readBuffer);
            }else{
                depositAmount=amount;
            }

            if (depositAmount != 0)
            {

                int newTransactionID = write_transaction_to_file(account.login,account.accountNumber, account.balance, account.balance + depositAmount, typeofTrans);
                write_transaction_to_array(account.transactions, newTransactionID);

                account.balance += depositAmount;
                write(STDOUT_FILENO,&depositAmount,sizeof(depositAmount));
                int accountFileDescriptor = open(ACCOUNT_FILE, O_WRONLY);
                off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Account), SEEK_SET);

                struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
                int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                if (lockingStatus == -1)
                {
                    perror("Error obtaining write lock on account file!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                writeBytes = write(accountFileDescriptor, &account, sizeof(struct Account));
                if (writeBytes == -1)
                {
                    perror("Error storing updated deposit money in account record!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                lock.l_type = F_UNLCK;
                fcntl(accountFileDescriptor, F_SETLK, &lock);

                write(connFD, DEPOSIT_AMOUNT_SUCCESS, strlen(DEPOSIT_AMOUNT_SUCCESS));
                // read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

                // get_balance(connFD,clientData);

                unlock_critical_section(&semOp);

                // return true;
            }
            else
                writeBytes = write(connFD, DEPOSIT_AMOUNT_INVALID, strlen(DEPOSIT_AMOUNT_INVALID));
        }
        else
            write(connFD, ACCOUNT_DEACTIVATED, strlen(ACCOUNT_DEACTIVATED));
        read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

        unlock_critical_section(&semOp);
    }
    else
    {
        // FAIL
        unlock_critical_section(&semOp);
        return false;
    }
}

bool withdraw(int connFD,char *uname,int amount,int typeofTrans)
{
    char readBuffer[1000], writeBuffer[1000],tempBuffer[1000];
    ssize_t readBytes, writeBytes;

    struct Account account;
    strcpy(tempBuffer, uname);
    account.accountNumber = get_last_number_of_loginID(tempBuffer);


    long int withdrawAmount = 0;

    // Lock the critical section
    struct sembuf semOp;
    lock_critical_section(&semOp);

    if (get_account_details(connFD, &account))
    {
        if (account.active)
        {
            if(amount==0){
            writeBytes = write(connFD, WITHDRAW_AMOUNT, strlen(WITHDRAW_AMOUNT));
            if (writeBytes == -1)
            {
                perror("Error writing WITHDRAW_AMOUNT message to client!");
                unlock_critical_section(&semOp);
                return false;
            }

            bzero(readBuffer, sizeof(readBuffer));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error reading withdraw amount from client!");
                unlock_critical_section(&semOp);
                return false;
            }

            withdrawAmount = atol(readBuffer);
            }else{
                withdrawAmount=amount;
            }

            if (withdrawAmount != 0 && account.balance - withdrawAmount >= 0)
            {

                int newTransactionID = write_transaction_to_file(account.login,account.accountNumber, account.balance, account.balance - withdrawAmount, typeofTrans);
                write_transaction_to_array(account.transactions, newTransactionID);

                account.balance -= withdrawAmount;

                int accountFileDescriptor = open(ACCOUNT_FILE, O_WRONLY);
                off_t offset = lseek(accountFileDescriptor, account.accountNumber * sizeof(struct Account), SEEK_SET);

                struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Account), getpid()};
                int lockingStatus = fcntl(accountFileDescriptor, F_SETLKW, &lock);
                if (lockingStatus == -1)
                {
                    perror("Error obtaining write lock on account record!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                writeBytes = write(accountFileDescriptor, &account, sizeof(struct Account));
                if (writeBytes == -1)
                {
                    perror("Error writing updated balance into account file!");
                    unlock_critical_section(&semOp);
                    return false;
                }

                lock.l_type = F_UNLCK;
                fcntl(accountFileDescriptor, F_SETLK, &lock);

                write(connFD, WITHDRAW_AMOUNT_SUCCESS, strlen(WITHDRAW_AMOUNT_SUCCESS));
                // read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

                // get_balance(connFD,clientData);

                unlock_critical_section(&semOp);

                // return true;
            }
            else
                writeBytes = write(connFD, WITHDRAW_AMOUNT_INVALID, strlen(WITHDRAW_AMOUNT_INVALID));
        }
        else
            write(connFD, ACCOUNT_DEACTIVATED, strlen(ACCOUNT_DEACTIVATED));
        read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

        unlock_critical_section(&semOp);
    }
    else
    {
        // FAILURE while getting account information
        unlock_critical_section(&semOp);
        return false;
    }
}

bool get_balance(int connFD,struct clientData clientData)
{
    char buffer[1000];
    struct Account account;
    account.accountNumber = get_last_number_of_loginID(clientData.username);
    if (get_account_details(connFD, &account))
    {
    // write(STDOUT_FILENO,clientData.username,sizeof(clientData.username));
        bzero(buffer, sizeof(buffer));
        if (account.active)
        {
            sprintf(buffer, "You have ₹ %ld imaginary money in our bank!^", account.balance);
            write(connFD, buffer, strlen(buffer));
        }
        else
            write(connFD, ACCOUNT_DEACTIVATED, strlen(ACCOUNT_DEACTIVATED));
        read(connFD, buffer, sizeof(buffer)); // Dummy read
    }
    else
    {
        // ERROR while getting balance
        return false;
    }
}

bool lock_critical_section(struct sembuf *semOp)
{
    semOp->sem_flg = SEM_UNDO;
    semOp->sem_op = -1;
    semOp->sem_num = 0;
    int semopStatus = semop(semIdentifier, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }
    return true;
}

bool unlock_critical_section(struct sembuf *semOp)
{
    semOp->sem_op = 1;
    int semopStatus = semop(semIdentifier, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while operating on semaphore!");
        _exit(1);
    }
    return true;
}

void write_transaction_to_array(int *transactionArray, int ID)
{
    // Check if there's any free space in the array to write the new transaction ID
    int iter = 0;
 

    while (transactionArray[iter] != 0)
        iter++;

    if (iter >= MAX_TRANSACTIONS)
    {
        // No space
        for (iter = 1; iter < MAX_TRANSACTIONS; iter++)
            // Shift elements one step back discarding the oldest transaction
            transactionArray[iter - 1] = transactionArray[iter];
        transactionArray[iter - 1] = ID;
    }
    else
    {
        // Space available
        transactionArray[iter] = ID;
    }
}

int write_transaction_to_file(char *name,int accountNumber, long int oldBalance, long int newBalance, int operation)
{
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    struct Transaction newTransaction;
    newTransaction.accountNumber = accountNumber;
    newTransaction.oldBalance = oldBalance;
    newTransaction.newBalance = newBalance;
    newTransaction.operation = operation;
   newTransaction.transferAcc=-1;
       strcpy(newTransaction.loginID,name);
    // newTransaction.transactionTime = time(NULL);
    strftime(newTransaction.transactionTime, sizeof(newTransaction.transactionTime), "%Y-%m-%d %H:%M:%S", timeInfo);

    ssize_t readBytes, writeBytes;

    int transactionFileDescriptor = open(TRANSACTION_FILE, O_CREAT | O_APPEND | O_RDWR, S_IRWXU);

    // Get most recent transaction number
    off_t offset = lseek(transactionFileDescriptor, -sizeof(struct Transaction), SEEK_END);
    if (offset >= 0)
    {
        // There exists at least one transaction record
        struct Transaction prevTransaction;
        readBytes = read(transactionFileDescriptor, &prevTransaction, sizeof(struct Transaction));

        newTransaction.transactionID = prevTransaction.transactionID + 1;
    }
    else
        // No transaction records exist
        newTransaction.transactionID = 0;

    writeBytes = write(transactionFileDescriptor, &newTransaction, sizeof(struct Transaction));

    return newTransaction.transactionID;
}

// =====================================================



void get_customer_NAME(int connFD,struct clientData clientData){
    
    char buffer[1000];  // Buffer to hold the string representation of the number
// long int depositAmount = 12345;

// // Convert the long int to a string
strcpy(buffer,clientData.username);
// sprintf(buffer, "--------------%s------------", clientData.username);  // %ld formats it as long int

// // Now use write to print the string to STDOUT
write(STDOUT_FILENO, buffer, strlen(buffer));
    get_customer_details(connFD,clientData.userid,clientData.username,0);
}
#endif
