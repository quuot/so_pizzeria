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

int shm_id_tables; // przypisanie wartosci znajduje sie w init_tables()
struct table *tables_ptr;
int msg_manager_client_id;
int msg_client_manager_id;
int fire_alarm_triggered = 0;
int shm_id_pizzeria1; //utworzenie pamieci wspoldzielonej dla informacji o lokalu
struct world *pizzeria_1_ptr;

void exit_handler(int code);
void sigint_handler(int sig);
void sigint_hadler_init();
struct table *init_tables(struct world* pizzeria);
void create_manager_firefighter(int seconds_untill_fire);
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

   shm_id_pizzeria1 = init_shm_world();
   pizzeria_1_ptr = (struct world *)shmat(shm_id_pizzeria1, NULL, 0);

   if(pizzeria_1_ptr == (void *)-1) {
      perror("main: Blad shmat dla pizzeria_1_ptr.");
      exit_handler(1);
   }

   printf("Czas do aktywacji strazaka [sek, 0=brak aktywacji]: ");
   scanf("%d", &pizzeria_1_ptr->time_to_fire);
   printf("Ilosc stolikow 1 osobowych: ");
   scanf("%d", &pizzeria_1_ptr->x1);
   printf("Ilosc stolikow 2 osobowych: ");
   scanf("%d", &pizzeria_1_ptr->x2);
   printf("Ilosc stolikow 3 osobowych: ");
   scanf("%d", &pizzeria_1_ptr->x3);
   printf("Ilosc stolikow 4 osobowych: ");
   scanf("%d", &pizzeria_1_ptr->x4);
   printf("Ilosc klientow: ");
   scanf("%d", &pizzeria_1_ptr->clients);

   pizzeria_1_ptr->tables_total = pizzeria_1_ptr->x1 + pizzeria_1_ptr->x2 + pizzeria_1_ptr->x3 + pizzeria_1_ptr->x4;
   pizzeria_1_ptr->clients_total = pizzeria_1_ptr->x1 + (pizzeria_1_ptr->x2 * 2) + (pizzeria_1_ptr->x3 * 3) + (pizzeria_1_ptr->x4 * 4);



   tables_ptr = init_tables(pizzeria_1_ptr);              // utworzenie tabeli, wypelnienie danymi, przekazanie do shared memory
   msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client
   msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager


   printf("\nW lokalu mamy:\n%d stolik(i) jednoosobowe,\n%d stolik(i) dwuosobowe,\n%d stolik(i) trzyosobowe,\n%d stolik(i) czterosobowe\n", pizzeria_1_ptr->x1, pizzeria_1_ptr->x2, pizzeria_1_ptr->x3, pizzeria_1_ptr->x4);
   printf("Lokal miesci lacznie %d stolikow i maksymalnie %d klientow.\n", pizzeria_1_ptr->tables_total , pizzeria_1_ptr->clients_total);
   printf("Alarm pozarowy nastapi za %d sekund\n", pizzeria_1_ptr->time_to_fire);
   printf("========================================================================\n\n");

   create_manager_firefighter(pizzeria_1_ptr->time_to_fire); // uruchomienie procesu manager i firefighter
   sleep(1);

   for (int i = 0; i < pizzeria_1_ptr->clients; i++)
   {
      if (fire_alarm_triggered == 1) //brak produkcji nowych klientow po alarmie pozarowym
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

struct table *init_tables(struct world *pizzeria)
{
   //struct table tables[pizzeria.tables_total]; // prawdopodobnie niepotrzebne

   shm_id_tables = init_shm_tables(pizzeria);                                        // inicjalizacja pamieci wspoldzielonej: TABLES (stoliki)(utils.h)
   struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0); // pobranie wskaznika pamieci wspoldzielonej tables
   if (tables_ptr == (void *)-1)
   {
      perror("Mainp: blad shmat dla TABLES\n");
      exit_handler(1);
   }

   for (int i = 0; i < pizzeria->x1; i++)
   { // inicjalizacja stolikow 1osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 1;
      tables_ptr[i].free = 1;
      tables_ptr[i].group_size = 0;
   }
   for (int i = pizzeria->x1; i < (pizzeria->x1 + pizzeria->x2); i++)
   { // inicjalizacja stolikow 2osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 2;
      tables_ptr[i].free = 2;
      tables_ptr[i].group_size = 0;
   }
   for (int i = (pizzeria->x1 + pizzeria->x2); i < (pizzeria->x1 + pizzeria->x2 + pizzeria->x3); i++)
   { // inicjalizacja stolikow 3osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 3;
      tables_ptr[i].free = 3;
      tables_ptr[i].group_size = 0;
   }
   for (int i = (pizzeria->x1 + pizzeria->x2 + pizzeria->x3); i < (pizzeria->x1 + pizzeria->x2 + pizzeria->x3 + pizzeria->x4); i++)
   { // inicjalizacja stolikow 4osobowych
      tables_ptr[i].id = i;
      tables_ptr[i].capacity = 4;
      tables_ptr[i].free = 4;
      tables_ptr[i].group_size = 0;
   }
   return tables_ptr;
}

void create_manager_firefighter(int seconds_untill_fire)
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
      char tmp_str[30];
      snprintf(tmp_str, sizeof(tmp_str), "%d", seconds_untill_fire);
      execl("./bin/firefighter", "firefighter", tmp_str, NULL);
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
