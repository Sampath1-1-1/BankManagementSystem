#ifndef EMPLOYEE_RECORD
#define EMPLOYEE_RECORD

#define MAX_TRANSACTIONS 100

struct Employee
{
    char name[25];
    char gender;
    int age;
    char login[30]; // Format : nameaccountID (name-accountID)
    char password[30];
    int empID;     // 0, 1, 2, ....
    int role; //0 -> manager 1->employee
    int active;           // 1 -> Active, 0 -> Deactivated (Deleted)
  
};

#endif