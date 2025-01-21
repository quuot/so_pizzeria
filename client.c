#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include "utils.h"

int msg_manager_client_id;
int msg_client_manager_id;

void fire_handler(int sig);
void fire_handler_init();
void ask_for_seat(struct conversation dialog);
void wait_for_seat(struct conversation *dialog);
void take_a_seat(struct conversation dialog);
void leave(struct conversation dialog);

int main(int argc, char *argv[])
{
    //printf("---Client  %d: Proces urchomiony\n", (int)getpid());
    fire_handler_init(); //inicjacja obslugi sygnalu 'POZAR'
    ignore_end_of_the_day_init(); //inicjacja obslugi sygnalu "KONIEC DNIA" (ignoruje sygnal)

    msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client, TU KLIENT ODBIERA
    msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager, TU KLIENT WYSYLA

    struct conversation dialog; // struktura komunikatu
    dialog.pid = getpid(); 
    dialog.topic = CHCE_WEJSC;
    dialog.individuals = atoi(argv[1]); // pobranie ilosci osob w tej grupie, atoi:string to int
    dialog.table_id = -1; //domyslnie: -1 = brak przypisanego stolika

    ask_for_seat(dialog); //wysyla komunikat CHCE_WEJSC do Managera

    wait_for_seat(&dialog); //oczekuje na zgode lub odmowe wejscia

    if (dialog.topic == WEJDZ) // Zgoda na wyjscie od Managera
    {
        take_a_seat(dialog);
        return 0;
    }

    if (dialog.topic == BRAK_MIEJSC)
    {
        printf("---Client %d: Nie zostalem wpuszczony. (zakonczenie procesu)\n", (int)getpid());
        return 0;
    }

    return 0;
}

void fire_handler(int sig)
{ // logika dzialania podczas pozaru
    printf("---Client: KLIENT %d ODEBRAL POZAR!!!! ZACZYNAM EWAKUACJE\n", (int)getpid());
    exit(0);
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

void ask_for_seat(struct conversation dialog)
{
        if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu CHCE WEJSC
    {
        printf("Client: blad wysylania komunikatu CHCE WEJSC\n");
        exit(1);
    }
    printf("---Client %d: Dzien dobry, chcialbym wejsc, stolik dla %d osob. (wysyla CHCE_WEJSC)\n", getpid(), dialog.individuals);
}

void wait_for_seat(struct conversation *dialog)
{
    if (msgrcv(msg_manager_client_id, dialog, conversation_size, getpid(), 0) == -1) //oczekuje na odebranie komunikatu o swoim PIDzie - zgoda na wyjscie
    { 
        printf("client: blad odbierania komunikatu\n");
        exit(1);
    }
}

void take_a_seat(struct conversation dialog)
{
        printf("---Client %d: Wszedlem, siadam do stolika %d, jem wspaniala pizze bez kechupu.\n", getpid(), dialog.table_id);
        sleep(2); //czas "siedzenia" w pizzerii
        dialog.topic = DO_WIDZENIA;

        leave(dialog);

        printf("---Client %d: Wychodze. Zwalniam stolik %d. (zakonczenie procesu)\n", (int)getpid(), dialog.table_id);
}

void leave(struct conversation dialog)
{
        if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu DO WIDZENIA
        {
            printf("Client: blad wysylania komunikatu DO WIDZENIA\n");
            exit(1);
        }
}