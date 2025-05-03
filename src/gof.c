#include "./raylib/include/raylib.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "hash.h"

#define KILL 0
#define ADD 1

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
const Color CLR_WHITE   = (Color) { 220, 220, 220, 255 };
const Color CLR_BLACK   = (Color) { 10, 10, 10, 255 };
const Color CLR_MIDGRAY = (Color) { 127, 127, 127, 255 };
const Color CLR_DEBUG   = (Color) { 15, 15, 15, 255};

// Constants
int GRID_SIZE_W;
int GRID_SIZE_H;
const Color             COLOR_BG           = CLR_BLACK;
const Color             COLOR_TILE_ALIVE   = WHITE;
const Color             COLOR_TILE_DEAD    = CLR_DEBUG;//COLOR_BG;
const Color             COLOR_TEXT         = CLR_MIDGRAY;
const float             CAM_MAX_ZOOM       = 3.15f;
const float             CAM_MIN_ZOOM       = 0.15f;
const float             CAM_SPEED          = 10.0f;
const float             TILE_GAP           = 1.0; // For some reason it doesn't work properly with < 1.0
const float             TILE_SIZE          = 15.0;
const unsigned short    FPS                = 60;

// Variables
unsigned short          speed_reduct_rate  = 6;

bool gof_on_hold = false;
bool gof_paused = false;
int generation = 0;
int population = 0;

typedef struct cell {
    bool alive; // status
    bool will_survive;
    bool will_be_born;
    Rectangle tile; // visual
} cell;


// FUNCTION PROTOTYPES
// --------------------------------------------------------------------------------
void build_grid_texture(cell (*cells)[GRID_SIZE_W], RenderTexture2D *grid);
void cell_add_kill(cell (*cells)[GRID_SIZE_W], Camera2D *camera, int mode);
unsigned long long count_population(const cell (*cells)[GRID_SIZE_W]);
void analyze_cells(cell (*cells)[GRID_SIZE_W]);
void start_next_generation(cell (*cells)[GRID_SIZE_W]);
bool fps_filter(unsigned short *counter_fps);

int t = 300; //DEBUG

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
    SetTargetFPS(FPS);

    // GRID (CELLS DATA) --------------------------------------------------
    GRID_SIZE_W = t;
    GRID_SIZE_H = t;
    cell (*cells)[GRID_SIZE_W] = malloc(GRID_SIZE_H * sizeof(*cells));

    if (cells == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);

        return 1;
    }

    float pos_x = 0;
    float pos_y = 0;
    for (int i = 0; i < GRID_SIZE_H; i++, (pos_y += TILE_SIZE + TILE_GAP)) {
        for (int y = 0; y < GRID_SIZE_W; y++, (pos_x += TILE_SIZE + TILE_GAP)) {
            cells[i][y].alive        = false;
            cells[i][y].will_survive = false;
            cells[i][y].will_be_born = false;
            cells[i][y].tile.x       = pos_x;
            cells[i][y].tile.y       = pos_y;
            cells[i][y].tile.height  = TILE_SIZE;
            cells[i][y].tile.width   = TILE_SIZE;
        }
        pos_x = 0.0;
    }

    // GRID (GRAPHICS) --------------------------------------------------
    RenderTexture2D grid = LoadRenderTexture(GRID_SIZE_W * (TILE_SIZE + TILE_GAP), 
                                             GRID_SIZE_H * (TILE_SIZE + TILE_GAP));
    build_grid_texture(cells, &grid);

    // CAMERA --------------------------------------------------
    Camera2D camera = {0};
    camera.target   = (Vector2){ WIN_W / 2.0f, WIN_H / 2.0f };
    camera.offset   = (Vector2){ WIN_W / 2.0f, WIN_H / 2.0f };
    // camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    // PROGRAM LOOP -----------------------------------------------------
    while (!WindowShouldClose()) {

    // UPDATE ------------------------------------------------------------
        // Manual Pause
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE)) {
            gof_paused = !gof_paused; 
            gof_on_hold = gof_paused;
        }
        
        // CAMERA CONTROLS -----------------------------------------------------
        // Camera movement
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 local_mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);

            // Y Axis
            if (local_mouse_pos.y < camera.target.y) {
                     camera.target.y -= fabsf(camera.target.y - local_mouse_pos.y) / CAM_SPEED;
            } else { camera.target.y += fabsf(camera.target.y - local_mouse_pos.y) / CAM_SPEED; }
            // X Axis
            if (local_mouse_pos.x < camera.target.x) {
                     camera.target.x -= fabsf(camera.target.x - local_mouse_pos.x) / CAM_SPEED;
            } else { camera.target.x += fabsf(camera.target.x - local_mouse_pos.x) / CAM_SPEED; }

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
        if (camera.zoom < 0.15) { camera.zoom = CAM_MIN_ZOOM; }
        if (camera.zoom > 3.00) { camera.zoom = CAM_MAX_ZOOM; }
        
        // Game speed controls
        if      (IsKeyPressed(KEY_MINUS) && speed_reduct_rate < 10) { speed_reduct_rate++; }
        else if (IsKeyPressed(KEY_EQUAL) && speed_reduct_rate > 1) { speed_reduct_rate--; }

        // Cursor mode (cell drawing)
        if (IsKeyPressed(KEY_M)) { cell_cursor_mode = !cell_cursor_mode; }

        // GOF SIMULATION --------------------------------------------------
        // Pause the life process while "drawing" cells
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            gof_on_hold = true;
            cell_add_kill(cells, &camera, cell_cursor_mode);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            gof_on_hold = false;
        }
        
        if ((!gof_on_hold && !gof_paused) && fps_filter(&counter_fps)) {
            analyze_cells(cells);
            start_next_generation(cells);
        }
            
        // COUNTERS --------------------------------------------------------
        unsigned long long counter_population = count_population(cells);
        if (counter_population > 0 && !gof_on_hold && !gof_paused) {
            counter_generations++;
        }

        // DRAW ------------------------------------------------------------
        BeginDrawing();
            ClearBackground(COLOR_BG);

            build_grid_texture(cells, &grid); 

            BeginMode2D(camera);
                DrawTexture(grid.texture, 0, 0, WHITE);
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

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

// FUNCTION DEFENITIONS
// --------------------------------------------------------------------------------
void build_grid_texture(cell (*cells)[GRID_SIZE_W], RenderTexture2D *grid) {
    // NOTE: texture renders Y-flipped (for some reason), so rect coloring v-reversed
    BeginTextureMode(*grid);
    for (int y = 0, rev_y = (GRID_SIZE_H - 1); y < GRID_SIZE_H; y++, rev_y--) {
        for (int x = 0; x < GRID_SIZE_W; x++) {
            Color color = (cells[y][x].alive) ? COLOR_TILE_ALIVE : COLOR_TILE_DEAD;
            DrawRectangleRec(cells[rev_y][x].tile, color);
        }
    }
    EndTextureMode();
}

void cell_add_kill(cell (*cells)[GRID_SIZE_W], Camera2D *camera, int mode) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            Vector2 mouse_pos_cam_relative = GetScreenToWorld2D(GetMousePosition(), *camera);
            bool tile_clicked = CheckCollisionPointRec(mouse_pos_cam_relative, cells[i][y].tile);
            if (tile_clicked) {
                cells[i][y].alive = mode;
            }
        }
    }
}

unsigned long long count_population(const cell (*cells)[GRID_SIZE_W]) {
    unsigned long long count_current = 0;

    // Count living cells
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            if (cells[i][y].alive) {
                count_current++;
            }
        }
    }
    
    return count_current;
}

void analyze_cells(cell (*cells)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            
            // Count the alive neighbors around the cell
            int neighbors = 0;

            int iy = i - 1;
            int ix = y - 1;
            for (int row = 0; row < 3; row++, iy++, ix = y - 1) {
                for (int col = 0; col < 3; col++, ix++) {
                    if (iy >= 0 && iy < GRID_SIZE_H &&
                        ix >= 0 && ix < GRID_SIZE_W) {
                        if (cells[iy][ix].alive && (iy != i || ix != y)) {
                            neighbors++;
                        }
                    }
                }
            }

            // Is the alive cell surivies
            if (cells[i][y].alive) {
                if (neighbors == 2 || neighbors == 3) {
                    cells[i][y].will_survive = true;
                } else {
                    cells[i][y].will_survive = false; 
                }
            }
            
            // Is the alive cell will be born
            if (!cells[i][y].alive && neighbors == 3) {
                cells[i][y].will_be_born = true;
            } else {cells[i][y].will_be_born = false; }
        }
    }
}

void start_next_generation(cell (*cells)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            if (cells[i][y].alive) {
                cells[i][y].alive = cells[i][y].will_survive;
            } else {
                cells[i][y].alive = cells[i][y].will_be_born;
            }
            // cells[i][y].alive = cells[i][y].will_survive;
        }
    }
}

bool fps_filter(unsigned short *counter_fps) {
    (*counter_fps)++;
    if (*counter_fps >= FPS) { *counter_fps = 0; }
    if (*counter_fps % speed_reduct_rate == 0) { return true; }
    return false;
}
