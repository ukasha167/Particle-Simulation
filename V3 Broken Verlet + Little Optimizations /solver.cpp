#include "solver.h"

// -------------------------------- STORAGE --------------------------------

Particle Solver::particles;

std::array<Vector2, TOTAL_PARTICLE_COUNT> Particle::currentPositions;
std::array<Vector2, TOTAL_PARTICLE_COUNT> Particle::oldPositions;
std::array<Vector2, TOTAL_PARTICLE_COUNT> Particle::accelerations;

std::array<int, GRID_WIDTH * GRID_HEIGHT> Solver::gridHeads;
std::array<int, TOTAL_PARTICLE_COUNT> Solver::gridNext;

// -------------------------------- PUBLIC FUNCTIONS --------------------------------

void Solver::updateSimulationState(const float dt)
{
    const int subSteps = 8;
    const float sub_dt = dt / (float)subSteps;
    for (int i = 0; i < subSteps; i++)
    {
        Solver::updateParticlesPositions(sub_dt);
        Solver::updateParticlesGrid();
        Solver::checkParticlesCollisions();
        for (uint32_t j = 0; j < Solver::currentParticles; j++)
        {
            Solver::resolveParticlesBoundaryCollisions(j);
        }
    }
}

void Solver::generateParticles(const int count)
{
    for (int k = 0; k < count; k++)
    {
        if (currentParticles >= TOTAL_PARTICLE_COUNT)
            return;
        float x = SCREEN_WIDTH - 50.0f;
        float y = 50.0f;
        particles.currentPositions[currentParticles] = {x, y};
        Vector2 initialVelocity = {-1.5f, 0.0f};
        particles.oldPositions[currentParticles] = {x - initialVelocity.x, y - initialVelocity.y};
        particles.accelerations[currentParticles] = {0.0f, 0.0f};
        currentParticles++;
    }
}

// -------------------------------- PRIVATE FUNCTIONS --------------------------------

void Solver::updateParticlesPositions(const float dt)
{
    for (uint32_t i = 0; i < Solver::currentParticles; i++)
    {
        Vector2 velocity = particles.currentPositions[i] - particles.oldPositions[i];
        velocity.x *= 0.999f;
        velocity.y *= 0.999f;

        const float maxSpeed = 500.0f; // tune to your needs
        float vmagSq = velocity.x * velocity.x + velocity.y * velocity.y;
        if (vmagSq > maxSpeed * maxSpeed)
        {
            float inv = 1.0f / std::sqrt(vmagSq);
            velocity.x = velocity.x * inv * maxSpeed;
            velocity.y = velocity.y * inv * maxSpeed;
        }

        particles.oldPositions[i] = particles.currentPositions[i];
        particles.accelerations[i].y += GRAVITY;

        Vector2 accelStep = particles.accelerations[i] * (dt * dt);
        particles.currentPositions[i] = particles.currentPositions[i] + velocity + accelStep;
        particles.accelerations[i] = {0.0f, 0.0f};
    }
}

void Solver::resolveParticlesBoundaryCollisions(const int i)
{
    Vector2 &pos = particles.currentPositions[i];
    Vector2 &oldPos = particles.oldPositions[i];
    float r = particles.radius;

    Vector2 vel = pos - oldPos;

    const float restitution = 0.3f; // bounce
    const float friction = 0.5f;    // tangential damping

    // Floor (bottom)
    if (pos.y > SCREEN_HEIGHT - r)
    {
        pos.y = SCREEN_HEIGHT - r;
        // reflect vertical component
        vel.y = -vel.y * restitution;
        // apply friction to x
        vel.x = vel.x * friction;
        // reconstruct oldPos so vel = pos - oldPos
        oldPos = pos - vel;
    }
    // Ceiling (top)
    else if (pos.y < r)
    {
        pos.y = r;
        vel.y = -vel.y * restitution;
        vel.x = vel.x * friction;
        oldPos = pos - vel;
    }

    // Left wall
    if (pos.x < r)
    {
        pos.x = r;
        vel.x = -vel.x * restitution;
        vel.y = vel.y * friction;
        oldPos = pos - vel;
    }
    // Right wall
    else if (pos.x > SCREEN_WIDTH - r)
    {
        pos.x = SCREEN_WIDTH - r;
        vel.x = -vel.x * restitution;
        vel.y = vel.y * friction;
        oldPos = pos - vel;
    }
}

void Solver::checkParticlesCollisions()
{
    for (int cellX = 0; cellX < GRID_WIDTH; cellX++)
    {
        for (int cellY = 0; cellY < GRID_HEIGHT; cellY++)
        {
            int cellIndex = cellY * GRID_WIDTH + cellX;
            int p1 = gridHeads[cellIndex];
            while (p1 != -1)
            {
                for (int k = 0; k < 9; ++k)
                {
                    int neighborX = cellX + dx[k];
                    int neighborY = cellY + dy[k];
                    if (neighborX < 0 || neighborY < 0 || neighborX >= GRID_WIDTH || neighborY >= GRID_HEIGHT)
                        continue;
                    int neighborIndex = neighborY * GRID_WIDTH + neighborX;
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

void Solver::resolveParticlesCollisions(const int p1, const int p2)
{
    Vector2 &pos1 = particles.currentPositions[p1];
    Vector2 &pos2 = particles.currentPositions[p2];
    Vector2 &old1 = particles.oldPositions[p1];
    Vector2 &old2 = particles.oldPositions[p2];

    Vector2 delta = pos1 - pos2;
    float distSq = delta.x * delta.x + delta.y * delta.y;
    float minDist = particles.radius * 2.0f;

    if (distSq < (minDist * minDist) && distSq > 1e-8f)
    {
        float dist = std::sqrt(distSq);
        Vector2 n = delta * (1.0f / dist); // normal from p2 -> p1
        float penetration = minDist - dist;

        // --- positional correction (split between the two particles)
        Vector2 correction = n * (penetration * 0.5f);
        pos1 = pos1 + correction;
        pos2 = pos2 - correction;

        // --- compute pre-collision velocities (Verlet style)
        Vector2 vel1 = pos1 - old1; // note: pos1 is already corrected, but we still use old1 to compute current vel
        Vector2 vel2 = pos2 - old2;

        // It's safer to compute velocities using positions before correction.
        // So recompute using saved pre-correction positions would be ideal;
        // if you want that, capture pos1_old/pos2_old before correction above.

        // --- relative normal velocity
        float relVel = (vel1.x - vel2.x) * n.x + (vel1.y - vel2.y) * n.y;

        // restitution: 0 = perfectly inelastic, 1 = perfectly elastic
        const float restitution = 0.5f;

        if (relVel < 0.0f)
        {
            // impulse scalar for equal masses:
            float j = (-(1.0f + restitution) * relVel) * 0.5f; // divide by (m1+m2) but equal masses => /2

            Vector2 impulse = n * j;

            // apply impulse to velocities
            vel1 = vel1 + impulse;
            vel2 = vel2 - impulse;
        }

        // --- write back corrected previous positions so velocity = pos - old yields vel1/vel2 above
        old1 = pos1 - vel1;
        old2 = pos2 - vel2;
    }
}

void Solver::updateParticlesGrid()
{
    std::fill(gridHeads.begin(), gridHeads.end(), -1);
    for (uint32_t i = 0; i < currentParticles; i++)
    {
        int cellX = (int)(particles.currentPositions[i].x / CELL_SIZE);
        int cellY = (int)(particles.currentPositions[i].y / CELL_SIZE);
        if (cellX < 0)
            cellX = 0;
        else if (cellX >= GRID_WIDTH)
            cellX = GRID_WIDTH - 1;
        if (cellY < 0)
            cellY = 0;
        else if (cellY >= GRID_HEIGHT)
            cellY = GRID_HEIGHT - 1;
        int cellIndex = cellY * GRID_WIDTH + cellX;
        gridNext[i] = gridHeads[cellIndex];
        gridHeads[cellIndex] = i;
    }
}

void Solver::mousePush(const Vector2 &pos, const float radius)
{
    float radiusSq = radius * radius;
    for (uint32_t i = 0; i < currentParticles; i++)
    {
        Vector2 diff = particles.currentPositions[i] - pos;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        if (distSq < radiusSq && distSq > 0.0001f)
        {
            float dist = std::sqrt(distSq);
            Vector2 dir = diff * (1.0f / dist);
            float forceStrength = (radius - dist) / radius;
            particles.accelerations[i] = particles.accelerations[i] + (dir * forceStrength * 600.0f);
        }
    }
}

void Solver::mousePull(const Vector2 &pos, const float radius)
{
    float radiusSq = radius * radius;
    for (uint32_t i = 0; i < currentParticles; i++)
    {
        Vector2 diff = particles.currentPositions[i] - pos;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        if (distSq < radiusSq && distSq > 0.0001f)
        {
            float dist = std::sqrt(distSq);
            Vector2 dir = diff * (1.0f / dist);
            float forceStrength = (radius - dist) / radius;
            particles.accelerations[i] = particles.accelerations[i] + (dir * forceStrength * -600.0f);
        }
    }
}