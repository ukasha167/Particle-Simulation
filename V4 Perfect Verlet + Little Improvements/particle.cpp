#ifndef PARTICLES_H
#define PARTICLES_H

#include <array>

#include "defines.h"

struct Particle
{
    // THE ONLY REASON TO USE std::array<> INSTEAD OF array[] IS TO USE IT'S HELPING FUNCTIONS
    // - std::fill()
    // - std::min()
    // - std::clamp()
    static std::array<float, TOTAL_PARTICLES_COUNT> posX;
    static std::array<float, TOTAL_PARTICLES_COUNT> posY;

    static std::array<float, TOTAL_PARTICLES_COUNT> oldX;
    static std::array<float, TOTAL_PARTICLES_COUNT> oldY;

    static std::array<float, TOTAL_PARTICLES_COUNT> accX;
    static std::array<float, TOTAL_PARTICLES_COUNT> accY;

    inline static float radius = MAX_PARTICLE_RADIUS;
};

#endif