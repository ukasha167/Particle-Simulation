#ifndef PARTICLES_H
#define PARTICLES_H

#include <vector>

#include "defines.h"
#include "raylib.h"

struct Particle
{
    static std::array<float, TOTAL_PARTICLE_COUNT> posX;
    static std::array<float, TOTAL_PARTICLE_COUNT> posY;
    static std::array<float, TOTAL_PARTICLE_COUNT> velX;
    static std::array<float, TOTAL_PARTICLE_COUNT> velY;

    static constexpr float radius = 5.0f;
};

#endif