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
    printf("---Client  %d: Proces urchomiony\n", (int)getpid());
    fire_handler_init();
    ignore_end_of_the_day_init();

    int msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client, TU KLIENT ODBIERA
    int msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager, TU KLIENT WYSYLA

    struct conversation dialog; // przygotowanie komunikatu
    dialog.pid = getpid();
    dialog.topic = CHCE_WEJSC;
    dialog.individuals = atoi(argv[1]); // pobranie ilosci osob w tej grupie, atoi:string to int
    dialog.table_id = -1;

    if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu CHCE WEJSC
    {
        printf("Client: blad wysylania komunikatu CHCE WEJSC\n");
        exit(1);
    }
    printf("---Client %d: Dzien dobry, chcialbym wejsc, stolik dla osob: %d. (wysyla CHCE_WEJSC)\n", getpid(), dialog.individuals);

    if (msgrcv(msg_manager_client_id, &dialog, conversation_size, getpid(), 0) == -1)
    { // odbieranie komunikatu WEJDZ LUB BRAK_MIEJSC
        printf("client: blad odbierania komunikatu\n");
        exit(1);
    }

    // printf("CLIENT Odebral: pid=%ld topic=%d ind=%d\n", dialog.pid, dialog.topic, dialog.individuals); //debug only

    if (dialog.topic == WEJDZ)
    {
        printf("---Client %d: Wszedlem, siadam do stolika %d, jem wspaniala pizze bez kechupu.\n", getpid(), dialog.table_id);
        sleep(2);
        dialog.topic = DO_WIDZENIA;
        if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu DO WIDZENIA
        {
            printf("Client: blad wysylania komunikatu DO WIDZENIA\n");
            exit(1);
        }
        printf("---Client %d: Wychodze. Zwalniam stolik %d. Koncze proces.\n", (int)getpid(), dialog.table_id);
        return 0;
    }

    if (dialog.topic == BRAK_MIEJSC)
    {
        printf("---Client %d: Nie zostalem wpuszczony. Koncze proces.\n", (int)getpid());
        return 0;
    }

    return 0;
}

void fire_handler(int sig)
{ // logika dzialania podczas pozaru
    printf("---Client: KLIENT %d ODEBRAL POZAR!!!! ZACZYNAM EWAKUACJE\n", (int)getpid());
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