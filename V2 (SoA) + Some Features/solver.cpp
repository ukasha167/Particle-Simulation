#include "solver.h"

// --------------------------------      STORAGE      --------------------------------
Particle Solver::particles;

std::array<float, TOTAL_PARTICLE_COUNT> Particle::posX;
std::array<float, TOTAL_PARTICLE_COUNT> Particle::posY;
std::array<float, TOTAL_PARTICLE_COUNT> Particle::velX;
std::array<float, TOTAL_PARTICLE_COUNT> Particle::velY;

std::array<int, GRID_WIDTH * GRID_HEIGHT> Solver::gridHeads;
std::array<int, TOTAL_PARTICLE_COUNT> Solver::gridNext;

// --------------------------------     FUNCTIONS     --------------------------------

void Solver::updateSimulationState(const float &dt)
{
    const uint8_t sub_steps = 8;
    const float sub_dt = dt / (float)sub_steps;

    for (uint8_t i = 0; i < sub_steps; i++)
    {
        Solver::updateParticlesPositions(sub_dt);
        Solver::updateParticlesGrid();
        Solver::checkParticlesCollisions(sub_dt);

        for (uint16_t j = 0; j < Solver::currentParticles; j++)
        {
            Solver::resolveParticlesBoundaryCollisions(j, sub_dt);
        }
    }
}

void Solver::generateParticles()
{
    // if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    //     Solver::generateParticles(GetMouseX(), GetMouseY());
    // else // RENDER IN THE MIDDLE OF THE SCREEN IF NO ARGUMENTS GIVEN
    Solver::generateParticles(HALF_SCREEN_WIDTH, HALF_SCREEN_HEIGHT);
}

void Solver::generateParticles(const float &posX, const float &posY)
{
    if (currentParticles >= TOTAL_PARTICLE_COUNT)
        return;

    particles.posX[currentParticles] = posX;
    particles.posY[currentParticles] = posY;
    particles.velX[currentParticles] = 0.1f;
    particles.velY[currentParticles] = 50.0f;

    currentParticles++;
}

void Solver::updateParticlesPositions(const float &dt)
{
    for (uint16_t i = 0; i < Solver::currentParticles; i++)
    {
        particles.velY[i] += GRAVITY * dt;

        if (particles.velX[i] < 0.1f && particles.velX[i] > -0.1f)
            particles.velX[i] = 0.0f;
        if (particles.velY[i] < 0.1f && particles.velY[i] > -0.1f)
            particles.velY[i] = 0.0f;

        particles.posX[i] += particles.velX[i] * dt;
        particles.posY[i] += particles.velY[i] * dt;
    }
}

void Solver::checkParticlesCollisions(const float &dt)
{
    for (uint16_t cellX = 0; cellX < GRID_WIDTH; ++cellX)
    {
        for (uint16_t cellY = 0; cellY < GRID_HEIGHT; ++cellY)
        {
            uint16_t cellIndex = cellY * GRID_WIDTH + cellX;
            int p1 = gridHeads[cellIndex];

            while (p1 != -1)
            {
                for (uint8_t k = 0; k < 9; ++k)
                {
                    uint16_t neighborX = cellX + dx[k];
                    uint16_t neighborY = cellY + dy[k];

                    if (neighborX >= GRID_WIDTH || neighborY >= GRID_HEIGHT)
                        continue;

                    uint16_t neighborIndex = neighborY * GRID_WIDTH + neighborX;
                    int p2 = gridHeads[neighborIndex];

                    while (p2 != -1)
                    {
                        if (p1 < p2)
                        {
                            Solver::resolveParticlesCollisions(p1, p2);
                        }
                        p2 = gridNext[p2];
                    }
                }
                p1 = gridNext[p1];
            }
        }
    }
}

void Solver::resolveParticlesBoundaryCollisions(const int &i, const float &dt)
{
    if (particles.posX[i] + particles.radius >= SCREEN_WIDTH)
    {
        particles.posX[i] = SCREEN_WIDTH - particles.radius;
        particles.velX[i] *= -DAMPENING;
    }
    else if (particles.posX[i] - particles.radius < 0)
    {
        particles.posX[i] = particles.radius;
        particles.velX[i] *= -DAMPENING;
    }

    if (particles.posY[i] + particles.radius >= SCREEN_HEIGHT)
    {
        particles.posY[i] = SCREEN_HEIGHT - particles.radius;
        particles.velY[i] *= -DAMPENING;
        particles.velX[i] -= particles.velX[i] * FRICTION * dt;
    }
    if (particles.posY[i] - particles.radius < 0)
    {
        particles.posY[i] = particles.radius;
        particles.velY[i] *= -DAMPENING;
    }
}

void Solver::resolveParticlesCollisions(const uint16_t &p1, const uint16_t &p2)
{
    float dx = particles.posX[p2] - particles.posX[p1];
    float dy = particles.posY[p2] - particles.posY[p1];

    float distanceSq = (dx * dx) + (dy * dy);
    float radii_sum = particles.radius * 2;

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
        particles.posX[p1] -= 0.5f * overlap * nx;
        particles.posY[p1] -= 0.5f * overlap * ny;

        particles.posX[p2] += 0.5f * overlap * nx;
        particles.posY[p2] += 0.5f * overlap * ny;

        // VELOCITY RESPONSE
        float k = (particles.velX[p2] - particles.velX[p1]) * nx + (particles.velY[p2] - particles.velY[p1]) * ny;

        // Apply changes
        float impulseX = k * nx * DAMPENING;
        float impulseY = k * ny * DAMPENING;

        particles.velX[p1] += impulseX;
        particles.velY[p1] += impulseY;
        particles.velX[p2] -= impulseX;
        particles.velY[p2] -= impulseY;
    }
}

void Solver::updateParticlesGrid()
{
    std::fill(gridHeads.begin(), gridHeads.end(), -1);

    for (uint16_t i = 0; i < currentParticles; i++)
    {
        uint16_t cellX = particles.posX[i] / CELL_SIZE;
        uint16_t cellY = particles.posY[i] / CELL_SIZE;

        if (cellX < 0)
            cellX = 0;
        else if (cellX >= GRID_WIDTH)
            cellX = GRID_WIDTH - 1;

        if (cellY < 0)
            cellY = 0;
        else if (cellY >= GRID_HEIGHT)
            cellY = GRID_HEIGHT - 1;

        uint16_t cellIndex = cellY * GRID_WIDTH + cellX;
        gridNext[i] = gridHeads[cellIndex];

        gridHeads[cellIndex] = i;
    }
}

void Solver::mousePush(const Vector2 &pos, const uint16_t radius)
{
    for (uint16_t i = 0; i < currentParticles; i++)
    {
        Vector2 direction = {pos.x - particles.posX[i], pos.y - particles.posY[i]};
        const float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        Solver::accelerateParticle({direction.x / distance * std::min(0.0f, -5 * (radius - distance)), direction.y / distance * std::min(0.0f, -5 * (radius - distance))}, i);
    }
}

void Solver::mousePull(const Vector2 &pos, const uint16_t radius)
{
    for (uint16_t i = 0; i < currentParticles; i++)
    {
        Vector2 direction = {pos.x - particles.posX[i], pos.y - particles.posY[i]};
        const float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        Solver::accelerateParticle({direction.x / distance * std::max(0.0f, 5 * (radius - distance)), direction.y / distance * std::max(0.0f, 5 * (radius - distance))}, i);
    }
}

void Solver::accelerateParticle(const Vector2 &accelerate, const uint16_t &i)
{
    particles.velX[i] += accelerate.x;
    particles.velY[i] += accelerate.y;
}