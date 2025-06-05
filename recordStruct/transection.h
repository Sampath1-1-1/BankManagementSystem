#ifndef TRANSACTIONS
#define TRANSACTIONS

struct Transaction
{   char loginID[30];
    int transactionID; 
    int accountNumber;
    int operation; // 1 -> Withdraw, 2 -> Deposit,3 -> Transfer,4->Loan Credit
    int transferAcc;
    long int oldBalance;
    long int newBalance;
    char transactionTime[100];

};

#endif