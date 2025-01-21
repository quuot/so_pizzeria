#include <stdio.h>
#include <stdlib.h>
#define TABLE_ONE 4
#define TABLE_TWO 3
#define TABLE_THREE 2
#define TABLE_FOUR 1
#define TABLES_TOTAL 10
#define CLIENTS_TOTAL 20

#define CHCE_WEJSC 1
#define WEJDZ 2
#define BRAK_MIEJSC 3
#define DO_WIDZENIA 4
#define EWAKUACJA 5
#define conversation_size 3 * sizeof(int)

struct table
{
    long id;
    int capacity;
    int free;
    int group_size;
};

struct conversation
{
    long int pid;
    int topic;
    int individuals;
    int table_id;
};

int init_shm_tables();
void print_tables(struct table *tables_ptr);
int init_msg_manager_client();
int init_msg_client_manager();
void ignore_fire_handler_init();
void ignore_end_of_the_day_init();
