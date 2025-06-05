#ifndef CUSTOMERFEEDBACK
#define CUSTOMERFEEDBACK

#include <time.h>

struct CustomerFeedback
{   int feedbackID;
    int accountNumber;
    char name[25];
    char login[30];
    char feedback[350];
    char transactionTime[100];
};

#endif