#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include "utils.h"
#include <time.h>
#include <errno.h>

int msg_manager_client_id; // id kolejki manager-client
int msg_client_manager_id; // id kolejki client-manager
int table_id = -1;         // domyślnie -1=brak przypisanego stolika
int individuals;           // ilosc osob w grupie
int inside = 0;

void fire_handler(int sig);
void fire_handler_init();
void ask_for_seat(struct conversation dialog);
void wait_for_seat(struct conversation *dialog);
void take_a_seat(struct conversation dialog);
void leave(struct conversation dialog);
char *random_color();

int main(int argc, char *argv[])
{
    fire_handler_init();          // inicjacja obslugi sygnalu 'POZAR'
    ignore_end_of_the_day_init(); // inicjacja obslugi sygnalu "KONIEC DNIA" (ignoruje sygnal)

    msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client
    msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager

    individuals = atoi(argv[1]); // ilosc osob w grupie, atoi:string to int

    struct conversation dialog; // struktura komunikatu
    dialog.pid = getpid();
    dialog.individuals = individuals;
    dialog.table_id = -1; // domyslnie: -1 = brak przypisanego stolika

    ask_for_seat(dialog); // wysyla komunikat CHCE_WEJSC do Managera

    wait_for_seat(&dialog); // oczekuje na zgode lub odmowe wejscia

    if (dialog.topic == WEJDZ) // Zgoda na wyjscie od Managera
    {
        inside = 1;
        take_a_seat(dialog); // zajecie miejsca, symulacja czasu jedzenia,  wyslanie komunikatu "DO WIDZENIA"
        return 0;
    }

    if (dialog.topic == BRAK_MIEJSC) // odmowa wejscia
    {
        printf("\033[36mClient %d: Nie zostalem wpuszczony. Nie czekam dluzej. [zakonczenie procesu] \x1b[0m\n", (int)getpid());
        return 0;
    }

    return 0;
}

/// Logika dzialania podczas pozaru: wydruk o otrzymaniu pozaru, przeslanie komunikatu o wyjsciu z lokalu
void fire_handler(int sig)
{
    if (inside == 0)
    {
        exit(0);
    }

    if (msg_client_manager_id == 0)
    {
        msg_client_manager_id = init_msg_client_manager();
    }

    printf("\033[36mClient: KLIENT %d ODEBRAL POZAR! ZACZYNAM EWAKUACJE! \x1b[0m\n", getpid());
    struct conversation dialog;
    dialog.pid = getpid();
    dialog.topic = EWAKUACJA;
    dialog.table_id = table_id;
    dialog.individuals = individuals;
    leave(dialog);

    exit(0); // prawidlowe zakonczenie w przypadku pozaru
}

/// Inicjacja obslugi sygnalu pozaru - SIGUSR1,
void fire_handler_init()
{
    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("Client: fire_handler_init(): blad ustawienia handlera pozaru\n");
        exit(errno);
    }
}

/// Wyslanie komunikatu "CHCE WEJSC", wystwietlenie powitania
void ask_for_seat(struct conversation dialog)
{

    dialog.topic = CHCE_WEJSC;
    if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1) // wyslanie komunikatu CHCE WEJSC
    {
        perror("Client: ask_for_seat(): blad wysylania komunikatu CHCE WEJSC\n");
        exit(errno);
    }
    printf("\033[36mClient %d: Dzien dobry, chcialbym wejsc, stolik dla %d osob. \x1b[0m\n", getpid(), dialog.individuals);
}

/// Oczekiwanie na odpowiedz w kolejce komunikatow przeznaczona dla tego procesu (getpid)
void wait_for_seat(struct conversation *dialog)
{
    if (msgrcv(msg_manager_client_id, dialog, conversation_size, getpid(), 0) == -1) // oczekuje na odebranie komunikatu o swoim PIDzie
    {
        perror("Client: wait_for_seat(): blad odbierania komunikatu\n");
        exit(errno);
    }
}

/// wyswietla informacje o wejsciu do pizzeri, symuluje 1-9s sekund pozostania w lokalu,
void take_a_seat(struct conversation dialog)
{
    int idle_time = getpid() % 9 + 1; // wybranie czasu ktory klient spedzi przy stoliku
    printf("\033[36mClient %d: Wchodze, siadam do stolika nr %d, jem wspaniala pizze bez kechupu. Pozostane tu %d sekund.\x1b[0m\n", getpid(), dialog.table_id, idle_time);
    table_id = dialog.table_id; // przypisanie do zmiennej globalnej

    sleep(idle_time); // symulowanie czasu spedzonego w pizzeri

    dialog.topic = DO_WIDZENIA; // przygotowanie komunikatu o wyjsciu
    leave(dialog);              // przeslanie komunikatu o wyjsciu

    printf("\033[36mClient %d: Wychodze. Zwalniam stolik %d. [zakonczenie procesu]\x1b[0m\n", (int)getpid(), dialog.table_id);
}

/// Funkcja wyjscia z lokali - wysyla komunikat "Do widzenia" do managera.
void leave(struct conversation dialog)
{
    if (msg_client_manager_id != 0)
    {
        if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1)
        {
            perror("Client: leave(): blad wysylania komunikatu DO WIDZENIA\n");
            exit(errno);
        }
    }
}
