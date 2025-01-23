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
    ignore_end_of_the_day_init();          // ignoruje sygal KONIEC DNIA
    ignore_fire_handler_init();            // ignoruje sygnal POZAR
    int shm_id_tables = init_shm_tables(); // pozyskanie dostepu tablicy TABLES (stoliki)
    struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0);

    sleep(7);

    printf("!!!FF:\t UWAAAAGAAA! POOOOOOOZAR! ==============================================\n");
    kill(0, SIGUSR1);

    shmdt(tables_ptr);

    printf("FF: zakonczenie.\n");
    return 0;
}
