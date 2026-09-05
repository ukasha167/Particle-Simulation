#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <raylib.h>

#include "defines.hpp"
#include "particle.hpp"

class Renderer
{
public:
    static Texture2D sprite;

public:
    static uint8_t loadAsset(const char *imageName, const uint8_t diameter);
    static void drawParticles(const Particle &particles, const uint32_t currentParticlesCount);
    static void drawStats(const uint16_t fps, const uint32_t currentParticlesCount);
    static void unloadAsset();
};

#endif
