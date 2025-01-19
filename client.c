#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include "utils.h"

void fire_handler(int sig);
void fire_handler_init();

int main()
{
    printf("client: start clienta, pid: %d\n", (int)getpid());
    fire_handler_init();

   key_t msg_manager_client_key = ftok(".", 'B');
   if ( msg_manager_client_key == -1 ) { 
      printf("manager: Blad ftok dla msg_manager_client_key\n"); 
      exit(1);
      }
   int msg_manager_client_id = msgget(msg_manager_client_key, IPC_CREAT | 0600); //todo zmiana uprawnien
   if (msg_manager_client_id == -1) {
      printf("manager: blad tworzenia kolejki komunikatow\n"); 
      exit(1);
   }

    struct conversation dialog; //przygotowanie komunikatu CHCE WEJSC
    dialog.pid = getpid();
    dialog.topic = CHCE_WEJSC;

    if (msgsnd(msg_manager_client_id, &dialog, sizeof(dialog.topic), 0) == -1) // wyslanie komunikatu CHCE WEJSC
	{
      printf("client: blad wysylania komunikatu CHCE WEJSC\n");
      exit(1);
	}
    printf("Chce wejsc\n");

    if (msgrcv(msg_manager_client_id, &dialog, sizeof(dialog.topic), getpid(), 0) == -1) { //odbieranie komunikatu WEJDZ
      printf("client: blad odbierania komunikatu\n");
      exit(1);
	} 
   
    if(dialog.topic == WEJDZ){
        printf("Wchodze\n");
        sleep(5);
        dialog.topic = DO_WIDZENIA;
        if (msgsnd(msg_manager_client_id, &dialog, sizeof(dialog.topic), 0) == -1) // wyslanie komunikatu DO WIDZENIA
	    {
            printf("client: blad wysylania komunikatu DO WIDZENIA\n");
            exit(1);
	    } 
        printf("Do widzenia, wyszedlem. \nProces CLIENT PID=%d zakonczyl dzialanie.\n", (int)getpid());
        return 0;
    }

    printf("Nie wchodze\n");
    return 0;
}

void fire_handler(int sig)
{ // logika dzialania podczas pozaru
    printf("KLIENT %d ODEBRAL POZAR!!!! ZACZYNAM EWAKUACJE\n", (int)getpid());
}

void fire_handler_init()
{ // uruchamia obsluge sygnalu pozaru
    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    { // obsluga bledu sygnalu pozaru
        perror("client: blad ustawienia handlera pozaru\n");
        exit(1);
    }
}