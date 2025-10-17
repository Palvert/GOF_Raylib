#include "./raylib/include/raylib.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "hash.h"

enum modes {
    KILL, ADD
};
#define uplimit(a,b) (((a) < (b)) ? (a) : (b))

Vector3 cubePosition = {0};

const int WIN_RES[][2] = {
    {1920, 1080},
    {1366, 768},
    {1280, 720},
    {800, 600},
    {640, 480},
    {320, 240}
};

// Color palette
const Color CLR_WHITE     = { 220, 220, 220, 255 };
const Color CLR_BLACK     = { 10, 10, 10, 255 };
const Color CLR_LIGHTGRAY = { 200, 200, 200, 255 };
const Color CLR_MIDGRAY   = { 127, 127, 127, 255 };
const Color CLR_DEBUG     = { 15, 15, 15, 255};

// Constants
const size_t CELLS_IN_GRID = 5120000;

  // Color theme. Comment out to switch
  // dark
const Color          COLOR_BG          = CLR_BLACK;
const Color          COLOR_TILE_ALIVE  = CLR_WHITE;
const Color          COLOR_TEXT        = CLR_MIDGRAY;

  // light
// const Color          COLOR_BG          = CLR_WHITE;
// const Color          COLOR_TILE_ALIVE  = CLR_BLACK;
// const Color          COLOR_TILE_DEAD   = CLR_LIGHTGRAY;
// const Color          COLOR_TEXT        = CLR_MIDGRAY;

const float          CAM_ZOOM_DEF      = 5.0f;  // zoom default
const float          CAM_ZOOM_MAX      = 10.0f; // zoom in
const float          CAM_ZOOM_MIN      = 0.1f;  // zoom out
const float          CAM_SPD_MODIF     = 10.0f;
const float          TILE_GAP          = 0.5;
const float          TILE_SIZE         = 4.0;
const unsigned short FPS               = 60;

// Variables
unsigned short          speed_reduct_rate  = 6;

bool gof_on_hold = false;
bool gof_paused = false;
unsigned long long generation = 0;
unsigned long long population = 0;

// Structures
typedef struct list {
    Vector2 pos;
    struct list *next;
} list;

// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------
void draw_grid_cells(hashtable *ht);
void cell_add_kill(hashtable *ht, Camera2D *camera, bool cursor_mode);
unsigned long long count_population(hashtable *ht);
void apply_game_rules(hashtable *ht);
bool fps_filter(unsigned short *counter_fps);
void snap_to_grid(Vector2 *pos);
static void list_add(list **the_list, Vector2 pos_to_add);
static void free_list(list **list_to_clear);
static void _execute_list(list *list_to_exec, short exec_mode, hashtable *ht);

// MAIN FUNCTION
// --------------------------------------------------------------------------------
int main() {
    unsigned short counter_fps = 0;
    unsigned long long counter_generations = 0;
    bool cell_cursor_mode = ADD;
    
    // WINDOW --------------------------------------------------
    const int WIN_W = WIN_RES[2][0];
    const int WIN_H = WIN_RES[2][1];

    InitWindow(WIN_W, WIN_H, "Game of Life");
    // SetTargetFPS(FPS);

    // GRID (CELLS DATA) --------------------------------------------------
    hashtable *ht_cells = malloc(sizeof(hashtable));
    if (ht_cells == NULL) { perror("Memory allocation failed! (cells)"); exit(EXIT_FAILURE); }

    init_hashtable(ht_cells, CELLS_IN_GRID);
    // DEBUG
    // test three-line
    ht_insert(ht_cells, create_cell((Vector2){40, 20}, TILE_SIZE));
    ht_insert(ht_cells, create_cell((Vector2){40, 20+TILE_SIZE}, TILE_SIZE));
    ht_insert(ht_cells, create_cell((Vector2){40, 20+(TILE_SIZE*2)}, TILE_SIZE));

    // test cube
    // ht_insert(ht_cells, create_cell((Vector2){40, 20}, TILE_SIZE));
    // ht_insert(ht_cells, create_cell((Vector2){40+TILE_SIZE, 20}, TILE_SIZE));
    // ht_insert(ht_cells, create_cell((Vector2){40, 20+TILE_SIZE}, TILE_SIZE));
    // ht_insert(ht_cells, create_cell((Vector2){40+TILE_SIZE, 20+TILE_SIZE}, TILE_SIZE));

    // CAMERA --------------------------------------------------
    Camera2D camera = {0};
    camera.target = (Vector2){0.0f, 0.0f};
    camera.offset = (Vector2){ WIN_W / 2.0f, WIN_H / 2.0f };
    // camera.rotation = 0.0f;
    camera.zoom   = CAM_ZOOM_DEF;
    
    // PROGRAM LOOP (UPDATE -----------------------------------------------------
    while (!WindowShouldClose()) {
        // Manual Pause
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
            gof_paused = !gof_paused; 
            gof_on_hold = gof_paused;
        }
        
        // CAMERA CONTROLS -----------------------------------------------------
        // Camera movement
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const int CAM_SPD_LIMIT = 100;
            Vector2 local_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);

            // Y Axis
            if (local_mouse_pos.y < camera.target.y) {
                     camera.target.y -= uplimit(fabsf(camera.target.y - local_mouse_pos.y) / CAM_SPD_MODIF, CAM_SPD_LIMIT);
            } else { camera.target.y += uplimit(fabsf(camera.target.y - local_mouse_pos.y) / CAM_SPD_MODIF, CAM_SPD_LIMIT); }
            // X Axis
            if (local_mouse_pos.x < camera.target.x) {
                     camera.target.x -= uplimit(fabsf(camera.target.x - local_mouse_pos.x) / CAM_SPD_MODIF, CAM_SPD_LIMIT);
            } else { camera.target.x += uplimit(fabsf(camera.target.x - local_mouse_pos.x) / CAM_SPD_MODIF, CAM_SPD_LIMIT); }
        }
        
        // Camera zoom
        camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));
        
        // Reset camera
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
        {
            camera.target = (Vector2){0.0f, 0.0f};
            // camera.zoom = 1.0f;
        }

        // Limit zoom
        if (camera.zoom < CAM_ZOOM_MIN) { camera.zoom = CAM_ZOOM_MIN; }
        if (camera.zoom > CAM_ZOOM_MAX) { camera.zoom = CAM_ZOOM_MAX; }
        
        // Game speed controls
        if      (IsKeyPressed(KEY_MINUS) && speed_reduct_rate < 10) { speed_reduct_rate++; }
        else if (IsKeyPressed(KEY_EQUAL) && speed_reduct_rate > 1) { speed_reduct_rate--; }

        // Cursor mode (cell drawing)
        if (IsKeyPressed(KEY_M)) { cell_cursor_mode = !cell_cursor_mode; }

        // GOF SIMULATION --------------------------------------------------
        // Pause the life process while "drawing" cells
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            gof_on_hold = true;
            cell_add_kill(ht_cells, &camera, cell_cursor_mode);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            gof_on_hold = false;
        }
        
        if ((!gof_on_hold && !gof_paused) && fps_filter(&counter_fps)) {
            apply_game_rules(ht_cells);
        }
            
        // COUNTERS --------------------------------------------------------
        uint64_t counter_population = count_population(ht_cells);
        if (counter_population > 0 && !gof_on_hold && !gof_paused) {
            counter_generations++;
        }

        // DRAW ------------------------------------------------------------
        BeginDrawing();
            ClearBackground(COLOR_BG);


            BeginMode2D(camera);
                draw_grid_cells(ht_cells); 
            EndMode2D();

            // UI -----------------------------------------------------
            // Text Pause
            if (gof_paused) {
                DrawText("Paused", (WIN_W / 2 - 40), 10, 20, MAROON);
            }
            // FPS
            DrawFPS(10, 10);
            // Text Speed
            char str_speed[20];
            snprintf(str_speed, 20, "Speed: %.2f%%", (100.0 / speed_reduct_rate));
            DrawText(str_speed, 10, 35, 20, LIME);
            // Generations
            char str_counter_generations[50];
            snprintf(str_counter_generations, 50, "Generations: %llu", counter_generations);
            DrawText(str_counter_generations, 10, 60, 20, DARKBLUE);
            // Alive cells Counter
            char str_counter_population[50];
            snprintf(str_counter_population, 50, "Population: %llu", counter_population);
            DrawText(str_counter_population, 10, 85, 20, DARKBLUE);
            // Alive cells Counter
            char str_cell_cursor_mode[20];
            snprintf(str_cell_cursor_mode, 20, "Cursor mode: %s", (cell_cursor_mode) ? "ADD" : "KILL");
            DrawText(str_cell_cursor_mode, 10, 105, 20, DARKGRAY);
            // Cursor Position
            char str_cursor_pos[100];
            Vector2 cur_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
            snprintf(str_cursor_pos, 100, "Cursor position: (%d:%d)", (int)cur_mouse_pos.x, (int)cur_mouse_pos.y);
            DrawText(str_cursor_pos, 10, 125, 20, DARKGRAY);
            // DEBUG camera zoom
            // char dbg_cam_zoom[100];
            // snprintf(dbg_cam_zoom, 100, "Cam.Zoom: %f", camera.zoom);
            // DrawText(dbg_cam_zoom, 10, 225, 20, GREEN);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// FUNCTION DEFENITIONS
// --------------------------------------------------------------------------------
void draw_grid_cells(hashtable *ht) {
    for (uint64_t i = 0; i < ht->size; i++) {
        cell *tmp = ht->cells[i];
        if (tmp != NULL) {
            while (tmp != NULL) {
                Rectangle cell_inner_clr = {0};
                cell_inner_clr.x      = tmp->tile.x      + TILE_GAP;
                cell_inner_clr.y      = tmp->tile.y      + TILE_GAP;
                cell_inner_clr.width  = tmp->tile.width  - (TILE_GAP * 2.0f);
                cell_inner_clr.height = tmp->tile.height - (TILE_GAP * 2.0f);

                DrawRectangleRec(tmp->tile, RED);
                DrawRectangleRec(cell_inner_clr, COLOR_TILE_ALIVE);
                tmp = tmp->next;
            }
        }
    }
}

void cell_add_kill(hashtable *ht, Camera2D *camera, bool cursor_mode) {
    Vector2 mouse_pos_cam_relative = GetScreenToWorld2D(GetMousePosition(), *camera);
    
    snap_to_grid(&mouse_pos_cam_relative);

    // Check if no cell has the same position
    bool pos_is_free = true;
    for (uint64_t i = 0; i < ht->size; i++) {
        cell *tmp = ht->cells[i];
        if (tmp != NULL && (tmp->tile.x == mouse_pos_cam_relative.x && tmp->tile.y == mouse_pos_cam_relative.y)) {
            pos_is_free = false;
            break;
        }
    }

    if (pos_is_free && cursor_mode) {
        ht_insert(ht, create_cell(mouse_pos_cam_relative, TILE_SIZE));
    } else if (!pos_is_free && !cursor_mode) {
        printf("%d-%d\n", (int)mouse_pos_cam_relative.x, (int)mouse_pos_cam_relative.y); // DEBUG
        ht_delete(ht, mouse_pos_cam_relative);
    }
}

uint64_t count_population(hashtable *ht) {
    uint64_t count = 0;

    for (uint64_t i = 0; i < ht->size; i++) {
        cell *tmp = ht->cells[i];
        if (tmp != NULL) {
            while (tmp != NULL) {
                count++;
                tmp = tmp->next;
            }
        }
    }

    return count;
}

static short _count_cell_neighbours(hashtable *ht, Vector2 position) {
    short counter = 0;
    if (ht_find(ht, (Vector2){position.x - TILE_SIZE, position.y - TILE_SIZE})) { counter++; }
    if (ht_find(ht, (Vector2){position.x,             position.y - TILE_SIZE})) { counter++; }
    if (ht_find(ht, (Vector2){position.x + TILE_SIZE, position.y - TILE_SIZE})) { counter++; }
    if (ht_find(ht, (Vector2){position.x - TILE_SIZE, position.y            })) { counter++; }
    if (ht_find(ht, (Vector2){position.x + TILE_SIZE, position.y            })) { counter++; }
    if (ht_find(ht, (Vector2){position.x            , position.y + TILE_SIZE})) { counter++; }
    if (ht_find(ht, (Vector2){position.x - TILE_SIZE, position.y + TILE_SIZE})) { counter++; }
    if (ht_find(ht, (Vector2){position.x + TILE_SIZE, position.y + TILE_SIZE})) { counter++; }
    return counter;
}

void apply_game_rules(hashtable *ht) {
    list *life_list  = NULL; // list of the cells that will be born
    list *death_list = NULL; // list of the cells that will die

    for (uint64_t i = 0; i < ht->size; i++) {
        cell *tmp = ht->cells[i];
        if (tmp != NULL) {
            while (tmp != NULL) {
                short count_neighbours_alive = 0;
                
                // Check the neighbours
                // Create neighbours' data
                struct neighbour {
                    bool is_alive;
                    Vector2 pos;
                    short neighbours_count;
                };
                struct neighbour neighbours[8];
                for (int nbr_i = 0; nbr_i < 8; nbr_i++) {
                    neighbours[nbr_i] = (struct neighbour){.is_alive = false, .neighbours_count = 0};
                }
                
                // Define positions around the current cell
                neighbours[0].pos = (Vector2){tmp->tile.x - TILE_SIZE, tmp->tile.y - TILE_SIZE};
                neighbours[1].pos = (Vector2){tmp->tile.x,             tmp->tile.y - TILE_SIZE};
                neighbours[2].pos = (Vector2){tmp->tile.x + TILE_SIZE, tmp->tile.y - TILE_SIZE};
                neighbours[3].pos = (Vector2){tmp->tile.x - TILE_SIZE, tmp->tile.y            };
                neighbours[4].pos = (Vector2){tmp->tile.x + TILE_SIZE, tmp->tile.y            };
                neighbours[5].pos = (Vector2){tmp->tile.x            , tmp->tile.y + TILE_SIZE}; // tip: 1 2 3
                neighbours[6].pos = (Vector2){tmp->tile.x - TILE_SIZE, tmp->tile.y + TILE_SIZE}; //      4   5
                neighbours[7].pos = (Vector2){tmp->tile.x + TILE_SIZE, tmp->tile.y + TILE_SIZE}; //      6 7 8

                // Find alive neighbours of the current cell
                for (int nbr_i = 0; nbr_i < 8; nbr_i++) {
                    if (ht_find(ht, neighbours[nbr_i].pos)) { neighbours[nbr_i].is_alive = true; count_neighbours_alive++; }
                }

                // Apply Rule 1-3: (under/over)population
                if (count_neighbours_alive < 2 || count_neighbours_alive > 3) {
                    Vector2 current_cell_pos = (Vector2){tmp->tile.x, tmp->tile.y};
                    if (ht_find(ht, current_cell_pos)) { list_add(&death_list, current_cell_pos); }
                }
                // Apply Rule 4: reproduction
                // Find the position of "the dead cell" with 3 alive neighbours, insert there a new cell
                for (int nbr_i = 0; nbr_i < 8; nbr_i++) {
                    if (neighbours[nbr_i].is_alive == false) {
                        neighbours[nbr_i].neighbours_count = _count_cell_neighbours(ht, neighbours[nbr_i].pos);

                        if (neighbours[nbr_i].neighbours_count == 3) {
                            list_add(&life_list, neighbours[nbr_i].pos);
                        }
                    }
                }

                tmp = tmp->next;
            }
        }
    }
    
    // Execute lists
    _execute_list(death_list, KILL, ht);
    _execute_list(life_list, ADD, ht);

    // Clean up the lists
    free_list(&life_list);
    free_list(&death_list);
}

bool fps_filter(unsigned short *counter_fps) {
    (*counter_fps)++;
    if (*counter_fps >= FPS) { *counter_fps = 0; }
    if (*counter_fps % speed_reduct_rate == 0) { return true; }
    return false;
}

void snap_to_grid(Vector2 *pos) {
    pos->x = round(pos->x / TILE_SIZE) * TILE_SIZE;
    pos->y = round(pos->y / TILE_SIZE) * TILE_SIZE;
}

static void list_add(list **list_head, Vector2 pos_to_add) {
    list *tmp = *list_head;
    
    if (*list_head == NULL) {
        *list_head = malloc(sizeof(list));
        (*list_head)->pos = pos_to_add;
        (*list_head)->next = NULL;
    } else {
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = malloc(sizeof(list));
        tmp->next->pos = pos_to_add;
        tmp->next->next = NULL;
    }
}

static void free_list(list **list_to_clear) {
    list *current = *list_to_clear; 
    list *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    *list_to_clear = NULL;
}

static void _execute_list(list *list_to_exec, short exec_mode, hashtable *ht) {
    list *list_tmp = list_to_exec;
    while (list_tmp != NULL) {
        switch (exec_mode) {
            case (ADD) : ht_insert(ht, create_cell(list_tmp->pos, TILE_SIZE)); break;
            case (KILL): ht_delete(ht, list_tmp->pos); break;
        }
        list_tmp = list_tmp->next;
    }
}
