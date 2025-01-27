#include <stdio.h>
#include <stdlib.h>

#define NOTEBOOK_LENGTH 100

// komunikaty do synchronizacji client-manager
#define CHCE_WEJSC 1
#define WEJDZ 2
#define BRAK_MIEJSC 3
#define DO_WIDZENIA 4
#define EWAKUACJA 5

#define conversation_size 3 * sizeof(int) // wielkosc struktury conversation (bez uzglednienia wielkosci long int)
#define world_size 8 * sizeof(int)        // wielkosc struktury world (bez uzglednienia wielkosci long int)

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

struct world
{
    long int world_id;
    int time_to_fire;
    int x1;
    int x2;
    int x3;
    int x4;
    int tables_total;
    int clients_total;
    int clients;
};

extern const char *colors[];

int init_shm_tables(struct world *world_ptr);
void print_tables(struct table *tables_ptr);
int init_msg_manager_client();
int init_msg_client_manager();
void ignore_fire_handler_init();
void ignore_end_of_the_day_init();
int init_shm_world();
void cprintf(const char *color, const char *format, ...);
void detach_mem_tables_world(struct table *tables_ptr, struct world *world_ptr);
