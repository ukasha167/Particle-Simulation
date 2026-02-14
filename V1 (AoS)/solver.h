#ifndef SOLVER_H
#define SOLVER_H

#include <vector>
#include "defines.h"
#include "particle.cpp"

class Solver
{
private:
public:
    inline static uint16_t currentParticles = 0;
    inline static int dx[9] = {0, -1, 1, 0, 0, -1, -1, 1, 1};
    inline static int dy[9] = {0, 0, 0, -1, 1, -1, 1, -1, 1};
    static Particle particles[TOTAL_PARTICLE_COUNT];
    static std::vector<uint16_t> grid[GRID_WIDTH][GRID_HEIGHT];

private:
public:
    static void generateParticles();
    static void updateSimulationState(const float dt);

    static void updateParticlesPositions(const float &dt);
    static void updateParticlesGrid();

    static void checkParticlesCollisions(const float &dt);

    static void resolveParticlesCollisions(Particle &p1, Particle &p2);
    static void resolveParticlesBoundaryCollisions(Particle &p, const float &dt);
};

#endif