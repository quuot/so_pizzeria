#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"

int init_shm_tables()
{
    int shm_id_tables;
    int shm_id_tables_key;
    shm_id_tables_key = ftok(".",'A');
    shm_id_tables=shmget(shm_id_tables_key, 10*3*sizeof(int), IPC_CREAT|IPC_EXCL|0600);
    if (shmID==-1){
        printf("utils: blad pamieci dzielonej\n"); 
        exit(1);
    }
    return shm_id_tables;
}