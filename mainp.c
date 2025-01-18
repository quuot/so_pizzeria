#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
// #include <time.h>
#include "utils.h"

int shm_id_tables;

#define TABLE_ONE 4
#define TABLE_TWO 3
#define TABLE_THREE 2
#define TABLE_FOUR 1

void sigint_handler(int sig) // funkcja SIGINT - obsluga przerwania
{
   // msgctl(msgID,IPC_RMID,NULL); TODO:zamkniecie kolejki
   shmctl(shm_id_tables, IPC_RMID, NULL);
   // semctl(semID,0,IPC_RMID,NULL); TODO:zamkniecie semafora
   printf("mainp: SIGINT sygnal. Koniec programu.\n");
   exit(1);
}

int main()
{
   struct sigaction act; // obsluga sygnalu SIGINT
   act.sa_handler = sigint_handler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT, &act, 0);

   struct table tables[10]; // inicjalizacja tablicy tables
   for (int i = 0; i < TABLE_ONE; i++)
   { // inicjalizacja stolikow 1osobowych
      tables[i].table_id = i;
      tables[i].capacity = 1;
      tables[i].free = 1;
      tables[i].group_size = 0;
   }
   for (int i = TABLE_ONE; i < (TABLE_ONE + TABLE_TWO); i++)
   { // inicjalizacja stolikow 2osobowych
      tables[i].table_id = i;
      tables[i].capacity = 2;
      tables[i].free = 2;
      tables[i].group_size = 0;
   }
   for (int i = (TABLE_ONE + TABLE_TWO); i < (TABLE_ONE + TABLE_TWO + TABLE_THREE); i++)
   { // inicjalizacja stolikow 3osobowych
      tables[i].table_id = i;
      tables[i].capacity = 3;
      tables[i].free = 3;
      tables[i].group_size = 0;
   }
   for (int i = (TABLE_ONE + TABLE_TWO + TABLE_THREE); i < (TABLE_ONE + TABLE_TWO + TABLE_THREE + TABLE_FOUR); i++)
   { // inicjalizacja stolikow 4osobowych
      tables[i].table_id = i;
      tables[i].capacity = 4;
      tables[i].free = 4;
      tables[i].group_size = 0;
   }

   shm_id_tables = init_shm_tables(); // inicjalizacja pamieci wspoldzielonej: tables
   struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0);

   switch (fork()) // tworzenie procesu managera
   {
   case -1:
      perror("mainp: Blad fork managera");
      exit(1);
   case 0:
      execl("./manager", "manager", NULL);
   }

   switch (fork()) // tworzenie procesu firefighter
   {
   case -1:
      perror("mainp: Blad fork firefightera");
      exit(1);
   case 0:
      execl("./firefighter", "firefighter", NULL);
   }

   wait(NULL);                            // zakonczenie procesu strazak
   wait(NULL);                            // zakonczenie procesu manager
   shmctl(shm_id_tables, IPC_RMID, NULL); // usuwanie pamieci wspoldzielonej - tables
}
