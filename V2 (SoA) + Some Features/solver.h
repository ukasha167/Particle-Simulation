#ifndef SOLVER_H
#define SOLVER_H

#include <vector>
#include <iostream>

#include "defines.h"
#include "particle.cpp"

struct Solver
{
public:
    static Particle particles;

    static std::array<int, GRID_WIDTH * GRID_HEIGHT> gridHeads;
    static std::array<int, TOTAL_PARTICLE_COUNT> gridNext;

    inline static constexpr int dx[9] = {0, -1, 1, 0, 0, -1, -1, 1, 1};
    inline static constexpr int dy[9] = {0, 0, 0, -1, 1, -1, 1, -1, 1};

    inline static uint16_t currentParticles = 0;

public:
    static void generateParticles();
    static void generateParticles(const float &posX, const float &posY);
    static void updateSimulationState(const float &dt);

    static void updateParticlesPositions(const float &dt);
    static void updateParticlesGrid();

    static void checkParticlesCollisions(const float &dt);

    static void resolveParticlesCollisions(const uint16_t &p1, const uint16_t &p2);
    static void resolveParticlesBoundaryCollisions(const int &i, const float &dt);

    static void mousePull(const Vector2 &pos, const uint16_t radius);
    static void mousePush(const Vector2 &pos, const uint16_t radius);

    static void accelerateParticle(const Vector2 &accelerate, const uint16_t &i);
};

#endif