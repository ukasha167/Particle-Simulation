#include <raylib.h>

#include "solver.h"
#include "renderer.h"

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "1.67");
    SetTargetFPS(62);

    Renderer::loadAsset("../circle.png", 10);

    while (!WindowShouldClose())
    {
        Solver::generateParticles(5);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            Solver::mousePush({(float)GetMouseX(), (float)GetMouseY()}, 120);
        }
        else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            Solver::mousePull({(float)GetMouseX(), (float)GetMouseY()}, 120);
        }

        Solver::updateSimulationState(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);

        Renderer::drawParticles(Particle::currentPositions, Solver::currentParticles);
        Renderer::drawStats(GetFPS(), Solver::currentParticles);

        EndDrawing();
    }

    Renderer::unloadAsset();
    CloseWindow();

    return 0;
}