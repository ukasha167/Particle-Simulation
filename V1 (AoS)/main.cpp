#include <raylib.h>

#include "defines.h"
#include "solver.h"

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "1.67");
    SetTargetFPS(61);

    Texture2D sprite;
    {
        Image img = LoadImage("../circle.png");
        ImageResize(&img,
                    (int)(img.width * 0.01f),
                    (int)(img.height * 0.01f));

        sprite = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    uint16_t fps = 0;
    while (WindowShouldClose() == false)
    {
        fps += GetFPS();

        if (fps >= (120))
        {
            Solver::generateParticles();
            fps = 0;
        }

        Solver::updateSimulationState(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);

        for (uint16_t i = 0; i < Solver::currentParticles; i++)
        {
            Vector2 drawPos = {
                Solver::particles[i].position.x - Solver::particles[i].radius,
                Solver::particles[i].position.y - Solver::particles[i].radius};

            DrawTextureV(sprite, drawPos, WHITE);
        }

        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 24, WHITE);
        DrawText(TextFormat("Particles: %d", Solver::currentParticles), 10, 35, 24, WHITE);
        EndDrawing();
    }
    UnloadTexture(sprite);
    CloseWindow();

    return 0;
}