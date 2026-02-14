#ifndef SOLVER_H
#define SOLVER_H

#include <iostream>
#include <algorithm>
#include <array>
#include <cmath>

#include "defines.h"
#include "particle.cpp"

class Solver
{
public:
    static Particle particles;

    static std::array<int16_t, GRID_WIDTH * GRID_HEIGHT> gridStart;     // SIGNED BECAUSE IT CAN STORE NEGATIVE VALUES
    static std::array<uint16_t, TOTAL_PARTICLES_COUNT> particleIndices; // UN_SIGNED BECAUSE IT CAN NOT STORE NEGATIVE VALUES AND TO GET MORE SPACE
    static std::array<uint16_t, TOTAL_PARTICLES_COUNT> particleCellIDs; // UN_SIGNED BECAUSE IT CAN NOT STORE NEGATIVE VALUES AND TO GET MORE SPACE

    inline static uint32_t currentParticlesCount = 0;
    inline static uint8_t subSteps = 8;

    // PARTICLES STARTING PARAMETERS
    inline static float initVelocityX = 0.0f;
    inline static float initVelocityY = 0.0f;
    inline static float initSpread = 0.0f;

public:
    static void setParticlesInitialValues();
    static void generateParticles(const uint16_t count);
    static void updateSimulationState(const float dt);

    static void mousePush(const Vector2 &pos, const float radius);
    static void mousePull(const Vector2 &pos, const float radius);

private:
    static void updateParticlesPositions(const float &dt);
    static void updateParticlesGrid();
    static void checkParticlesCollisions();
    static void resolveParticlesCollisions(const uint16_t &p1, const uint16_t &p2);
    static void resolveBoundaryCollisions();
};

#endif