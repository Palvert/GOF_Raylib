#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>

Vector3 cubePosition = {0};

const int WIN_RES[][2] = {
    {1920, 1080},
    {1366, 768},
    {1280, 720},
    {800, 600},
    {640, 480},
    {320, 240}
};

int GRID_SIZE_W;
int GRID_SIZE_H;
const float TILE_SIZE = 15.0;
const float TILE_GAP = 1.0; // For some reason it doesn't work properly with < 1.0
const Color COLOR_TILE_ALIVE = WHITE;
const Color COLOR_TILE_DEAD = BLACK;
const Color COLOR_BG = BLACK;
const double GOF_DELAY = 0.1; // sec

bool gof_is_running = true;
int generation = 0;
int population = 0;

typedef struct cell {
    bool alive; // status
    bool will_survive;
    bool will_be_born;
    Rectangle tile; // visual
} cell;


// FUNCTIONS
void draw_grid(cell (*cells)[GRID_SIZE_W]);
void cell_make_alive(cell (*cells)[GRID_SIZE_W]);
void analyze_cells(cell (*cells)[GRID_SIZE_W]);
void start_next_generation(cell (*cells)[GRID_SIZE_W]);

int main() {
    // Create the window --------------------------------------------------
    const int WIN_W = WIN_RES[2][0]; // Changes window resolution
    const int WIN_H = WIN_RES[2][1];

    InitWindow(WIN_W, WIN_H, "Game of Life");
    SetTargetFPS(60);

    // Create the grid ----------------------------------------------------
    GRID_SIZE_W = WIN_W / (TILE_SIZE + TILE_GAP);
    GRID_SIZE_H = WIN_H / (TILE_SIZE + TILE_GAP);

    cell cells[GRID_SIZE_H][GRID_SIZE_W];
    float pos_x = 0;
    float pos_y = 0;
    for (int i = 0; i < GRID_SIZE_H; i++, pos_y += TILE_SIZE + TILE_GAP) {
        for (int y = 0; y < GRID_SIZE_W; y++, pos_x += TILE_SIZE + TILE_GAP) {
            cells[i][y].alive        = false;
            cells[i][y].will_survive = false;
            cells[i][y].will_be_born    = false;
            cells[i][y].tile.x       = pos_x;
            cells[i][y].tile.y       = pos_y;
            cells[i][y].tile.height  = TILE_SIZE;
            cells[i][y].tile.width   = TILE_SIZE;
        }
        pos_x = 0.0;
    }

    // Main Loop ---------------------------------------------------------
    while (!WindowShouldClose()) {
    BeginDrawing();
        ClearBackground(COLOR_BG);

        draw_grid(cells);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            gof_is_running = false;
            cell_make_alive(cells);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            gof_is_running = true;
        }
        
        // Pause the life process while "drawing" cells
        if (gof_is_running) {
            analyze_cells(cells);
            start_next_generation(cells);
            WaitTime(GOF_DELAY);
        }

        // DrawGrid(10, 1.0f);

        // DrawFPS(10, 10);
        // printf("%lf:%lf\n", GetMousePosition().x, GetMousePosition().y);        // DEBUG

    EndDrawing();
    }

    CloseWindow(); // Close window and OpenGL context
    return 0;
}

// FUNCTION DEFENITIONS
void draw_grid(cell (*cells)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            Color clr = (cells[i][y].alive) ? COLOR_TILE_ALIVE : COLOR_TILE_DEAD;
            DrawRectangleRec(cells[i][y].tile, clr);
        }
    }
}

void cell_make_alive(cell (*cells)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            bool tile_clicked = CheckCollisionPointRec(GetMousePosition(), cells[i][y].tile);
            if (tile_clicked) {
                cells[i][y].alive = true;
            }
        }
    }
}

void analyze_cells(cell (*cells)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            // Rule 1 -------------------------
            // (Any live cell with fewer than two live (< 2) neighbours dies as if caused by underpopulation.)

            // Scan 8 closest neghibors
            int neighbors = 0;

            int iy = i - 1;
            int ix = y - 1;
            for (int row = 0; row < 3; row++, iy++, ix = y - 1) {
                for (int col = 0; col < 3; col++, ix++) {
                    if (cells[iy][ix].alive &&
                        iy >= 0 && iy < GRID_SIZE_H &&
                        ix >= 0 && ix < GRID_SIZE_W &&
                        (iy != i || ix != y)) {
                        neighbors++;
                    }
                }
            }

            // Kill the cell
            if (neighbors == 2 || neighbors == 3) {
                cells[i][y].will_survive = true;
            } else {cells[i][y].will_survive = false;}
            
            // Make alive
            if (neighbors == 3) {
                cells[i][y].will_be_born = true;
            } else {cells[i][y].will_be_born = false;}
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