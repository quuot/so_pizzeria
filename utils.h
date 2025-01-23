#include <stdio.h>
#include <stdlib.h>
#define TABLE_ONE 2
#define TABLE_TWO 2
#define TABLE_THREE 1
#define TABLE_FOUR 1
#define TABLES_TOTAL TABLE_ONE + TABLE_TWO + TABLE_THREE + TABLE_FOUR
#define CLIENTS_TOTAL TABLE_ONE + (TABLE_TWO * 2) + (TABLE_THREE * 3) + (TABLE_FOUR * 4)

#define CHCE_WEJSC 1
#define WEJDZ 2
#define BRAK_MIEJSC 3
#define DO_WIDZENIA 4
#define EWAKUACJA 5
#define conversation_size 3 * sizeof(int) // wielkosc struktury conversation bez uzglednienia wielkosci long int pid

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
