#ifndef LOANAPPLY
#define LOANAPPLY

#include <time.h>

struct Loanapply
{
    int loanid;
    int accountNumber;
    char custName[30];
    char custLogID[30];
    // char nameAccountHolder;
    long int newBalance;
    int handleByEmpID;
    char nameEmployee[30];    
    int status; //0->applied , 1->assigned ,2->approved,3->declined,4->processed
    int approvedByEMP;
    //  char transactionTime[100];
    char processedTime[100];
    char appliedTime[100];
};

#endif