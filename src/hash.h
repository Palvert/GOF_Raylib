#pragma once

#include <stdlib.h>
#include "./raylib/include/raylib.h"
#include <stdint.h>

typedef struct cell {
    bool will_survive;
    bool will_be_born;
    Rectangle tile;

    struct cell *next;
} cell;

typedef struct hashtable {
    char *name; // DELETE
    size_t size;
    cell **cells;
} hashtable;


uint32_t hash(Vector2 position, uint64_t ht_size);
void init_hashtable(hashtable *ht, size_t size);
cell* create_cell(Vector2 position, float tile_size);
void ht_insert(hashtable *ht, cell *new_cell);
cell* ht_find(hashtable *ht, Vector2 position);
int ht_delete(hashtable *ht, Vector2 position);
