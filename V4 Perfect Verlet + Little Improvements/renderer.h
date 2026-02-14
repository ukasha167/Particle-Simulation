#ifndef RENDERER_H
#define RENDERER_H

#include <raylib.h>
#include <array>

#include "defines.h"
#include "particle.cpp"

class Renderer
{
    // TO SIMPLIFY THIS MONSTEROSITY OF A LINE
    using stdArray = std::array<float, TOTAL_PARTICLES_COUNT>;

public:
    static Texture2D sprite;

public:
    static void loadAsset(const char *imageName, const uint8_t &diameter);
    static void drawParticles(const stdArray &posX, const stdArray &posY, const uint16_t &currentParticlesCount);
    static void drawStats(const uint16_t fps, const uint16_t &currentParticlesCount);
    static void unloadAsset();
};

#endif