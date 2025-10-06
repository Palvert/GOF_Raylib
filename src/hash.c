// NOTE: My goal is to create a DS that will store all the alive cells with their coordinates as the key
// So I wonder about the size of the array in hash table, how should I manage it's size: grow and shrink it
// by reallocating?
#include "hash.h"
#include "./raylib/include/raylib.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// --------------------------------------------------
// FUNCTIONS
// --------------------------------------------------

void init_hashtable(hashtable *ht,/* char *name,*/ size_t size) {
    // DELETE name related
    // ht->name = malloc(strlen(name) + 1);
    // if (ht->name == NULL) { perror("Allocation failed!"); exit(EXIT_FAILURE); }
    // strcpy(ht->name, name);

    ht->size = size;
    ht->cells = calloc(size, sizeof(cell*));
    if (ht->cells == NULL) { perror("Allocation failed!"); exit(EXIT_FAILURE); }
}


cell* create_cell(Vector2 position, float tile_size) {
    cell *new_cell = malloc(sizeof(cell));
    if (new_cell == NULL) { puts("Allocation failed!"); return NULL; }

    new_cell->will_survive = true; //change to false - default
    new_cell->tile.x       = position.x;
    new_cell->tile.y       = position.y;
    new_cell->tile.height  = tile_size;
    new_cell->tile.width   = tile_size;
    new_cell->next = NULL;

    return new_cell;
}



uint32_t hash(Vector2 position, uint64_t ht_size) {
    uint64_t hash = 5381;
    char coords_str[200] = {0};
    char coords_str_x[100] = {0};
    char coords_str_y[100] = {0};
    _itoa(abs((int)position.x), coords_str_x, 10);
    _itoa(abs((int)position.y), coords_str_y, 10);
    snprintf(coords_str, 200, "%s%s", coords_str_x, coords_str_y);
    printf("new hash:%s\n", coords_str);// DEBUG
    hash = atoi(coords_str);

    return hash % ht_size;
}

void ht_insert(hashtable *ht, cell *new_cell) {
    uint64_t index = hash((Vector2){.x = new_cell->tile.x, .y=new_cell->tile.y}, ht->size);
    // printf("cell:%.1fx%.1f to index:%lld\n", new_cell->tile.x, new_cell->tile.y, index); // DEBUG

    if (ht->cells[index] == NULL) {
        ht->cells[index] = new_cell;
    } else {
        cell *tmp = ht->cells[index];
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = new_cell;
    }
}

cell* ht_find(hashtable *ht, Vector2 position) {
    uint64_t index = hash(position, ht->size);
    cell *tmp = ht->cells[index];
    while (tmp != NULL) {
        if (tmp->tile.x == position.x && tmp->tile.y == position.y) {
            return tmp;
        } else {
            tmp = tmp->next;
        }
    }
    return NULL;
}

int ht_delete(hashtable *ht, Vector2 position) {
    uint64_t index = hash(position, ht->size);

    // If the wanted cell is first in the chain
    if (ht->cells[index]->tile.x == position.x && ht->cells[index]->tile.y == position.y) {
        if (ht->cells[index]->next) {
            cell *cell_next = ht->cells[index]->next;
            free(ht->cells[index]);
            ht->cells[index] = cell_next;
        } else {
            free(ht->cells[index]);
            ht->cells[index] = NULL;
        }
        return 0;
    }

    // Check the rest of the chain
    cell *tmp = ht->cells[index];
    while (tmp->next != NULL) {
        if (tmp->next->tile.x == position.x && tmp->next->tile.y == position.y) {
            if (tmp->next->next != NULL) {
                cell *tmp2 = tmp->next->next;
                free(tmp->next);
                tmp->next = tmp2;
            } else {
                free(tmp->next);
                tmp->next = NULL;
            }
            return 0;
        } else {
            tmp = tmp->next;
        }
    }
    return 1;
}

