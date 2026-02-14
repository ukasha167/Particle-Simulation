#include <raylib.h>

#include "defines.h"
#include "solver.h"

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "1.67");
    SetTargetFPS(62);

    Texture2D sprite;
    {
        Image img = LoadImage("../circle.png");
        ImageResize(&img,
                    (int)(img.width * 0.01f),
                    (int)(img.height * 0.01f));

        sprite = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    while (!WindowShouldClose())
    {
        // UNROLLED LOOP TO MINIMIZE OVERHEAD (POOKIE ME) :(
        Solver::generateParticles();
        Solver::generateParticles();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            Solver::mousePush({(float)GetMouseX(), (float)GetMouseY()}, 60);
        }
        else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Solver::mousePull({(float)GetMouseX(), (float)GetMouseY()}, 60);
        }

        Solver::updateSimulationState(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);

        for (uint16_t i = 0; i < Solver::currentParticles; i++)
        {
            Vector2 drawPos = {
                Solver::particles.posX[i] - Solver::particles.radius,
                Solver::particles.posY[i] - Solver::particles.radius};

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