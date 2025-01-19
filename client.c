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

int main(int argc, char *argv[])
{
    printf("client: start clienta, pid: %d\n", (int)getpid());
    fire_handler_init();
    int msg_manager_client_id = init_msg_manager_client();

    struct conversation dialog; //przygotowanie komunikatu CHCE WEJSC
    dialog.pid = getpid();
    dialog.topic = CHCE_WEJSC;
    dialog.individuals = atoi(argv[1]);

    if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu CHCE WEJSC
	{
      printf("client: blad wysylania komunikatu CHCE WEJSC\n");
      exit(1);
	}
    printf("Chce/my wejsc. Jest nas %d.\n", dialog.individuals);

    sleep(1);

    if (msgrcv(msg_manager_client_id, &dialog, conversation_size, getpid(), 0) == -1) { //odbieranie komunikatu WEJDZ
      printf("client: blad odbierania komunikatu\n");
      exit(1);
	} 

    printf("CLIENT Odebral: pid=%ld topic=%d ind=%d\n", dialog.pid, dialog.topic, dialog.individuals);
    
    if(dialog.topic == WEJDZ){
        printf("Wchodzimy.\n");
        sleep(5);
        dialog.topic = DO_WIDZENIA;
        if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu DO WIDZENIA
	    {
            printf("client: blad wysylania komunikatu DO WIDZENIA\n");
            exit(1);
	    } 
        printf("Do widzenia, wychodzimy. \nProces CLIENT PID=%d zakonczyl dzialanie.\n", (int)getpid());
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