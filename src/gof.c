#include "./raylib/include/raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

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
const double GOF_DELAY = 0.10; // sec
const float CAM_SPEED = 10.0f;

bool gof_is_running = true;
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
void draw_grid(cell (*cells)[GRID_SIZE_W]);
void cell_make_alive(cell (*cells)[GRID_SIZE_W], Camera2D *camera);
void analyze_cells(cell (*cells)[GRID_SIZE_W]);
void start_next_generation(cell (*cells)[GRID_SIZE_W]);

int main() {
    // Create the window --------------------------------------------------
    
        // Set resolution
    const int WIN_W = WIN_RES[2][0];
    const int WIN_H = WIN_RES[2][1];

        // Initialize
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
            cells[i][y].will_be_born = false;
            cells[i][y].tile.x       = pos_x;
            cells[i][y].tile.y       = pos_y;
            cells[i][y].tile.height  = TILE_SIZE;
            cells[i][y].tile.width   = TILE_SIZE;
        }
        pos_x = 0.0;
    }

        // Camera 2D
    Camera2D camera = {0};
    // camera.target   = (Vector2){0.0f, 0.0f};
    camera.offset   = (Vector2){WIN_W / 2.0f, WIN_H / 2.0f};
    // camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    // Main Loop -----------------------------------------------------
    while (!WindowShouldClose()) {
    BeginDrawing();
        ClearBackground(COLOR_BG);

        BeginMode2D(camera);

            draw_grid(cells);
            
            // Pause the life process while "drawing" cells
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                gof_is_running = false;
                cell_make_alive(cells, &camera);
            } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                gof_is_running = true;
            }
            
            if (gof_is_running) {
                analyze_cells(cells);
                start_next_generation(cells);
                WaitTime(GOF_DELAY);
            }
            
            // DrawGrid(10, 1.0f); // TODO: doesn't work properly, fix

        EndMode2D();

        DrawFPS(10, 10);
        // printf("%lf:%lf\n", GetMousePosition().x, GetMousePosition().y);        // DEBUG

        // Camera controls -----------------------------------------------------
        
            // Camera rotation
        // if (IsKeyDown(KEY_Q)) camera.rotation--;
        // else if (IsKeyDown(KEY_E)) camera.rotation++;
        
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
            camera.rotation = 0.0f;
        }
        // ---------------------------------------------------------------------

    EndDrawing();
    }

    CloseWindow(); // Close window and OpenGL context
    return 0;
}

// FUNCTION DEFENITIONS
// --------------------------------------------------------------------------------
void draw_grid(cell (*cells)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            Color color = (cells[i][y].alive) ? COLOR_TILE_ALIVE : COLOR_TILE_DEAD;
            DrawRectangleRec(cells[i][y].tile, color);
        }
    }
}

void cell_make_alive(cell (*cells)[GRID_SIZE_W], Camera2D *camera) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            Vector2 mouse_pos_cam_relative = GetScreenToWorld2D(GetMousePosition(), *camera);
            bool tile_clicked = CheckCollisionPointRec(mouse_pos_cam_relative, cells[i][y].tile);
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
