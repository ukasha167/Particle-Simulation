#include <raylib.h>

#include "solver.h"
#include "renderer.h"

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "1.67");
    SetTargetFPS(62);

    // - THE ASSET'S LOCATION
    // - DIAMTER OF A PARTICLE (Greater Than 0)
    Renderer::loadAsset("../assets/circle.png", MAX_PARTICLE_RADIUS * 2);
    Solver::setParticlesInitialValues();

    bool pause = false;
    while (!WindowShouldClose())
    {
        // TOGGLE PARTICLE_GENERATION WITH MOUSE CLICK
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            pause = !pause;
        }

        if (!pause)
        {
            Solver::generateParticles(GENERATE_PARTICLES_PER_SECOND); // BREAKS IF > 1 :(
        }

        Solver::updateSimulationState(GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);

        Renderer::drawParticles(Particle::posX, Particle::posY, Solver::currentParticlesCount);
        Renderer::drawStats(GetFPS(), Solver::currentParticlesCount);

        EndDrawing();
    }

    Renderer::unloadAsset();
    CloseWindow();

    return 0;
}