#include <raylib.h>

#include "solver.h"
#include "renderer.h"

int main()
{
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "1.67");

    SetTargetFPS(62); // RAYLIB UNDER THE HOOD USES FRAMETIME TO SLOW DOWN THE SIMULATION,
    // AND FOR SOME REASONS IT ALWAYS SETS THE FPS BY SOME NUMBER BELOW THE SPECIFIED LIMIT
    // SO TO KEEP THE SIMULATION AT A SOLID @60FPS, WE NEED TO USE JUST A BIT HIGHER NUMBER

    // PARAMETERS:
    // - THE ASSET'S LOCATION
    // - DIAMTER OF A PARTICLE (Should Be Greater Than 0)
    // RETURNS:
    // - 1 IF DIAMETER IS INVALID OR IMAGE NOT FOUND
    // - 0 IF EVERYTHING GOES WELL
    if (Renderer::loadAsset("../assets/circle.png", MAX_PARTICLE_RADIUS * 2))
    {
        return 1;
    }

    // CALCULATES THE INITIAL VELOCITIES, AND COLOR VALUES FOR THE PARTICLES
    Solver::preComputeInitialValues();

    bool pause = false;
    while (!WindowShouldClose())
    {
        // TOGGLE PARTICLE_GENERATION WITH SPACEBAR
        if (IsKeyPressed(KEY_SPACE))
        {
            pause = !pause;
        }

        if (!pause)
        {
            Solver::generateParticles(GENERATE_PARTICLES_PER_SECOND);
        }

        // JUST A SINGLE FUNCTION THAT HANDLES ALL OF THE SIMULATION
        Solver::updateSimulationState(GetFrameTime());

        // RENDERING PART:

        // RAYLIB SYNTAX
        BeginDrawing();
        ClearBackground(BLACK);

        // DRAW THE NUMBER OF PARTICLES THE SIMULATION CURRENTLY KNOWS
        Renderer::drawParticles(Solver::particles, Solver::currentParticlesCount);

        // DRAW FPS AND THE PARTICLE COUNT (REMOVING IT MIGHT ACTUALLY IMPROVE PERFORMANCE)
        Renderer::drawStats(GetFPS(), Solver::currentParticlesCount);

        EndDrawing();
    }

    Renderer::unloadAsset();
    CloseWindow();

    return 0;
}