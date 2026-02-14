#include "solver.h"

// --------------------------------      STORAGE      --------------------------------

Particle Solver::particles[TOTAL_PARTICLE_COUNT];
std::vector<uint16_t> Solver::grid[GRID_WIDTH][GRID_HEIGHT];

// --------------------------------  PUBLIC FUNCTIONS  --------------------------------

void Solver::updateSimulationState(float dt)
{
    const int sub_steps = 10;
    const float sub_dt = dt / (float)sub_steps;

    for (int i = 0; i < sub_steps; i++)
    {
        Solver::updateParticlesPositions(sub_dt);
        Solver::updateParticlesGrid();
        Solver::checkParticlesCollisions(sub_dt);

        for (int j = 0; j < Solver::currentParticles; j++)
        {
            Solver::resolveParticlesBoundaryCollisions(Solver::particles[j], sub_dt);
        }
    }
}

void Solver::generateParticles()
{
    if (currentParticles >= TOTAL_PARTICLE_COUNT)
    {
        return;
    }

    currentParticles++;
}

void Solver::updateParticlesPositions(const float &dt)
{
    for (int i = 0; i < Solver::currentParticles; i++)
    {
        particles[i].updatePosition(dt);
    }
}

void Solver::checkParticlesCollisions(const float &dt)
{
    uint16_t cellX, cellY, k, neighborX, neighborY;

    // Iterate over every cell in the grid
    for (cellX = 0; cellX < GRID_WIDTH; ++cellX)
    {
        for (cellY = 0; cellY < GRID_HEIGHT; ++cellY)
        {
            // For the current cell [cellX, cellY]
            const std::vector<uint16_t> &current_cell = Solver::grid[cellX][cellY];

            // Iterate over the 9 relevant cells
            for (k = 0; k < 9; ++k)
            {
                neighborX = cellX + Solver::dx[k];
                neighborY = cellY + Solver::dy[k];

                // Skip invalid neighbors
                if (neighborX < 0 || neighborY < 0 || neighborX >= GRID_WIDTH || neighborY >= GRID_HEIGHT)
                    continue;

                const auto &neighbor_cell = Solver::grid[neighborX][neighborY];

                for (const uint16_t &p1_index : current_cell)
                {
                    for (const uint16_t &p2_index : neighbor_cell)
                    {
                        // Avoid duplicate checks for same cell
                        if (p1_index >= p2_index)
                            continue;

                        Particle &p1 = Solver::particles[p1_index];
                        Particle &p2 = Solver::particles[p2_index];

                        Solver::resolveParticlesCollisions(p1, p2);
                    }
                }
            }
        }
    }
}

void Solver::resolveParticlesBoundaryCollisions(Particle &p, const float &dt)
{
    if (p.position.x + p.radius >= SCREEN_WIDTH)
    {
        p.position.x = SCREEN_WIDTH - p.radius;
        p.velocity.x *= -DAMPENING;
    }
    else if (p.position.x - p.radius < 0)
    {
        p.position.x = p.radius;
        p.velocity.x *= -DAMPENING;
    }

    if (p.position.y + p.radius >= SCREEN_HEIGHT)
    {
        p.position.y = SCREEN_HEIGHT - p.radius;
        p.velocity.y *= -DAMPENING;
        // p.velocity.x *= FRICTION;
        p.velocity.x -= p.velocity.x * FRICTION * dt;
    }
    if (p.position.y - p.radius < 0)
    {
        p.position.y = p.radius;
        p.velocity.y *= -DAMPENING;
    }
}

void Solver::resolveParticlesCollisions(Particle &p1, Particle &p2)
{
    float dx = p2.position.x - p1.position.x;
    float dy = p2.position.y - p1.position.y;

    float distanceSq = (dx * dx) + (dy * dy);
    float radii_sum = p1.radius + p2.radius;

    if (distanceSq < radii_sum * radii_sum)
    {
        float distance = std::sqrt(distanceSq);

        if (distance == 0.0f)
            distance = 0.0001f;

        // Normalized Collision Vector (n)
        float nx = dx / distance;
        float ny = dy / distance;

        // SEPARATION (Unstick the particles)
        float overlap = radii_sum - distance;

        // Move each particle apart by half the overlap
        p1.position.x -= 0.5f * overlap * nx;
        p1.position.y -= 0.5f * overlap * ny;
        p2.position.x += 0.5f * overlap * nx;
        p2.position.y += 0.5f * overlap * ny;

        // VELOCITY RESPONSE
        // Calculate relative velocity
        // The previous logic was complex; here is the standard impulse-like swap for equal mass
        float k = (p2.velocity.x - p1.velocity.x) * nx + (p2.velocity.y - p1.velocity.y) * ny;

        // Apply changes
        float impulseX = k * nx * DAMPENING;
        float impulseY = k * ny * DAMPENING;

        p1.velocity.x += impulseX;
        p1.velocity.y += impulseY;
        p2.velocity.x -= impulseX;
        p2.velocity.y -= impulseY;
    }
}

void Solver::updateParticlesGrid()
{
    uint16_t i, j, x, y;
    for (i = 0; i < GRID_WIDTH; ++i)
    {
        for (j = 0; j < GRID_HEIGHT; ++j)
        {
            grid[i][j].clear();
        }
    }

    for (i = 0; i < currentParticles; i++)
    {
        x = particles[i].position.x / CELL_SIZE;
        y = particles[i].position.y / CELL_SIZE;

        if (x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT)
        {
            grid[x][y].push_back(i);
        }
    }
}