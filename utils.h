#include <stdio.h>
#include <stdlib.h>

#define TABLE_ONE 4
#define TABLE_TWO 3
#define TABLE_THREE 2
#define TABLE_FOUR 1
#define TABLES_TOTAL 10

#define CHCE_WEJSC 1 
#define WEJDZ 2 
#define BRAK_MIEJSC 3
#define DO_WIDZENIA 4


struct table
{
    long id;
    int capacity;
    int free;
    int group_size;
};

struct conversation{
	long int pid;
	int topic;
};

int init_shm_tables();
void print_tables(struct table *tables, int tables_total);