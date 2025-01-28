#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>
#include "utils.h"
#include <errno.h>

int msg_manager_client_id;             // ID kolejki MANAGER-->CLIENT
int msg_client_manager_id;             // ID kolejki CLIENT-->MANAGER
int end_of_the_day;                    // flaga konca dnia, ustawiana na 1 po sygnale SIGUSR2
int end_of_the_day_recently_triggered; // flaga; wartosc 1 oznacza ze sygnal SIGUSR2(koniec dnia) wplynal w biezacym przejsciu petli while
int fire_alarm_triggered = 0;          // flaga pozaru, ustawiana na 1 po sygnale SIGUSR1
int fire_alarm_recently_triggered;     // flaga; wartosc 1 oznacza ze sygnal SIGUSR1(pozar) wplynal w biezacym przejsciu petli while
pid_t clients[NOTEBOOK_LENGTH][2];     // tablica z klientami w lokalu (notatnik managera)
struct table *tables_ptr;              // wskaznik do tablicy struktur z ukladem stolikow w lokalu
struct world *world_ptr;               // wskaznik do struktury z danymi poczatkowymi (ilosc i rodzaj stolikowm, czas do alarmu)

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
void grant_seat(struct conversation dialog);
void walk_to_the_door(struct conversation dialog);
void wait_for_interaction(struct conversation *dialog);
void sigint_handler(int sig);
void sigint_hadler_init();
void exit_handler(int error_number);

int main()
{
    sigint_hadler_init();
    fire_handler_init();           // inicjacja obslugi sygnalu 'POZAR', manager zaczyna sprawdzanie czy klienci wychodza z lokalu
    end_of_the_day_handler_init(); // inicjacja obslugi sygnalu "KONIEC DNIA". Po sygnale nie bedzie juz wiecej klientow

    int shm_id_world = init_shm_world();                      // uzyskanie dostepu do pamieci wspoldzielonej: dane poczatkowe "swiata"
    world_ptr = (struct world *)shmat(shm_id_world, NULL, 0); // pobranie wskaznika do struktury "świat"

    int shm_id_tables = init_shm_tables(world_ptr);             // uzyskanie dostepu do pamieci wspoldzielonej: uklad stolikow
    tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0); // pobranie wskaznika do tablicy z ukladem stolikow w lokalu

    init_clients_tab(); // inicjalizacja tablicy lokalnej "notes managera": wartosc poczatkowa=-1 (stoliki puste)

    msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client, MANAGER WYSYLA
    msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager, MANAGER ODBIERA

    struct conversation dialog; // stworzenie struktury do wymiany komunikatow z klientami

    end_of_the_day = 0; // flaga konca dnia

    while (1)
    {
        // zerowanie struktury komunikatu
        dialog.pid = 0;
        dialog.topic = 0;
        dialog.individuals = 0;

        // resetowanie flag zabezpieczajacych przez bledem msgrcv. Patrz: komentarz w wait_for_interaction().
        end_of_the_day_recently_triggered = 0;
        fire_alarm_recently_triggered = 0;

        // oczekuje na komunikat od klienta
        wait_for_interaction(&dialog);

        if (dialog.topic == CHCE_WEJSC && fire_alarm_triggered == 0)
        {
            // komunikat "chce wejsc" oznacza, że klient przyszedł do lokalu i i oczekuje na stolik
            //  manager szuka wolnego stolika zgodnie z algorytmem
            //  Jesli znajdzie stolik to wysyla komunikat "wejdz" i dopisuje klienta do listy
            //  Jeśli nie ma wolnego stolika to wysyla komunikat "brak miejsc".
            grant_seat(dialog);
        }

        if (dialog.topic == DO_WIDZENIA || dialog.topic == EWAKUACJA)
        {
            // komunikat "do widzenia" oznacza, że klient wstal od stolika i opuszcza lokal
            // manager aktualizuje tabelę klientów w lokalu (odnotowuje zwolnienie stolika).
            walk_to_the_door(dialog);
        }

        if (end_of_the_day == 1 || fire_alarm_triggered == 1)
        {
            // Jeżeli jest aktywowana flaga "koniec dnia" to manager sprawdza czy lokal jest pusty
            //  funkcja everypne_left zwraca 1 jeśli lokal jest pusty, co przerywa glowna petla while u managera
            if (everyone_left() == 1)
            {
                break;
            }
        }
    }

    // odlaczenie pamieci wspoldzielonej TABLES oraz WORLD
    detach_mem_tables_world(tables_ptr, world_ptr);

    printf("\033[32mManager:\t Zamykam pizzerie. (prawidlowe wyjscie z petli)\x1b[0m\n");
    kill(getppid(), SIGINT);
    return 0;
}

void fire_handler(int sig)
{
    /// logika dzialania podczas pozaru
    /// ustawienie flag sterujacych

    fire_alarm_triggered = 1;
    fire_alarm_recently_triggered = 1;
}

void fire_handler_init()
{
    /// uruchamia obsluge sygnalu pozaru - SIGUSR1

    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("Manager: fire_handler_init: blad ustawienia handlera pozaru (sigaction, SIGUSR1)");
        exit_handler(errno);
    }
}

int everyone_left()
{
    /// sprawdzanie czy wszyscy wyszli. Lokal pusty zwraca: 1 | Są klienci zwraca: 0
    /// wymaga zmiennej globalnej world_ptr oraz clients[][]

    int empty = 1;
    for (int i = 0; i < world_ptr->clients_total; i++)
    {
        if (clients[i][0] != -1)
        {
            empty = 0;
            printf("\033[32mManager:\t Czekam na wyjscie klientow.\x1b[0m\n");
            return empty;
        }
    }

    printf("\033[32mManager:\t WSZYSCY WYSZLI. Zamykam kase.\x1b[0m\n");

    return empty;
}

void end_of_the_day_handler(int sig)
{
    /// logika dzialania podczas sygnalu "koniec dnia"
    /// ustawienie flag sterujacych
    end_of_the_day = 1;
    end_of_the_day_recently_triggered = 1;
}

void end_of_the_day_handler_init()
{
    /// uruchamia obsluge sygnalu pozaru - SIGUSR2

    struct sigaction sa;
    sa.sa_handler = end_of_the_day_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        perror("Manager: end_of_the_day_handler_init: blad ustawienia handlera sygnalu konca dnia(sigaction,SIGUSR2)");
        exit_handler(errno);
    }
}

void init_clients_tab()
{
    /// wyplenienie tablicy lokalnej - notes managera
    /// kolumna [0]: pidy klientow jesli stolik zajety; -1 jesli stolik wolny
    /// kolumna [1]: ilosc osob przy stoliku
    /// wymaga zmiennej globalnej clients

    for (int i = 0; i < world_ptr->clients_total; i++)
    {
        clients[i][0] = -1;
    }
}

void add_client(struct conversation *dialog)

{
    /// dodanie klienta do tablicy clients (notes managera)
    ///  zapisuje pid klienta oraz ilosc osob
    /// wymaga zmiennej globalnej clients

    for (int i = 0; i < world_ptr->clients_total; i++)
    {
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
    /// usuwa klienta z tablicy clients (notes managera)
    /// ustawia kolumne 0 na -1 (brak klienta)

    for (int i = 0; i < world_ptr->clients_total; i++)
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
    /// wpuszcza klienta do lokalu
    /// drukuje wiadomosc, wysyla komunikat "wejdz" w kolejce manager-client, dodaje klienta do notesu: add_client

    printf("\033[32m Manager:\t Witaj %ld. Mozesz wejsc, otrzymujesz stolik %d. Dodaje %d osob do listy klientow.\x1b[0m\n", dialog.pid, dialog.table_id, dialog.individuals);

    dialog.topic = WEJDZ;
    if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1) //
    {
        printf("manager: blad wysylania komunikatu WEJDZ\n");
        exit_handler(errno);
    }
    add_client(&dialog); // dodanie klienta do tablicy CLIENTS
}

void reject_client(struct conversation dialog)
{
    /// odmowa wejscia z powodu braku wolnego stolika
    /// drukuje wiadomosc, wysyla komunikat "brak miejsc" w kolejce mamanger-client

    printf("\033[32mManager:\t Brak wolnych miejsc. Nie przyjmuje klienta  %ld (%d osob).\x1b[0m\n", dialog.pid, dialog.individuals);

    dialog.topic = BRAK_MIEJSC;
    if (msgsnd(msg_manager_client_id, &dialog, conversation_size, 0) == -1)
    {
        printf("\033[32mmanager: reject_client: blad wysylania komunikatu BRAK_MIEJSC (msgsnd, msg_manager_client_id)\x1b[0m\n");
        exit_handler(errno);
    }
}

void client_leaves(struct conversation dialog)
{
    /// obsluga wyjscia klienta z lokalu
    /// drukowanie komunikatu w zaleznosci czy klient wychodzi normalnie czy sie ewakuuje
    /// usuwa klienta z tablicy clients: remove_client

    if (dialog.topic == EWAKUACJA)
    {
        printf("\033[32mManager:\t Klient %ld (osob: %d) ewakuowal sie pomyslnie\x1b[0m\n", dialog.pid, dialog.individuals);
    }
    else
    {
        printf("\033[32mManager:\t Do widzenia. Usuwam %ld (osob: %d) z listy klientow\x1b[0m\n", dialog.pid, dialog.individuals);
    }

    remove_client(&dialog);
}

void grant_seat(struct conversation dialog)
{
    /// algorytm przydzielania stolika
    /// wyszukiwanie zaczyna sie od najmniejszych mozliwych stolikow pasujacych do nowego klienta
    /// jezeli miejsce sie znajdzie to sprawdza czy grupy przy stoliku sa rownoliczne
    /// jesli znajdzie odpowiednie miejsce to ustawia komunikat i daje zgode poprzez allow_client_in
    /// jesli nie znajdzie miejsca to uzywa funkcji reject_client

    int seat_granted_flag = 0;

    for (int i = 0; i < world_ptr->tables_total; i++)
    {
        if (tables_ptr[i].free >= dialog.individuals && (tables_ptr[i].group_size == 0 || tables_ptr[i].group_size == dialog.individuals))
        {
            tables_ptr[i].free = tables_ptr[i].free - dialog.individuals;
            tables_ptr[i].group_size = dialog.individuals;
            dialog.table_id = tables_ptr[i].id;
            allow_client_in(dialog);
            seat_granted_flag = 1;
            break;
        }
    }

    if (seat_granted_flag == 0)
    {
        reject_client(dialog);
    }
}

void walk_to_the_door(struct conversation dialog)
{
    /// aktualizuje stan stolikow podczas wyjscia klienta
    /// aktualizuje ilosc wolnych miejsc
    /// jesli nikt inny nie pozostal przy stole to resetuje wskaznik wielkosci grupy przy danym stole
    /// odsyla do funkcji obslugujacej wyjscie client_leaves

    tables_ptr[dialog.table_id].free = tables_ptr[dialog.table_id].free + dialog.individuals;

    if (tables_ptr[dialog.table_id].free == tables_ptr[dialog.table_id].capacity)
    {
        tables_ptr[dialog.table_id].group_size = 0;
    }

    client_leaves(dialog);
}

void wait_for_interaction(struct conversation *dialog)
{
    /// oczekiwanie na komunikat od klienta
    /// posiada zabezpieczenie dzieki ktoremu przychodacy sygnal nie wywoluje
    /// bledu msgrcv

    if (msgrcv(msg_client_manager_id, dialog, conversation_size, 0, 0) == -1)
    {
        // odbieranie dowolnego komunikatu na linii CLIENT-MANAGER

        if (end_of_the_day_recently_triggered == 1 || fire_alarm_recently_triggered == 1)
        {
            // DO NOTHING
            // Zabezpieczenie przed falszywa wartoscia -1
            // Jeżeli proces czeka na msgrcv() i otrzyma sygnal "pożar" lub "koniec dnia" to funkcja zwraca -1
            // Aby różnicować ten przypadek od faktycznego bledu w odbieraniu komunikatu
            // wprowadzono flagi sterowane sygnalem end_of_the_day_recently_triggered i fire_alarm_recently_triggered
            // taka flaga blokuje aktywacje perror w ponizszym "else"
        }
        else
        {
            perror("Manager: main: blad odbierania (msgrcv, msg_client_manager_id) \n");
            exit_handler(errno);
        }
    }
}

void sigint_handler(int sig)
{
    // obsluga sygnalu SIGINT - obsluga przerwania
    // wysyla SIGINT do mainp
    // mainp obsluguje zamkniecie wszystkich procesow
    kill(getppid(), SIGINT);
    exit(1);
}

void sigint_hadler_init()
{
    // Inicjalizacja obsługi sygnału SIGINT
    // ustawia handler, który obsługuje sygnał przerwania: sigint_handler

    struct sigaction act;
    act.sa_handler = sigint_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);
}

void exit_handler(int error_number)
{
    // obsluga awaryjnego zamkniecia managera
    // wysyla informacje do mainp oraz konczy dzialanie z numerem bledu

    kill(getppid(), SIGINT);
    exit(error_number);
}