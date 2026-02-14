#ifndef PARTICLES_H
#define PARTICLES_H

#include <array>
#include <raylib.h>

#include "defines.h"

struct Particle
{
    // THE ONLY REASON TO USE std::array<> INSTEAD OF 'float array[]' IS TO USE IT'S HELPING FUNCTIONS, BECAUSE THEY GENERATE EFFICIENT ASSEMBLY AND I WAS A TOO LAZY TO CODE THESE FUNCTIONS MYSELF :(
    // - std::fill()
    // - std::min()
    // - std::clamp()
    static std::array<float, TOTAL_PARTICLES_COUNT> posX;
    static std::array<float, TOTAL_PARTICLES_COUNT> posY;

    static std::array<float, TOTAL_PARTICLES_COUNT> oldX;
    static std::array<float, TOTAL_PARTICLES_COUNT> oldY;

    static std::array<float, TOTAL_PARTICLES_COUNT> accX;
    static std::array<float, TOTAL_PARTICLES_COUNT> accY;

    static std::array<float, TOTAL_PARTICLES_COUNT> rad;
    static std::array<Color, TOTAL_PARTICLES_COUNT> color;

    // inline static float radius = MAX_PARTICLE_RADIUS;
};

#endif