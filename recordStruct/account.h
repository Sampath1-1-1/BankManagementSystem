#ifndef ACCOUNT_RECORD
#define ACCOUNT_RECORD

#define MAX_TRANSACTIONS 100

struct Account
{
    char name[25];
    char gender;
    int age;
    char login[30]; // Format : nameaccountID (name-accountID)
    char password[30];
    int accountNumber;     // 0, 1, 2, ....
    // bool isRegularAccount; // 1 -> Regular account, 0 -> Joint account
    bool active;           // 1 -> Active, 0 -> Deactivated (Deleted)
    long int balance;      // Amount of money in the account
    int transactions[MAX_TRANSACTIONS];  // A list of transaction IDs. Used to look up the transactions. // -1 indicates unused space in array
};

#endif