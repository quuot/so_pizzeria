#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h> // C Standard changed to gnu99
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"

int shm_id_tables; // przypisanie wartosci w init_tables()
struct table *tables_ptr;
int msg_manager_client_id;
int msg_client_manager_id;
int fire_alarm_triggered = 0;

void exit_handler(int code);
void sigint_handler(int sig);
void sigint_hadler_init();
struct table *init_tables();
void create_manager_firefighter();
void create_client(int individuals);
void wait_all_processes();
void fire_handler_init();
void fire_handler(int sig);
void sigterm_handler_init();
void sigterm_handler(int sig);

int main()
{
   sigint_hadler_init();         // inicjalizacja obslugi sygnalu SIGINT
   fire_handler_init();          // inicjalizacja obslugi sygnalu POZAR
   sigterm_handler_init();       // inicjalizacja obslugi sygnalu SIGTERM (pierwszy sygnal jest ignorowany, potem default)
   ignore_end_of_the_day_init(); // inicjalizacja obslugi sygnalu END OF THE DAY

   tables_ptr = init_tables();                        // utworzenie tabeli, wypelnienie danymi, przekazanie do SHM
   msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client
   msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager

   create_manager_firefighter(); // uruchomienie procesu manager i firefighter
   sleep(1);

   for (int i = 0; i < 10; i++)
   {
      if (fire_alarm_triggered == 1)
      {
         break;
      }

      int people = i % 3 + 1;
      create_client(people);
      sleep(1);
   }

   sleep(1);
   kill(0, SIGUSR2); // sygnal KONIEC DNIA, nie bedzie wiecej klientow

   wait_all_processes();

   exit_handler(0); // zamkniecie zasobow IPC, terminacja procesow
   printf("Main: zakonczenie.\n");
   return 0;
}

//===================FUNCJE=================================================================================================

void exit_handler(int code)
{
   // funkcja obsguje zamkniecie programu mainp. Parametr: 0-zakonczenie prawidlowe, >0-zakonczenie bledem

   if (shmdt(tables_ptr) == -1)
   { // odlaczenie pamieci wspoldzielonej TABLES (uklad stolikow)
      perror("Mainp: Blad odlaczania pamieci wspoldzielonej (shmdt).");
      exit(1);
   }

   if (shmctl(shm_id_tables, IPC_RMID, NULL) == -1)
   { // usuniecie pamieci wspoldzielonej TABLES (uklad stolikow)
      perror("Mainp: Blad usuwania pamieci wspoldzielonej (shmctl).");
      exit(1);
   }

   if (msgctl(msg_manager_client_id, IPC_RMID, NULL) == -1)
   { // usuniecie kolejki komunikatow MANAGER -> CLIENT
      perror("Mainp: Blad usuwania kolejki komunikatow manager-client (msgctl).");
      exit(1);
   }

   if (msgctl(msg_client_manager_id, IPC_RMID, NULL) == -1)
   { // usuniecie kolejki komunikatow CLIENT -> MANAGER
      perror("Mainp: Blad usuwania kolejki komunikatow client-manager (msgctl).");
      exit(1);
   }
   kill(0, SIGTERM);
   exit(code);
}

void sigint_handler(int sig)
{ // funkcja SIGINT - obsluga przerwania
   kill(0, SIGTERM);
   exit_handler(1);
}

void sigint_hadler_init()
{ // inicjalizacja obslugi SIGINT
   struct sigaction act;
   act.sa_handler = sigint_handler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT, &act, 0);
}

struct table *init_tables()
{
   struct table tables[TABLES_TOTAL]; // inicjalizacja tablicy tables

   shm_id_tables = init_shm_tables();                                        // inicjalizacja pamieci wspoldzielonej: TABLES (stoliki)(utils.h)
   struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0); // pobranie wskaznika pamieci wspoldzielonej tables
   if (tables_ptr == (void *)-1)
   {
      perror("Mainp: blad shmat dla TABLES\n");
      exit_handler(1);
   }

   for (int i = 0; i < TABLE_ONE; i++)
   { // inicjalizacja stolikow 1osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 1;
      tables_ptr[i].free = 1;
      tables_ptr[i].group_size = 0;
   }
   for (int i = TABLE_ONE; i < (TABLE_ONE + TABLE_TWO); i++)
   { // inicjalizacja stolikow 2osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 2;
      tables_ptr[i].free = 2;
      tables_ptr[i].group_size = 0;
   }
   for (int i = (TABLE_ONE + TABLE_TWO); i < (TABLE_ONE + TABLE_TWO + TABLE_THREE); i++)
   { // inicjalizacja stolikow 3osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 3;
      tables_ptr[i].free = 3;
      tables_ptr[i].group_size = 0;
   }
   for (int i = (TABLE_ONE + TABLE_TWO + TABLE_THREE); i < (TABLE_ONE + TABLE_TWO + TABLE_THREE + TABLE_FOUR); i++)
   { // inicjalizacja stolikow 4osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 4;
      tables_ptr[i].free = 4;
      tables_ptr[i].group_size = 0;
   }

   return tables_ptr;
}

void create_manager_firefighter()
{
   switch (fork()) // tworzenie procesu managera
   {
   case -1:
      perror("mainp: Blad fork managera");
      exit_handler(1);
   case 0:
      execl("./bin/manager", "manager", NULL);
   }

   switch (fork()) // tworzenie procesu firefighter
   {
   case -1:
      perror("mainp: Blad fork firefightera");
      exit_handler(1);
   case 0:
      execl("./bin/firefighter", "firefighter", NULL);
   }
}

void create_client(int individuals)
{
   if (fire_alarm_triggered == 1)
   {
      return;
   }

   if ((individuals > 0) && (individuals < 4)) // mozliwe tworzenie grup 1,2,3 - osobowych
   {
      switch (fork())
      {
      case -1:
         perror("mainp: Blad fork kienta");
         exit_handler(1);
      case 0:
         char tmp_str[30];
         snprintf(tmp_str, sizeof(tmp_str), "%d", individuals);
         execl("./bin/client", "client", tmp_str, NULL);
         perror("main: blad execl ze zmienna iloscia klientow");
         exit_handler(1);
      }
   }
   else
   {
      printf("Mainp: Nie utworzono procesu klienta - za duza ilosc klientow w grupie. Grupy moga byc 1, 2 lub 3 osobowe.\n");
   }
}

void wait_all_processes()
{
   while (wait(NULL) > 0)
   { // oczekiwanie na zakonczenie procesow potomnych
     // nothing here, just waiting
   }
}

void fire_handler_init()
{ // uruchamia obsluge sygnalu pozaru
   struct sigaction sa;
   sa.sa_handler = fire_handler; // ignoruje sygnal
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGUSR1, &sa, NULL) == -1)
   { // obsluga bledu sygnalu pozaru
      perror("Mainp: blad ustawienia handlera pozaru\n");
      exit_handler(1);
   }
}

void fire_handler(int sig)
{
   fire_alarm_triggered = 1;
}

void sigterm_handler_init()
{ // uruchamia obsluge sygnalu SIGTERM. Sygnal jest ignorowany 1 raz. Potem dziala standardowo.
   struct sigaction sa;
   sa.sa_handler = sigterm_handler;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGTERM, &sa, NULL) == -1)
   {
      perror("Mainp: blad ustawienia handlera sigterm nr 1\n");
      exit_handler(1);
   }
}

void sigterm_handler(int sig)
{ // obsluga sygnalu SIGTERM: pierwszy sygnal jest ignorowany, potem dzialanie defaultowe.
   struct sigaction sa;
   sa.sa_handler = SIG_DFL; // ignoruje sygnal
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGTERM, &sa, NULL) == -1)
   {
      perror("Mainp: blad ustawienia handlera sigterm nr 1\n");
      exit_handler(1);
   }
}
