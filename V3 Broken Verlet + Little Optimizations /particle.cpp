#ifndef PARTICLES_H
#define PARTICLES_H

#include <vector>
#include <array>
#include "defines.h"

struct Particle
{
    static std::array<Vector2, TOTAL_PARTICLE_COUNT> currentPositions;
    static std::array<Vector2, TOTAL_PARTICLE_COUNT> oldPositions;
    static std::array<Vector2, TOTAL_PARTICLE_COUNT> accelerations;

    inline static float radius = MAX_PARTICLE_RADIUS;
};

#endif