#include "raylib.h"

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
const float TILE_SIZE = 5.0;
const float TILE_GAP = 0.5;
const Color COLOR_TILE_ALIVE = GREEN;

// FUNCTIONS
void draw_grid(Rectangle (*grid)[GRID_SIZE_W]);

int main()
{
    // Create the window --------------------------------------------------
    const int WIN_W = WIN_RES[2][0]; // Changes window resolution
    const int WIN_H = WIN_RES[2][1];

    InitWindow(WIN_W, WIN_H, "Game of Life");
    SetTargetFPS(60);

    // Create the grid ----------------------------------------------------
    GRID_SIZE_W = WIN_W / (TILE_SIZE + TILE_GAP);
    GRID_SIZE_H = WIN_H / (TILE_SIZE + TILE_GAP);

    Rectangle tiles[GRID_SIZE_H][GRID_SIZE_W];
    float pos_x = 0;
    float pos_y = 0;
    for (int i = 0; i < GRID_SIZE_H; i++, pos_y += TILE_SIZE + TILE_GAP) {
        for (int y = 0; y < GRID_SIZE_W; y++, pos_x += TILE_SIZE + TILE_GAP) {
            tiles[i][y].x      = pos_x;
            tiles[i][y].y      = pos_y;
            tiles[i][y].height = TILE_SIZE;
            tiles[i][y].width  = TILE_SIZE;
        }
        pos_x = 0.0;
    }

    // Main Loop ---------------------------------------------------------
    while (!WindowShouldClose()) {
    BeginDrawing();
        ClearBackground(RAYWHITE);

        draw_grid(tiles);
        DrawGrid(10, 1.0f);

        DrawFPS(10, 10);

    EndDrawing();
    }

    CloseWindow(); // Close window and OpenGL context
    return 0;
}

// FUNCTION DEFENITIONS
void draw_grid(Rectangle (*grid)[GRID_SIZE_W]) {
    for (int i = 0; i < GRID_SIZE_H; i++) {
        for (int y = 0; y < GRID_SIZE_W; y++) {
            DrawRectangleRec(grid[i][y], COLOR_TILE_ALIVE);
        }
    }
}