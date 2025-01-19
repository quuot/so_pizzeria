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

//#include <unistd.h>
//#include <time.h>


void fire_handler(int sig);
void fire_handler_init();

int main()
{
    fire_handler_init();
    printf("Manager: start managera\n");

    int shm_id_tables = init_shm_tables();
    struct table *tables_ptr = (struct table*)shmat(shm_id_tables, NULL, 0);

    sleep(10);

    return 0;
}

void fire_handler(int sig) { //logika dzialania podczas pozaru
    printf("Manager: ALARM ALARM, OGLASZAM POZAR\n");
}

void fire_handler_init() { //uruchamia obsluge sygnalu pozaru
    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGUSR1, &sa, NULL) == -1) { //obsluga bledu sygnalu pozaru
        perror("man: blad ustawienia handlera pozaru\n");
        exit(1);
    }
    
}