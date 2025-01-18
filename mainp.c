#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
//#include <unistd.h>
//#include <time.h>
#include "utils.h"


#define TABLE_ONE 4
#define TABLE_TWO 3
#define TABLE_THREE 2
#define TABLE_FOUR 1

void sigint_handler(int sig) //funkcja SIGINT - obsluga przerwania
{
   //msgctl(msgID,IPC_RMID,NULL); TODO:zamkniecie kolejki
   shmctl(shm_id_tables, IPC_RMID, NULL);
   //semctl(semID,0,IPC_RMID,NULL); TODO:zamkniecie semafora
   printf("mainp: SIGINT sygnal. Koniec programu.\n");
   exit(1);
}

int main()
{
    struct sigaction act; //obsluga sygnalu SIGINT
    act.sa_handler=sigint_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGINT,&act,0);


    switch (fork()) //tworzenie procesu managera
      {
         case -1:
            perror("mainp: Blad fork managera");
            exit(1);
         case 0:
            execl("./manager","manager", NULL);
      }

    switch (fork()) //tworzenie procesu firefighter
      {
         case -1:
            perror("mainp: Blad fork firefightera");
            exit(1);
         case 0:
            execl("./firefighter","firefighter", NULL);
      }

    int shm_id_tables;
    shm_id_tables = init_shm_tables();

    struct table tables[10];


    wait(NULL);//zakonczenie procesu strazak
    wait(NULL);//zakonczenie procesu manager
    shmctl(shm_id_tables, IPC_RMID, NULL);//usuwanie pamieci wspoldzielonej - tables

}
