#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"

// ftok A - tables


int init_shm_tables()
{
    int shm_id_tables_key;
    shm_id_tables_key = ftok(".", 'A');

    int shm_id_tables = shmget(shm_id_tables_key, TABLES_TOTAL * sizeof(struct table), IPC_CREAT | 0600);

    if (shm_id_tables == -1)
    {
        perror("utils: blad pamieci dzielonej dla TABLES\n");
        exit(1);
    }
    return shm_id_tables;
}

void print_tables(struct table *tables_ptr, int tables_total)
{
    for (int i = 0; i < tables_total; i++)
    {
        printf("ID=%ld\ncapacity=%d\nfree=%d\ngroup size=%d\n\n", tables_ptr[i].id, tables_ptr[i].capacity, tables_ptr[i].free, tables_ptr[i].group_size);
    }
}

int init_msg_manager_client()
{
    key_t msg_manager_client_key = ftok(".", 'B');
   if ( msg_manager_client_key == -1 ) { 
      perror("utils: Blad ftok dla msg_manager_client_key\n"); 
      exit(1);
      }
   int msg_manager_client_id = msgget(msg_manager_client_key, IPC_CREAT|0600); 
   if (msg_manager_client_id == -1) {
      perror("utils: blad tworzenia kolejki komunikatow msg_manager_client\n"); 
      exit(1);
   }
   return msg_manager_client_id;
}

int init_msg_client_manager()
{
    key_t msg_client_manager_key = ftok(".", 'C');
   if ( msg_client_manager_key == -1 ) { 
      perror("utils: Blad ftok dla msg_client_manager_key\n"); 
      exit(1);
      }
   int msg_client_manager_id = msgget(msg_client_manager_key, IPC_CREAT|0600); 
   if (msg_client_manager_id == -1) {
      perror("utils: blad tworzenia kolejki komunikatow msg_client_manager_id\n"); 
      exit(1);
   }
   return msg_client_manager_id;
}

void ignore_fire_handler_init()
{ // uruchamia obsluge sygnalu pozaru
   struct sigaction sa;
   sa.sa_handler = SIG_IGN; // ignoruje sygnal
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGUSR1, &sa, NULL) == -1)
   { // obsluga bledu sygnalu pozaru
      perror("utils: blad ustawienia handlera pozaru\n");
      exit(1);
   }
}

void ignore_end_of_the_day_init()
{ //ignoruje sygnal end of the day
   struct sigaction sa;
   sa.sa_handler = SIG_IGN; // ignoruje sygnal
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGUSR2, &sa, NULL) == -1)
   { 
      perror("utils: blad ustawienia ignore end of the day\n");
      exit(1);
   }
}