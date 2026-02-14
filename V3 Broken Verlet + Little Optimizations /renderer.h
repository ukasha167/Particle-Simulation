#ifndef RENDERER_H
#define RENDERER_H

#include <array>

#include "defines.h"
#include "particle.cpp"

class Renderer
{
public:
    static Texture2D sprite;

public:
    static void loadAsset(const char *imageName, const int diameter);
    static void drawParticles(const std::array<Vector2, TOTAL_PARTICLE_COUNT> &positions, const int &current);
    static void drawStats(const int fps, const int &currentCount);
    static void unloadAsset();
};

#endif