#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"

//ftok A - tables
//ftok B - pid procesow

int init_shm_tables()
{
    int shm_id_tables_key;
    shm_id_tables_key = ftok(".",'A');

    int shm_id_tables = shmget(shm_id_tables_key, TABLES_TOTAL * sizeof(struct table), IPC_CREAT|0600);

    if (shm_id_tables==-1){
        perror("utils: blad pamieci dzielonej dla TABLES\n"); 
        exit(1);
    }
    return shm_id_tables;
}

void print_tables(struct table *tables_ptr, int tables_total)
{
    for(int i=0; i<tables_total; i++){
        printf("ID=%ld\ncapacity=%d\nfree=%d\ngroup size=%d\n\n", tables_ptr[i].id, tables_ptr[i].capacity, tables_ptr[i].free, tables_ptr[i].group_size);
    }
}
