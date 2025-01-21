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
int msg_manager_client_id;
int msg_client_manager_id;

void exit_handler();
void sigint_handler(int sig);
void sigint_hadler_init();
struct table *init_tables();
void create_manager_firefighter();
void create_client(int individuals);
void wait_all_processes();

int main()
{
   sigint_hadler_init();         // inicjalizacja obslugi sygnalu SIGINT
   ignore_fire_handler_init();   // inicjalizacja obslugi sygnalu POZAR
   ignore_end_of_the_day_init(); // inicjalizacja obslugi sygnalu END OF THE DAY

   struct table *tables_ptr = init_tables();          // utworzenie tabeli, wypelnienie danymi, przekazanie do SHM
   msg_manager_client_id = init_msg_manager_client(); // utworzenie ID kolejki manager-client
   msg_client_manager_id = init_msg_client_manager(); // utworzenie ID kolejki client-manager

   create_manager_firefighter(); // uruchomienie procesu manager i firefighter
   sleep(1);

   // create_client(1);
   // sleep(2);
   // create_client(2);
   // sleep(2);
   // create_client(3);
   // sleep(2);
   // create_client(2);

   for (int i = 0; i < 20; i++)
   {
      int people = i % 3 + 1;
      create_client(people);
      sleep(1);
   }

   sleep(1);
   kill(0, SIGUSR2); // TO NIE MOŻE BYC SYGNAL LUB SYGNAL NIE MOZE ZAKLOCAC DZIALANIA WHILE. ROZWAZYC MSG QUEUE

   wait_all_processes();
   shmdt(tables_ptr);                             // odlaczanie pamieci tables
   shmctl(shm_id_tables, IPC_RMID, NULL);         // usuwanie pamieci wspoldzielonej - tables
   msgctl(msg_manager_client_id, IPC_RMID, NULL); // usuniecie kolejki komunikatow - manager-client
   msgctl(msg_client_manager_id, IPC_RMID, NULL); // usuniecie kolejki komunikatow - client-manager

   return 0;
}

//===================FUNCJE=================================================================================================

void exit_handler() // funkcja SIGINT - obsluga przerwania
{
   msgctl(msg_manager_client_id, IPC_RMID, NULL);
   msgctl(msg_client_manager_id, IPC_RMID, NULL);
   shmctl(shm_id_tables, IPC_RMID, NULL);
   kill(0, SIGTERM); // zabija procesy z process-group
   // semctl(semID,0,IPC_RMID,NULL); TODO:zamkniecie semafora
   exit(1);
}

void sigint_handler(int sig) // funkcja SIGINT - obsluga przerwania
{
   msgctl(msg_manager_client_id, IPC_RMID, NULL);
   msgctl(msg_client_manager_id, IPC_RMID, NULL);
   shmctl(shm_id_tables, IPC_RMID, NULL);
   // semctl(semID,0,IPC_RMID,NULL); TODO:zamkniecie semafora
   exit(1);
}

void sigint_hadler_init()
{
   struct sigaction act; // obsluga sygnalu SIGINT
   act.sa_handler = sigint_handler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT, &act, 0);
}

struct table *init_tables()
{
   struct table tables[TABLES_TOTAL]; // inicjalizacja tablicy tables

   shm_id_tables = init_shm_tables();                                        // inicjalizacja pamieci wspoldzielonej: tables
   struct table *tables_ptr = (struct table *)shmat(shm_id_tables, NULL, 0); // pobranie wskaznika pamieci wspoldzielonej tables
   if (tables_ptr == (void *)-1)
   {
      perror("main: blad shmat dla TABLES\n");
      exit_handler();
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
      exit_handler();
   case 0:
      execl("./bin/manager", "manager", NULL);
   }

   switch (fork()) // tworzenie procesu firefighter
   {
   case -1:
      perror("mainp: Blad fork firefightera");
      exit_handler();
   case 0:
      execl("./bin/firefighter", "firefighter", NULL);
   }
}

void create_client(int individuals)
{
   if ((individuals > 0) && (individuals < 4)) // mozliwe tworzenie grup 1,2,3 - osobowych
   {
      switch (fork())
      {
      case -1:
         perror("mainp: Blad fork kienta");
         exit_handler();
      case 0:
         char tmp_str[30];
         snprintf(tmp_str, sizeof(tmp_str), "%d", individuals);
         execl("./bin/client", "client", tmp_str, NULL);
         perror("main: blad execl ze zmienna iloscia klientow");
         exit_handler();
      }
   }
   else
   {
      printf("!!!Mainp: Nie utworzono procesu klienta - za duza ilosc klientow w grupie. Grupy moga byc 1, 2 lub 3 osobowe.\n");
   }
}

void wait_all_processes()
{
   while (wait(NULL) > 0)
   { // oczekiwanie na zakonczenie procesow potomnych
     // nothing here, just waiting
   }
}
