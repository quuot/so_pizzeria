#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include "utils.h"
#include <stdarg.h> //dla va_list (kolory)

//ftok: A-shm tables, B-msg manager client, C-msg clienta manager, D-shm word

   const char *colors[] = {
    "\033[0m",      // 0-Reset
    "\033[33m",     // 1-zolty
    "\033[34m",     // 2-niebieski
    "\033[35m",     // 3-Fioletowy
    "\033[36m",     // 4-Cyjan
    "\033[32m",     // 5-zielony TYLKO DLA MANAGERA
    "\033[31m",     // 6-czerwony TYLKO DLA STRAZAKA
};


int init_shm_tables(struct world *world_ptr)
{
    int shm_id_tables_key;
    shm_id_tables_key = ftok(".", 'A');

    int shm_id_tables = shmget(shm_id_tables_key, world_ptr->tables_total * sizeof(struct table), IPC_CREAT | 0600);

    if (shm_id_tables == -1)
    {
        perror("utils: blad pamieci dzielonej dla TABLES\n");
        exit(1);
    }
    return shm_id_tables;
}

int init_msg_manager_client()
{
    key_t msg_manager_client_key = ftok(".", 'B');
    if (msg_manager_client_key == -1)
    {
        perror("utils: Blad ftok dla msg_manager_client_key.");
        exit(1);
    }
    int msg_manager_client_id = msgget(msg_manager_client_key, IPC_CREAT | 0600);
    if (msg_manager_client_id == -1)
    {
        perror("utils: blad tworzenia kolejki komunikatow msg_manager_client.");
        exit(1);
    }
    return msg_manager_client_id;
}

int init_msg_client_manager()
{
    key_t msg_client_manager_key = ftok(".", 'C');
    if (msg_client_manager_key == -1)
    {
        perror("utils: Blad ftok dla msg_client_manager_key.");
        exit(1);
    }
    int msg_client_manager_id = msgget(msg_client_manager_key, IPC_CREAT | 0600);
    if (msg_client_manager_id == -1)
    {
        perror("utils: blad tworzenia kolejki komunikatow msg_client_manager_id.");
        exit(1);
    }
    return msg_client_manager_id;
}

void ignore_fire_handler_init()
{ // uruchamia obsluge sygnalu pozaru
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // ignoruje sygnal
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    { // obsluga bledu sygnalu pozaru
        perror("utils: blad ustawienia handlera pozaru");
        exit(1);
    }
}

void ignore_end_of_the_day_init()
{ // ignoruje sygnal end of the day
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // ignoruje sygnal
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("utils: blad ustawienia ignore end of the day.");
        exit(1);
    }
}

int init_shm_world()
{
    int shm_id_world_key;
    shm_id_world_key = ftok(".", 'D');

    int shm_id_world = shmget(shm_id_world_key, sizeof(struct world), IPC_CREAT | 0600);

    if (shm_id_world == -1)
    {
        perror("utils: blad pamieci dzielonej dla WORLD.");
        exit(1);
    }
    return shm_id_world;
}

void cprintf(const char *color, const char *format, ...) 
{
    va_list args;
    va_start(args, format);
    printf("%s", color);       //ustawienie koloru 
    vprintf(format, args);     // print listy argumentow va_list
    printf("\033[0m");         // powrót do domyslnego
    va_end(args);
}