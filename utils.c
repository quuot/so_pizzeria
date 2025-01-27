#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "utils.h"
#include <stdarg.h> //dla va_list (kolory)
#include <errno.h>

//Przydzial kluczy (ftok): 
// A-shm tables, 
// B-msg manager client, 
// C-msg clienta manager, 
// D-shm word

   const char *colors[] = { //tablica z kodami kolorow
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
    /// Tworzenie/uzyskiwanie dostepu do pamieci wspoldzielonej dla tablicy struktur TABLES (stoliki). 
    /// Tablica zawiera rozklad stolikow (id stolika, ile miejsc ogolem, ile miejsc wolnych, jak liczna grupa zajmuje stolik).
    /// Przejmuje wskaznik do struktury world (wykorzystuje dane inicjalizacyjne wprowadzone przez uzytkownika). Zwraca ID pamieci.

    int shm_id_tables_key = ftok(".", 'A');
    if (shm_id_tables_key == -1)
    {
        perror("Utils: init_shm_tables: Blad tworzenia klucza (ftok, shm_id_tables_key).");
        exit(errno);
    }

    int shm_id_tables = shmget(shm_id_tables_key, world_ptr->tables_total * sizeof(struct table), IPC_CREAT | 0600);
    if (shm_id_tables == -1)
    {
        perror("Utils: init_shm_tables: blad pamieci dzielonej (shmget, shm_id_tables).");
        exit(errno);
    }

    return shm_id_tables;
}

int init_msg_manager_client()
{
    /// uzyskiwanie dostepu do kolejki MANAGER-->CLIENT. Zwraca ID kolejki.
    /// Sluzy do jednokierunkowej komunikacji miedzy managerem a clientem. Manager tylko wysyla, klient tylko odbiera.

    int msg_manager_client_key = ftok(".", 'B');
    if (msg_manager_client_key == -1)
    {
        perror("Utils: init_msg_manager_client: Blad tworzenia klucza (ftok, msg_manager_client_key).");
        exit(errno);
    }

    int msg_manager_client_id = msgget(msg_manager_client_key, IPC_CREAT | 0600);
    if (msg_manager_client_id == -1)
    {
        perror("Utils: blad tworzenia kolejki komunikatow (msgget, msg_manager_client_id).");
        exit(errno);
    }

    return msg_manager_client_id;
}

int init_msg_client_manager()
{
    /// uzyskiwanie dostepu do kolejki CLIENT-->MANAGER. Zwraca ID kolejki.
    /// Sluzy do jednokierunkowej komunikacji miedzy clientem a managerem. Klient tylko wysyla, manager tylko odbiera.

    int msg_client_manager_key = ftok(".", 'C');
    if (msg_client_manager_key == -1)
    {
        perror("Utils: init_msg_client_manager: Blad tworzenia klucza (ftok, msg_client_manager_key).");
        exit(errno);
    }

    int msg_client_manager_id = msgget(msg_client_manager_key, IPC_CREAT | 0600);
    if (msg_client_manager_id == -1)
    {
        perror("Utils: blad tworzenia kolejki komunikatow (msgget, msg_client_manager_id).");
        exit(errno);
    }

    return msg_client_manager_id;
}

void ignore_fire_handler_init()
{ 
    /// Uruchomienie obslugi sygnalu pozaru SIGUSR1 - Ignoruje sygnal
    /// wymagany do procesow, ktore nie zmieniaja swojej pracy pomimo ogloszenia alarmu (np. mainp, firefighter)

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;  //Ignore
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    { 
        perror("Utils: ignore_fire_handler_init: blad ustawienia handlera pozaru (sigaction, SIGUSR1)"); 
        exit(errno);
    }
}


void ignore_end_of_the_day_init()
{ 
    /// Uruchomienie obslugi sygnalu konca dnia SIGUSR2- Ignoruje sygnal
    /// wymagany do procesow, ktore nie zmieniaja swojej pracy pomimo ogloszenia konca dnia (np. firefighter)

    struct sigaction sa;
    sa.sa_handler = SIG_IGN; //Ignore
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("Utils: ignore_end_of_the_day_init: blad ustawienia handlera konca dnia (sigaction, SIGUSR2)");
        exit(errno);
    }
}

int init_shm_world()
{
    /// Tworzenie/uzyskiwanie dostepu do pamieci wspoldzielonej dla struktury WORLD (dane inicjacyjne). Zwraca ID pamieci. 
    /// Umozliwia dostep do danych inicjalizacyjnych "swiata" w roznych procesach.

    int shm_id_world_key = ftok(".", 'D');
    if (shm_id_world_key == -1)
    {
        perror("Utils: init_shm_world: Blad tworzenia klucza (ftok, shm_id_world_key).");
        exit(errno);
    }

    int shm_id_world = shmget(shm_id_world_key, sizeof(struct world), IPC_CREAT | 0600);
    if (shm_id_world == -1)
    {
        perror("Utils: init_shm_world: blad pamieci dzielonej (shmget, shm_id_world).");
        exit(errno);
    }

    return shm_id_world;
}

void cprintf(const char *color, const char *format, ...) 
{
    /// Wlasna funkcja drukujaca w terminalu przy uzyciu kolorow
    /// 1 parametr: kod koloru (z tabeli colors[]), kolejne parametry analogicznie do printf

    va_list args;
    va_start(args, format);
    printf("%s", color);       //ustawienie koloru 
    vprintf(format, args);     // print listy argumentow va_list
    printf("\033[0m");         // powrót do domyslnego
    va_end(args);
}

void detach_mem_tables_world(struct table* tables_ptr, struct world* world_ptr)
{
    /// Odlaczenie pamieci wspoldzielonej TABLES oraz WORLD.
    /// Do uzycia w plikach gdzie stosowano obie funkcje: init_shm_world oraz init_shm_tables
    /// Przyjmuje wskazniki do odczepianej pamieci jako parametry

    if (shmdt(tables_ptr) == -1)
    {
        perror("Utils: main(): Blad odlaczania pamieci wspoldzielonej (shmdt, tables_ptr).");
        exit(errno);
    }
    if (shmdt(world_ptr) == -1)
    {
        perror("Utils: main(): Blad odlaczania pamieci wspoldzielonej (shmdt, world_ptr).");
        exit(errno);
    }
}