#ifndef SET
#define SET
#include <stdio.h>
#include <string.h>
#include "./shFile.h"
#include <stdbool.h>
#define MAX_SET_SIZE 100 
#define MAX_STR_LEN 100   
void display_set(char set[][MAX_STR_LEN], int size) {
    printf("Set of strings:\n");
    for (int i = 0; i < size; i++) {
        printf("%s\n", set[i]);
    }
}
bool is_present(char set[][MAX_STR_LEN], int size, const char *str){
     for (int i = 0; i < size; i++) {
        if (strcmp(set[i], str) == 0) {
            return true;  
        }
    }
    return false;  
}
bool add_to_shared_set(const char *str) {
    
    // return false;
    if (!is_present(shared_set, *shared_set_size, str)) {
        strcpy(shared_set[*shared_set_size], str);  
        (*shared_set_size)++;     
        display_set(shared_set, *shared_set_size);
        return true;        
    }else return false;
}
// Function to remove a string from the shared set
bool remove_from_shared_set(const char *str) {
    // Find if the string is present in the set
    for (int i = 0; i < *shared_set_size; i++) {
        if (strcmp(shared_set[i], str) == 0) {
            // Shift elements to the left to remove the string
            for (int j = i; j < *shared_set_size - 1; j++) {
                strcpy(shared_set[j], shared_set[j + 1]);
            }
            // Decrease the size of the set
            (*shared_set_size)--;
            display_set(shared_set, *shared_set_size); // Display updated set
            return true;  // String found and removed
        }
    }
    return false;  // String not found
}

#endif 
