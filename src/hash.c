#include "./raylib/include/raylib.h"
#include "hash.h"

typedef struct node {
    Vector2 key;
    // Insert cell's parameters here, right now just a test value
    char* value;

    struct node* next;
} node;

typedef struct htable {
    unsigned long long size;
    node** table; 
} htable;


// --------------------------------------------------
// FUNCTIONS
// --------------------------------------------------

unsigned long long hash_function(Vector2 key, unsigned long long size) {
    unsigned long long index;

    return index;
}
