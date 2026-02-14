#ifndef SOLVER_H
#define SOLVER_H

#include <array>
#include <cmath>
#include <algorithm>
#include "defines.h"
#include "particle.cpp"

class Solver
{
public:
    static Particle particles;

    static std::array<int, GRID_WIDTH * GRID_HEIGHT> gridHeads;
    static std::array<int, TOTAL_PARTICLE_COUNT> gridNext;

    inline static uint32_t currentParticles = 0;

    inline static int dx[9] = {0, -1, 1, 0, 0, -1, -1, 1, 1};
    inline static int dy[9] = {0, 0, 0, -1, 1, -1, 1, -1, 1};

public:
    static void generateParticles(const int count);
    static void updateSimulationState(const float dt);

    static void mousePush(const Vector2 &pos, const float radius);
    static void mousePull(const Vector2 &pos, const float radius);

private:
    static void updateParticlesPositions(const float dt);
    static void updateParticlesGrid();
    static void checkParticlesCollisions();
    static void resolveParticlesCollisions(const int p1, const int p2);
    static void resolveParticlesBoundaryCollisions(const int i);
};

#endif