#include "./raylib/include/raylib.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "hash.h"

#define KILL 0
#define ADD 1
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
const float          CAM_ZOOM_MIN      = 1.0f;  // zoom out
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


// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------
void draw_grid_cells(hashtable *ht);
void cell_add_kill(hashtable *ht, Camera2D *camera, bool cursor_mode);
unsigned long long count_population(hashtable *ht);
void start_next_generation(hashtable *ht);
bool fps_filter(unsigned short *counter_fps);

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
            // TODO: GOF LOGIC HERE
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
            sprintf(str_speed, "Speed: %.2f%%", (100.0 / speed_reduct_rate));
            DrawText(str_speed, 10, 35, 20, LIME);
            // Generations
            char str_counter_generations[50];
            sprintf(str_counter_generations, "Generations: %llu", counter_generations);
            DrawText(str_counter_generations, 10, 60, 20, DARKBLUE);
            // Alive cells Counter
            char str_counter_population[50];
            sprintf(str_counter_population, "Population: %llu", counter_population);
            DrawText(str_counter_population, 10, 85, 20, DARKBLUE);
            // Alive cells Counter
            char str_cell_cursor_mode[20];
            sprintf(str_cell_cursor_mode, "Cursor mode: %s", (cell_cursor_mode) ? "ADD" : "KILL");
            DrawText(str_cell_cursor_mode, 10, 105, 20, DARKGRAY);
            // Cursor Position
            char str_cursor_pos[100];
            Vector2 cur_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);
            sprintf(str_cursor_pos, "Cursor position: (%d:%d)", (int)cur_mouse_pos.x, (int)cur_mouse_pos.y);
            DrawText(str_cursor_pos, 10, 125, 20, DARKGRAY);
            // DEBUG camera zoom
            char dbg_cam_zoom[100];
            snprintf(dbg_cam_zoom, 100, "Cam.Zoom: %f", camera.zoom);
            DrawText(dbg_cam_zoom, 10, 225, 20, GREEN);

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

bool fps_filter(unsigned short *counter_fps) {
    (*counter_fps)++;
    if (*counter_fps >= FPS) { *counter_fps = 0; }
    if (*counter_fps % speed_reduct_rate == 0) { return true; }
    return false;
}
