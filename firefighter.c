#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "utils.h"

int main(int argc, char *argv[])
{
    ignore_end_of_the_day_init();          // ignoruje sygal KONIEC DNIA, strazak czuwa do konca
    ignore_fire_handler_init();            // ignoruje sygnal POZAR

    int seconds_untill_fire = atoi(argv[1]); //pobranie czasu do wywolania alarmu z listy argumentow

    if(seconds_untill_fire > 0) { //brak pozaru dla wartosci 0
        sleep(seconds_untill_fire); //czeka z ogloszeniem pozaru, czas podaje uzytkownik
        cprintf(colors[6], "============================ STRAZAK: SYGNAL POZARU ============================\n");
        kill(0, SIGUSR1); 
    }

    return 0;
}
