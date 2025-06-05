#ifndef SHFILE
#define SHFILE
#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>
#define MAX_SET_SIZE 100
#define MAX_STR_LEN 100
#define SHM_KEY 2024003   
#define SHM_KEY_2 20240031   
#define SHM_KEY_3 20240032   

char (*shared_set)[MAX_STR_LEN]; 
int *shared_set_size;            
void init_shared_memorySession_management();
void init_shared_memory_total_client();
void detach_shared_memory();


char (*shared_set)[MAX_STR_LEN];   
int *shared_set_size;        
 int *total_clients;     
void init_shared_memorySession_management() {
    int shmid = shmget(SHM_KEY_2, 1008, 0666 | IPC_CREAT);
    if (shmid < 0) {
        perror("shmget0");
                  perror("shmat0");

        return;
    }
    shared_set = shmat(shmid, NULL, 0);
    if (shared_set == (char (*)[MAX_STR_LEN]) -1) {
        perror("shmat1");
                  perror("shmat1");

        return;
    }

     int shmid2 = shmget(SHM_KEY_3, sizeof(int), 0666 | IPC_CREAT);
    if (shmid2 < 0) {
        perror("shmget0");
                  perror("shmat0");

        return;
    }
    shared_set_size = (int *)shmat(shmid2 ,NULL, 0);
    // if (*shared_set_size == 0) {
        *shared_set_size = 0;
    // }
}
void init_shared_memory_total_client(){
     int shmidTotClient;
    if ((shmidTotClient = shmget(SHM_KEY, sizeof(int), 0666 | IPC_CREAT)) < 0) {
        perror("shmget2");
          perror("shmat2");
        return;
    }
    total_clients= (int *)shmat(shmidTotClient, NULL, 0);
    *total_clients = 0;
}
void detach_shared_memory() {
    shmdt(shared_set);
    shmdt(total_clients);
}

#endif 
