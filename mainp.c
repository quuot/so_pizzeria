#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h> // C Standard changed to gnu99
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"
#include <errno.h>

int shm_id_tables = -1;           // ID pamieci wspoldzielonej TABLES (uklad stolikow)
struct table *tables_ptr = 0;     // wskaznik na TABLES w shm
int msg_manager_client_id = -1;   // ID kolejki MANAGER-->CLIENT
int msg_client_manager_id = -1;   // ID kolejki CLIENT-->MANAGER
int fire_alarm_triggered = 0;     // flaga pozaru, ustawiana na 1 po sygnale SIGUSR1
int shm_id_pizzeria1 = -1;        // utworzenie pamieci wspoldzielonej dla informacji o lokalu
struct world *pizzeria_1_ptr = 0; // wskaznik na WORLD w shm (dane poczatkowe)
int time_between_klients;         // czas pomiedzy tworzeniem kolejnych klientow, definiuje uzytkownik

void exit_handler(int code);
void sigint_handler(int sig);
void sigint_hadler_init();
struct table *init_tables(struct world *pizzeria);
void create_manager_firefighter(int seconds_untill_fire);
void create_client(int individuals);
void wait_all_processes();
void fire_handler_init();
void fire_handler(int sig);
void sigterm_handler_init();
void sigterm_handler(int sig);
void get_starting_data();
void validate_input(const char *message, int *variable, int min, int max);
void print_starting_data();

int main()
{
   sigint_hadler_init();         // inicjalizacja obslugi sygnalu SIGINT
   fire_handler_init();          // inicjalizacja obslugi sygnalu POZAR
   sigterm_handler_init();       // inicjalizacja obslugi sygnalu SIGTERM (pierwszy sygnal jest ignorowany, potem default)
   ignore_end_of_the_day_init(); // inicjalizacja obslugi sygnalu END OF THE DAY

   // inicjalizacja pamieci wspoldzielonej dla struktury world, pobranie wskaznika
   shm_id_pizzeria1 = init_shm_world();
   pizzeria_1_ptr = (struct world *)shmat(shm_id_pizzeria1, NULL, 0);
   if (pizzeria_1_ptr == (void *)-1)
   {
      perror("main: Blad uzyskania wskaznika (shmat,pizzeria_1_ptr).");
      exit_handler(errno);
   }

   // pobranie danych startowych od uzytkownika
   get_starting_data();

   tables_ptr = init_tables(pizzeria_1_ptr);          // utworzenie tabeli, wypelnienie danymi, przekazanie do shared memory
   msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client
   msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager

   // wyswietlenie danych startowych pobranych przez uzytkownika
   print_starting_data();

   // uruchomienie procesu manager i firefighter
   create_manager_firefighter(pizzeria_1_ptr->time_to_fire);

   // petla produkujaca klientow
   for (int i = 0; i < pizzeria_1_ptr->clients; i++)
   {
      if (fire_alarm_triggered == 1) // brak produkcji nowych klientow po alarmie pozarowym
      {
         break;
      }

      // int people = i % 3 + 1; //inna mozliwosc generowania ilosci osob (zalezna od petli)

      int people = rand() % 3 + 1; // podstawowa opcja losowania ilosci osob - wartosci losowe

      create_client(people);       // tworzenie klienta
      sleep(time_between_klients); // opoznienie miedzy klientami
   }

   sleep(1);

   kill(0, SIGUSR2); // sygnal KONIEC DNIA, nie bedzie wiecej klientow

   wait_all_processes();

   exit_handler(0); // zamkniecie zasobow IPC, terminacja procesow
}

void exit_handler(int code)
{
   // funkcja obsluguje zamkniecie programu mainp. Parametr: 0-zakonczenie prawidlowe, >0-zakonczenie bledem
   // odlacza pamiec TABLES i WORLD
   // usuwa kolejki: manager-client, client-manager
   // usuwa pamiec wspoldzielona tables i world

   // odlaczenie pamieci wspoldzielonej
   detach_mem_tables_world(tables_ptr, pizzeria_1_ptr);

   if (shm_id_tables != -1)
   {
      if (shmctl(shm_id_tables, IPC_RMID, NULL) == -1)
      {
         // usuniecie pamieci wspoldzielonej TABLES (uklad stolikow)
         perror("Mainp: exit_handler: Blad usuwania pamieci wspoldzielonej (shmctl).");
         exit(errno);
      }
   }

   if (shm_id_pizzeria1 != -1)
   {
      if (shmctl(shm_id_pizzeria1, IPC_RMID, NULL) == -1)
      {
         // usuniecie pamieci wspoldzielonej PIZZERIA/WORLD (dane poczatkowe)
         perror("Mainp: exit_handler: Blad usuwania pamieci wspoldzielonej (shmctl, shm_id_pizzeria1).");
         exit(errno);
      }
   }

   if (msg_manager_client_id != -1)
   {
      if (msgctl(msg_manager_client_id, IPC_RMID, NULL) == -1)
      {
         // usuniecie kolejki komunikatow MANAGER -> CLIENT
         perror("Mainp: exit_handler: Blad usuwania kolejki komunikatow manager-client (msgctl).");
         exit(errno);
      }
   }

   if (msg_client_manager_id != -1)
   {
      if (msgctl(msg_client_manager_id, IPC_RMID, NULL) == -1)
      { // usuniecie kolejki komunikatow CLIENT -> MANAGER
         perror("Mainp: exit_handler: Blad usuwania kolejki komunikatow client-manager (msgctl).");
         exit(errno);
      }
   }

   kill(0, SIGTERM);

   exit(code);
}

void sigint_handler(int sig)
{
   // obsluga sygnalu SIGINT - obsluga przerwania
   // terminuje wszystkie procesy z grupie procesow
   // konczy program przez exit_handler

   kill(0, SIGTERM);
   exit_handler(errno);
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

struct table *init_tables(struct world *pizzeria)
{
   // Inicjalizuje tablicę stolików w pamięci współdzielonej
   // tworzy segment pamięci współdzielonej na podstawie danych poczatkowych programu
   // zwraca wskaźnik do tablicy stolików w pamięci

   shm_id_tables = init_shm_tables(pizzeria);                                // inicjalizacja pamieci wspoldzielonej: TABLES (stoliki)(utils.h)
   struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0); // pobranie wskaznika pamieci wspoldzielonej tables
   if (tables_ptr == (void *)-1)
   {
      perror("Mainp: blad shmat dla TABLES\n");
      exit_handler(1);
   }

   for (int i = 0; i < pizzeria->x1; i++)
   {
      // inicjalizacja stolikow 1osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 1;
      tables_ptr[i].free = 1;
      tables_ptr[i].group_size = 0;
   }
   for (int i = pizzeria->x1; i < (pizzeria->x1 + pizzeria->x2); i++)
   {
      // inicjalizacja stolikow 2osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 2;
      tables_ptr[i].free = 2;
      tables_ptr[i].group_size = 0;
   }
   for (int i = (pizzeria->x1 + pizzeria->x2); i < (pizzeria->x1 + pizzeria->x2 + pizzeria->x3); i++)
   {
      // inicjalizacja stolikow 3osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 3;
      tables_ptr[i].free = 3;
      tables_ptr[i].group_size = 0;
   }
   for (int i = (pizzeria->x1 + pizzeria->x2 + pizzeria->x3); i < (pizzeria->x1 + pizzeria->x2 + pizzeria->x3 + pizzeria->x4); i++)
   {
      // inicjalizacja stolikow 4osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 4;
      tables_ptr[i].free = 4;
      tables_ptr[i].group_size = 0;
   }
   return tables_ptr;
}

void create_manager_firefighter(int seconds_untill_fire)
{
   // Tworzy procesy managera i strażaka.
   // manager odpowiada za zarządzanie lokalem,
   // strażak obsługuje alarm pożarowy po czasie podanym przez uzytkownika

   switch (fork()) // tworzenie procesu managera
   {
   case -1:
      perror("mainp: create_manager_firefighter: Blad fork managera");
      exit_handler(1);
   case 0:
      execl("./bin/manager", "manager", NULL);
   }

   switch (fork()) // tworzenie procesu firefighter
   {
   case -1:
      perror("mainp: create_manager_firefighter: Blad fork firefightera");
      exit_handler(1);
   case 0:
      char tmp_str[30];
      snprintf(tmp_str, sizeof(tmp_str), "%d", seconds_untill_fire); // przygotowanie argumentu wywolania
      execl("./bin/firefighter", "firefighter", tmp_str, NULL);
   }
}

void create_client(int individuals)
{
   // Tworzy proces klienta. Grupy 1,2 lub 3 osobowe
   // Parametr individuals to liczba osob w ramach jednego klienta.
   // dane przekazywane do funkcji sa sprawdzane
   // Jeśli wystapi pozar - procesy klientów nie są tworzone.

   if (fire_alarm_triggered == 1)
   {
      return;
   }

   if ((individuals > 0) && (individuals < 4)) // mozliwe tworzenie grup 1,2,3 - osobowych
   {
      switch (fork())
      {
      case -1:
         perror("mainp: create_client: Blad fork kienta");
         exit_handler(errno);
      case 0:
         char tmp_str[30];
         snprintf(tmp_str, sizeof(tmp_str), "%d", individuals);
         execl("./bin/client", "client", tmp_str, NULL);
         perror("main: create_client: blad execl ze zmienna iloscia klientow");
         exit_handler(errno);
      }
   }
   else
   {
      printf("Mainp: Nie utworzono procesu klienta - za duza ilosc klientow w grupie. Grupy moga byc 1, 2 lub 3 osobowe.\n");
   }
}

void wait_all_processes()
{
   // oczekiwanie na zakonczenie procesow potomnych

   while (wait(NULL) > 0)
   {
      // nothing here, just waiting
   }
}

void fire_handler_init()
{
   // uruchamia obsluge sygnalu pozaru - SIGUSR1
   // obsluga: funkcja fire_handler

   struct sigaction sa;
   sa.sa_handler = fire_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGUSR1, &sa, NULL) == -1)
   { // obsluga bledu sygnalu pozaru
      perror("Mainp: fire_handler_init: blad ustawienia handlera pozaru");
      exit_handler(errno);
   }
}

void fire_handler(int sig)
{
   // Obsługa sygnału pozar - SIGUSR1
   // Ustawia flagę, wymaga zmiennej globalnej fire_alarm_triggered

   fire_alarm_triggered = 1;
}

void sigterm_handler_init()
{
   // uruchamia obsluge sygnalu SIGTERM.
   // Sygnal jest ignorowany 1 raz. Potem dziala domyslnie --> sigterm_handler

   struct sigaction sa;
   sa.sa_handler = sigterm_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGTERM, &sa, NULL) == -1)
   {
      perror("Mainp: sigterm_handler_init: blad ustawienia handlera sigterm nr 1");
      exit_handler(errno);
   }
}

void sigterm_handler(int sig)
{
   // obsluga sygnalu SIGTERM: pierwszy sygnal jest ignorowany,
   // funkcja przywraca domyslna obsluge sygnalu

   struct sigaction sa;
   sa.sa_handler = SIG_DFL; // wartosc domyslna
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGTERM, &sa, NULL) == -1)
   {
      perror("Mainp: sigterm_handler: blad ustawienia handlera sigterm nr 1");
      exit_handler(errno);
   }
}

void get_starting_data()
{
   // pobranie danych startowych od uzytkownika
   // sprawdzenie danych statowych funkcja validate_input
   // zakres min - JUZ DOZWOLONY
   // zakres max - JESZCZE DOZWOLONY

   validate_input("Czas do aktywacji strazaka [0-120sek, 0=brak aktywacji]: ", &pizzeria_1_ptr->time_to_fire, 0, 120);
   validate_input("Ilosc stolikow 1 osobowych [0-10]: ", &pizzeria_1_ptr->x1, 0, 10);
   validate_input("Ilosc stolikow 2 osobowych [0-10]: ", &pizzeria_1_ptr->x2, 0, 10);
   validate_input("Ilosc stolikow 3 osobowych [0-10]: ", &pizzeria_1_ptr->x3, 0, 10);
   validate_input("Ilosc stolikow 4 osobowych [0-10]: ", &pizzeria_1_ptr->x4, 0, 10);
   validate_input("Ilosc klientow [1-4 000]: ", &pizzeria_1_ptr->clients, 1, 4000);
   validate_input("Odstep czasowy pomiedzy klientami [0-3s]: ", &time_between_klients, 0, 3);

   pizzeria_1_ptr->tables_total = pizzeria_1_ptr->x1 + pizzeria_1_ptr->x2 + pizzeria_1_ptr->x3 + pizzeria_1_ptr->x4;
   pizzeria_1_ptr->clients_total = pizzeria_1_ptr->x1 + (pizzeria_1_ptr->x2 * 2) + (pizzeria_1_ptr->x3 * 3) + (pizzeria_1_ptr->x4 * 4);
}

void validate_input(const char *message, int *variable, int min, int max)
{
   // sprawdza poprawnosc danych podanych z klawiatury
   // parametr 1: tresc promptu o podanie danych
   // parametr 2: adres do zapisu danych (wskaznik do zmiennej)
   // parametr 3: wartosc minimalna (juz dozwolona)
   // parametr 4: wartosc maxymalna (jeszcze dozwolona)

   printf("%s", message);

   if (scanf("%d", variable) != 1 || *variable < min || *variable > max)
   {
      fprintf(stderr, "Błąd: Bledne dane wejściowe. Stosuj sie do instrukcji w nawiasach [ ].\n");
      exit_handler(1);
   }
}

void print_starting_data()
{
   // drukuje wartosci startowe podane przez uzytkownika
   // wymaga zmiennej globalnej pizzeria_1_ptr

   printf("\nW lokalu mamy:\n%d stolik(i) jednoosobowe,\n%d stolik(i) dwuosobowe,\n%d stolik(i) trzyosobowe,\n%d stolik(i) czterosobowe\n", pizzeria_1_ptr->x1, pizzeria_1_ptr->x2, pizzeria_1_ptr->x3, pizzeria_1_ptr->x4);
   printf("Lokal miesci lacznie %d stolikow i maksymalnie %d klientow.\n", pizzeria_1_ptr->tables_total, pizzeria_1_ptr->clients_total);
   printf("Alarm pozarowy nastapi za %d sekund\n", pizzeria_1_ptr->time_to_fire);
   printf("========================================================================\n\n");
}
