#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"
//#include <unistd.h>
//#include <time.h>

int main()
{
    printf("ff: start managera\n");

    int shm_id_tables_key;
    shm_id_tables_key = ftok(".",'A');
    shm_id_tables=shmget(shm_id_tables_key, 10*3*sizeof(int), IPC_CREAT|IPC_EXCL|0600);
    if (shmID==-1){
        printf("mainp: blad pamieci dzielonej\n"); 
        exit(1);
    }


}