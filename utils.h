#include <stdio.h>
#include <stdlib.h>

struct table{
    long table_id;
    int capacity;
    int free;
    int group_size;
};

int init_shm_tables();