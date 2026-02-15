#ifndef RENDERER_H
#define RENDERER_H

#include <raylib.h>
#include <array>

#include "defines.h"
#include "particle.cpp"

class Renderer
{
public:
    static Texture2D sprite;

public:
    static uint8_t loadAsset(const char *imageName, const uint8_t diameter);
    static void drawParticles(const Particle &particles, const uint16_t currentParticlesCount);
    static void drawStats(const uint16_t fps, const uint16_t currentParticlesCount);
    static void unloadAsset();
};

#endif