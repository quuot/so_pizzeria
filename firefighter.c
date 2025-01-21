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

int main()
{
    ignore_end_of_the_day_init();
    int shm_id_tables = init_shm_tables(); // pozyskanie tablicy TABLES
    struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0);

    sleep(5);
    // printf("@@@FF:\t UWAAAAGAAA! POOOOOOOZAR!\n");
    // kill(0, SIGUSR1);

    shmdt(tables_ptr);
    return 0;
}
