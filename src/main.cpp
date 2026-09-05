#include <cstdint>
#include <ctime>
#include <raylib.h>

#include "defines.hpp"
#include "solver.hpp"
#include "renderer.hpp"

int main()
{
    srand(static_cast<unsigned>(time(nullptr)));
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "1.68");

    // THE SOLVER RUNS ON A FIXED TIME STEP OF ITS OWN AND NEVER READS GetFrameTime(),
    // SO THIS ONLY CONTROLS HOW OFTEN WE PRESENT. IF THE MACHINE CANNOT KEEP UP THE
    // SIMULATION GOES INTO SLOW MOTION INSTEAD OF DESTABILISING.
    SetTargetFPS(static_cast<int>(SIMULATION_FPS));

    // PARAMETERS:
    // - THE ASSET'S LOCATION
    // - DIAMTER OF A PARTICLE (Should Be Greater Than 0)
    // RETURNS:
    // - 1 IF DIAMETER IS INVALID OR IMAGE NOT FOUND
    // - 0 IF EVERYTHING GOES WELL
    if (Renderer::loadAsset("../assets/circle.png", static_cast<uint8_t>(PARTICLE_DIAMETER)))
    {
        return 1;
    }

    // BUILDS THE COLOR TABLE AND CLEARS THE PARTICLE STATE
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
            Solver::generateParticles(SPAWN_PER_FRAME);
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
        {
            Solver::mousePush(GetMousePosition(), 200.0f);
        }
        else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
        {
            Solver::mousePull(GetMousePosition(), 200.0f);
        }

        // JUST A SINGLE FUNCTION THAT HANDLES ALL OF THE SIMULATION
        Solver::updateSimulationState();

        // RENDERING PART:

        // RAYLIB SYNTAX
        BeginDrawing();
        ClearBackground(BLACK);

        // DRAW THE NUMBER OF PARTICLES THE SIMULATION CURRENTLY KNOWS
        Renderer::drawParticles(Solver::particles, Solver::currentParticlesCount);

        // DRAW FPS AND THE PARTICLE COUNT (REMOVING IT MIGHT ACTUALLY IMPROVE PERFORMANCE)
        Renderer::drawStats(static_cast<uint16_t>(GetFPS()), Solver::currentParticlesCount);

        EndDrawing();
    }

    Renderer::unloadAsset();
    CloseWindow();

    return 0;
}
