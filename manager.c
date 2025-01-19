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
int everyone_left(pid_t clients[20][2]);

int main()
{
    fire_handler_init();
    printf("Manager: start managera\n");

    int shm_id_tables = init_shm_tables();
    struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0);

    pid_t clients[20][2]; // tablica z klientami w lokalu
    for (int i = 0; i < 20; i++)
    {
        clients[i][0] = -1;
    }

    int msg_manager_client_id = init_msg_manager_client();

    struct conversation dialog;

    int end_of_the_day = 0;
    while (1)
    {
        dialog.pid = 0;
        dialog.topic = 0;
        dialog.individuals = 0;

        if (msgrcv(msg_manager_client_id, &dialog, conversation_size, 0, 0) == -1)
        { // odbieranie DOWOLNEGO KOMUNIKATU
            printf("manager: blad odbierania DOWOLNEGO KOMUNIKATU\n");
            exit(1);
        }
        printf("MANAGER Odebral: pid=%ld topic=%d ind=%d\n", dialog.pid, dialog.topic, dialog.individuals);

        if (dialog.topic == KONIEC_DNIA)
        { // odebranie komunikatu o koncu dnia
            end_of_the_day = 1;
        }

        if (end_of_the_day == 1)
        { // przerwanie petli jesli wszyscy wyszli
            if (everyone_left(clients) == 1)
            {
                break;
            }
        }

        if (dialog.topic == CHCE_WEJSC)
        {
            printf("Manager: Zapraszam. Mamy wolne stoliki\n");
            dialog.topic = WEJDZ;
            if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1) //
            {
                printf("manager: blad wysylania komunikatu WEJDZ\n");
                exit(1);
            }
            for (int i = 0; i < 20; i++)
            { // dodowanie klienta do bazy
                if (clients[i][0] == -1)
                {
                    clients[i][0] = dialog.pid;
                    clients[i][1] = dialog.individuals;
                    break;
                }
            }
        }
        else if (dialog.topic == DO_WIDZENIA)
        {
            printf("Manager: Do widzenia, zapraszma ponownie\n");
            for (int i = 0; i < 20; i++)
            {
                if (dialog.pid == clients[i][0])
                {
                    clients[i][0] = -1;
                    break;
                }
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

int everyone_left(pid_t clients[20][2]) // sprawdzanie czy wszyscy wyszli. Lokal pusty:1 | są klienci: 0
{
    int empty = 1;
    for (int i = 0; i < 20; i++)
    {
        if (clients[i][0] != -1)
        {
            empty = 0;
            break;
        }
    }
    return empty;
}