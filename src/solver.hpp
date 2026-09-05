#ifndef SOLVER_H
#define SOLVER_H

#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>

#include "defines.hpp"
#include "particle.hpp"
#include "raylib.h"

class Solver
{
public:
    static Particle particles;

    inline static uint32_t currentParticlesCount = 0;

public:
    static void preComputeInitialValues();

    // SPAWNS UP TO 'count' PARTICLES. THE NOZZLE CHOKES AT EMITTER_CAPACITY AND
    // SKIPS ANY LATTICE SLOT THAT IS ALREADY OCCUPIED, SO THIS NEVER CREATES AN
    // OVERLAPPING PAIR. RETURNS HOW MANY WERE ACTUALLY EMITTED.
    static uint32_t generateParticles(const uint32_t count);

    // RUNS ONE FRAME ON A FIXED TIME STEP. DELIBERATELY TAKES NO dt.
    static void updateSimulationState();

    static void mousePush(const Vector2 &pos, const float radius);
    static void mousePull(const Vector2 &pos, const float radius);

private:
    // ONE FULL SET OF PARTICLE FIELDS. TWO OF THESE EXIST AND THE SOLVER PING-PONGS
    // BETWEEN THEM EVERY TIME IT REORDERS PARTICLES INTO SPATIAL ORDER.
    struct ParticleBuffer
    {
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> posX;
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> posY;
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> velX;
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> velY;
        alignas(64) std::array<Color, TOTAL_PARTICLES_COUNT> color;
    };

    static ParticleBuffer bufferA;
    static ParticleBuffer bufferB;
    static bool usingBufferA;

    // CELL c OCCUPIES SORTED PARTICLE INDICES [cellRange[c], cellRange[c + 1]).
    // ONE ARRAY GIVES BOTH THE START AND THE END OF EVERY CELL, AND BECAUSE THE
    // GRID IS ROW-MAJOR, THREE HORIZONTALLY ADJACENT CELLS ARE ONE CONTIGUOUS RUN.
    static std::array<uint32_t, CELL_RANGE_SIZE> cellRange;
    static std::array<uint32_t, TOTAL_PARTICLES_COUNT> particleCellIDs;

    static std::array<Color, 256> rainbowLUT;
    inline static uint32_t spawnColorCursor = 0;

    static void bindBuffers();
    static void computeColorValues();
    static void integrateAndBound(const float dt);
    static void buildGrid();
    // ShockPropagation = false: ordinary symmetric relaxation, momentum conserving.
    //                           This is the default and the original look.
    // ShockPropagation = true:  the lower particle of each contact is treated as
    //                           immovable, which is what makes tall piles stand up
    //                           like sand. Off unless SHOCK_PROPAGATION says so.
    template <bool ShockPropagation>
    static void solveContacts();
    static void applyBoundaries();

    // TRUE IF NOTHING ALREADY SITS WITHIN ONE DIAMETER OF (x, y). USES THE GRID
    // LEFT OVER FROM THE PREVIOUS FRAME, WHICH IS STILL EXACT BECAUSE NOTHING HAS
    // MOVED SINCE.
    static bool isSpawnSlotFree(const float x, const float y);
};

#endif
