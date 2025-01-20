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
void fire_handler_init();
void send_end_of_the_day();
void create_client();
void wait_all_processes();

int main()
{
   sigint_hadler_init(); // inicjalizacja obslugi sygnalu SIGINT
   fire_handler_init(); //inicjalizacja obslugi sygnalu POZAR

   struct table *tables_ptr = init_tables(); // utworzenie tabeli, wypelnienie danymi, przekazanie do SHM
   msg_manager_client_id = init_msg_manager_client(); //utworzenie ID kolejki manager-client
   msg_client_manager_id = init_msg_client_manager(); //utworzenie ID kolejki client-manager

   create_manager_firefighter(); // uruchomienie procesu manager i firefighter
   sleep(1);
   create_client(); //tworzenie klienta
   sleep(10);
   send_end_of_the_day(); //komunikat KONIEC DNIA - nie bedzie wiecej klientow


   wait_all_processes();
   shmdt(tables_ptr);                     // odlaczanie pamieci tables
   shmctl(shm_id_tables, IPC_RMID, NULL); // usuwanie pamieci wspoldzielonej - tables
   msgctl(msg_manager_client_id, IPC_RMID, NULL); //usuniecie kolejki komunikatow - manager-client
   msgctl(msg_client_manager_id, IPC_RMID, NULL); //usuniecie kolejki komunikatow - client-manager

   return 0;
}

//===================FUNCJE=================================================================================================

void exit_handler() // funkcja SIGINT - obsluga przerwania
{
   msgctl(msg_manager_client_id, IPC_RMID, NULL);
   msgctl(msg_client_manager_id, IPC_RMID, NULL);
   shmctl(shm_id_tables, IPC_RMID, NULL);
   kill(0, SIGTERM);
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

void fire_handler_init()
{ // uruchamia obsluge sygnalu pozaru
   struct sigaction sa;
   sa.sa_handler = SIG_IGN; // ignoruje sygnal
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;

   if (sigaction(SIGUSR1, &sa, NULL) == -1)
   { // obsluga bledu sygnalu pozaru
      perror("main: blad ustawienia handlera pozaru\n");
      exit_handler();
   }
}

void send_end_of_the_day()
{
   struct conversation dialog; 
   dialog.pid = getpid();
   dialog.topic = KONIEC_DNIA;
   dialog.individuals = 0;
   if (msgsnd(msg_client_manager_id, &dialog, conversation_size, 0) == -1) //wyslanie komunikatu KONIEC DNIA
	{
      perror("mainp: blad wysylania komunikatu KONIEC DNIA\n");
      exit_handler();
	}   
}

void create_client()
{
   switch (fork()) 
   {
   case -1:
      perror("mainp: Blad fork kienta");
      exit_handler();
   case 0:
      execl("./bin/client", "client", "1", NULL);
   }
}

void wait_all_processes()
{
   while (wait(NULL) > 0) { //oczekiwanie na zakonczenie procesow potomnych
      //nothing here, just waiting
   }
}