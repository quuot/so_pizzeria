#include <stdio.h>
#include <stdlib.h>

#define TABLE_ONE 4
#define TABLE_TWO 3
#define TABLE_THREE 2
#define TABLE_FOUR 1
#define TABLES_TOTAL 10


struct table{
    long id;
    int capacity;
    int free;
    int group_size;
};

int init_shm_tables();
void print_tables(struct table *tables, int tables_total);