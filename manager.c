#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"

// #include <unistd.h>
// #include <time.h>

void fire_handler(int sig);
void fire_handler_init();

int main()
{
    fire_handler_init();
    printf("Manager: start managera\n");

    int shm_id_tables = init_shm_tables();
    struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0);

    pid_t clients[20]; //tablica z klientami w lokalu
    for(int i=0; i<0; i++){
        clients[i] = -1;
    }

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

    struct conversation dialog; 

    //tu bedzie pierwszy while - ZAPRASZANIE KLIENTOW
    dialog.pid = 0;
    dialog.topic = 0;

    if (msgrcv(msg_manager_client_id, &dialog, sizeof(dialog.topic), 0, 0) == -1) { //odbieranie DOWOLNEGO KOMUNIKATU
      printf("manager: blad odbierania komunikatu CHCE WEJSC\n");
      exit(1);
	} 

    if(dialog.topic == CHCE_WEJSC){
        printf("Manager: Zapraszam. Mamy wolne stoliki\n");
        dialog.topic = WEJDZ;
        if (msgsnd(msg_manager_client_id, &dialog, sizeof(dialog.topic), 0) == -1) //
	    {
            printf("manager: blad wysylania komunikatu WEJDZ\n");
            exit(1);
	    }
        for(int i=0; i<20; i++){ //dodowanie klienta do bazy
            if(clients[i] == -1){
                clients[i] = dialog.pid;
                break;
            }
        }
    } 
    else if(dialog.topic == DO_WIDZENIA){
        printf("Manager: Do widzenia, zapraszma ponownie\n");
        for(int i=0; i<20; i++){
            if(dialog.pid == clients[i]){
                clients[i] = -1;
                break;
            }
        }
    }


    return 0;
}


void fire_handler(int sig)
{ // logika dzialania podczas pozaru
    printf("Manager: ALARM ALARM, OGLASZAM POZAR\n");
}

void fire_handler_init()
{ // uruchamia obsluge sygnalu pozaru
    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    { // obsluga bledu sygnalu pozaru
        perror("blad ustawienia handlera pozaru\n");
        exit(1);
    }
}