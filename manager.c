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
int msg_manager_client_id;
int msg_client_manager_id;
int end_of_the_day;
int end_of_the_day_recently_triggered;
pid_t clients[CLIENTS_TOTAL][2]; // tablica z klientami w lokalu

void fire_handler(int sig);
void fire_handler_init();
int everyone_left();
void end_of_the_day_handler(int sig);
void end_of_the_day_handler_init();
void init_clients_tab();
void add_client(struct conversation *dialog);
void remove_client(struct conversation *dialog);
void allow_client_in(struct conversation dialog);
void reject_client(struct conversation dialog);
void client_leaves(struct conversation dialog);

int main()
{
    fire_handler_init();
    end_of_the_day_handler_init();

    int shm_id_tables = init_shm_tables();
    struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0);

    init_clients_tab(); // inicjalizacja tablicy - lokal jest pusty

    msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client, MANAGER WYSYLA
    msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager, MANAGER ODBIERA

    struct conversation dialog;

    end_of_the_day = 0;

    while (1)
    {
        dialog.pid = 0;
        dialog.topic = 0;
        dialog.individuals = 0;

        end_of_the_day_recently_triggered = 0;

        if (msgrcv(msg_client_manager_id, &dialog, conversation_size, 0, 0) == -1)
        { // odbieranie DOWOLNEGO KOMUNIKATU
            if (end_of_the_day_recently_triggered == 0)
            { // zabezpieczenie przed wartoscia -1 po odebraniu sygnalu END OF THE DAY (SIGUSR2)
                printf("manager: blad odbierania DOWOLNEGO KOMUNIKATU\n");
                exit(1);
            }
        }
        // printf("***MANAGER Odebral: pid=%ld topic=%d ind=%d\n", dialog.pid, dialog.topic, dialog.individuals); //debug only

        if (dialog.topic == CHCE_WEJSC)
        {
            allow_client_in(dialog);
        }

        if (dialog.topic == CHCE_WEJSC)
        {
            reject_client(dialog);
        }

        if (dialog.topic == DO_WIDZENIA)
        {
            client_leaves(dialog);
        }

        if (end_of_the_day == 1)
        { // przerwanie petli jesli jest koniec dnia i wszyscy wyszli
            if (everyone_left() == 1)
            {
                break;
            }
        }
    }

    printf("$$$MANAGER:\t Zamykam pizzerie. (prawidlowe wyjscie z petli)\n");
    return 0;
}

//===================FUNCJE=================================================================================================

void fire_handler(int sig)
{ // logika dzialania podczas pozaru
    printf("$$$Manager:\t ALARM ALARM, OGLASZAM POZAR\n");
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

int everyone_left() // sprawdzanie czy wszyscy wyszli. Lokal pusty:1 | są klienci: 0
{
    int empty = 1;
    for (int i = 0; i < CLIENTS_TOTAL; i++)
    {
        if (clients[i][0] != -1)
        {
            empty = 0;
            printf("$$$Manager:\t wiecej klientow nie bedzie ale pizzeria NIE JEST PUSTA. Czekam.\n");
            return empty;
        }
    }
    printf("$$$Manager:\t wiecej klientow nie bedzie i pizzeria JEST PUSTA. Mozna zamykac.\n");
    return empty;
}

void end_of_the_day_handler(int sig)
{
    end_of_the_day = 1;
    end_of_the_day_recently_triggered = 1;
}

void end_of_the_day_handler_init()
{
    struct sigaction sa;
    sa.sa_handler = end_of_the_day_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("blad ustawienia handlera sygnalu konca dnia\n");
        exit(1);
    }
}

void init_clients_tab()
{
    for (int i = 0; i < CLIENTS_TOTAL; i++) // wypelnianie tablicy klientow -1 (-1 to brak klienta w danym slocie)
    {
        clients[i][0] = -1;
    }
}

void add_client(struct conversation *dialog)
{
    for (int i = 0; i < CLIENTS_TOTAL; i++)
    { // dodowanie klienta do bazy
        if (clients[i][0] == -1)
        {
            clients[i][0] = dialog->pid;
            clients[i][1] = dialog->individuals;
            break;
        }
    }
}

void remove_client(struct conversation *dialog)
{
    for (int i = 0; i < CLIENTS_TOTAL; i++)
    {
        if (dialog->pid == clients[i][0])
        {
            clients[i][0] = -1;
            break;
        }
    }
}

void allow_client_in(struct conversation dialog)
{
    printf("$$$Manager:\t Mozesz wejsc. Dodaje %ld (%d osob) do listy klientow.\n", dialog.pid, dialog.individuals);
    dialog.topic = WEJDZ;
    if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1) //
    {
        printf("manager: blad wysylania komunikatu WEJDZ\n");
        exit(1);
    }
    add_client(&dialog); // dodanie klienta do tablicy CLIENTS
}

void reject_client(struct conversation dialog)
{
    printf("$$$Manager:\t Brak wolnych miejsc. Nie przyjmuje klienta  %ld (%d osob).\n", dialog.pid, dialog.individuals);
    dialog.topic = BRAK_MIEJSC;
    if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1)
    {
        printf("manager: blad wysylania komunikatu BRAK_MIEJSC\n");
        exit(1);
    }
}

void client_leaves(struct conversation dialog)
{
    printf("$$$Manager:\t Do widzenia. Usuwam %ld (%d osob) z listy klientow\n", dialog.pid, dialog.individuals);
    remove_client(&dialog);
}
