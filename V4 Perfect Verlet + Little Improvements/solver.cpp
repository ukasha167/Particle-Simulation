#include "solver.h"

// ---------------------------   Storage   ---------------------------

std::array<int16_t, GRID_WIDTH * GRID_HEIGHT> Solver::gridStart;
std::array<uint16_t, TOTAL_PARTICLES_COUNT> Solver::particleIndices;
std::array<uint16_t, TOTAL_PARTICLES_COUNT> Solver::particleCellIDs;

Particle Solver::particles;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::posX;
std::array<float, TOTAL_PARTICLES_COUNT> Particle::posY;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::oldX;
std::array<float, TOTAL_PARTICLES_COUNT> Particle::oldY;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::accX;
std::array<float, TOTAL_PARTICLES_COUNT> Particle::accY;

// ---------------------------   Public Functions   ---------------------------

void Solver::setParticlesInitialValues()
{
    if (GENERATE_PARTICLES_PER_SECOND > 1)
    {
        initVelocityX = -3.0f * (GENERATE_PARTICLES_PER_SECOND * 0.5);
        initVelocityY = 1.5f * (GENERATE_PARTICLES_PER_SECOND * 0.5);
        initSpread = 15.0f * (GENERATE_PARTICLES_PER_SECOND * 0.5);
    }
    else
    {
        initVelocityX = -2.0f;
        initVelocityY = 0.0f;
        initSpread = 0.0f;
    }
}

void Solver::generateParticles(const uint16_t count)
{
    if (currentParticlesCount >= TOTAL_PARTICLES_COUNT)
    {
        return;
    }

    // MATH TRICK FOR BRANCHLESS PROGRAMMING (NO LONGER USEFUL) :(
    // const uint16_t available = TOTAL_PARTICLES_COUNT - currentParticlesCount;
    // uint16_t spawnLimit = available * (available < count) + count * (available >= count);

    // BETTER VECTORIZATION
    uint16_t spawnLimit = std::min(static_cast<int>(count), static_cast<int>(TOTAL_PARTICLES_COUNT - currentParticlesCount));

    for (uint16_t i = 0; i < spawnLimit; i++)
    {
        // FOR JITTERED START

        // SUBTRACTING 1.0 TO GET RANGE OF - 1.0 to 1.0
        // float r1 = ((rand() % 1000) / 500.0f) - 1.0f;
        // float r2 = ((rand() % 1000) / 500.0f) - 1.0f;

        // INITAL POSITIONS
        // particles.posX[currentParticlesCount] = (SCREEN_WIDTH - 30.0f) + (r1 * initSpread);
        // particles.posY[currentParticlesCount] = 30.0f + (r2 * initSpread);

        // // IMPLICITLY ADDING VELOCITY ALONG A DIRECTION
        // particles.oldX[currentParticlesCount] = particles.posX[currentParticlesCount] - initVelocityX;
        // particles.oldY[currentParticlesCount] = particles.posY[currentParticlesCount] - initVelocityY;

        // FOR NON-JITTERED START (UNCOMMENT BELOW)

        // INITAL POSITIONS
        particles.posX[currentParticlesCount] = (SCREEN_WIDTH - 30.0f);
        particles.posY[currentParticlesCount] = 30.0f;

        // // IMPLICITLY ADDING VELOCITY ALONG A DIRECTION
        particles.oldX[currentParticlesCount] = particles.posX[currentParticlesCount] - initVelocityX;
        particles.oldY[currentParticlesCount] = particles.posY[currentParticlesCount] - initVelocityY;

        // SETING THE ACCELERATING TO ZERO
        particles.accX[currentParticlesCount] = 0.0f;
        particles.accY[currentParticlesCount] = 0.0f;

        currentParticlesCount++;
    }
}

void Solver::updateSimulationState(const float dt)
{
    // DIVIDING ONE ITERATION INTO SEVERAL SMALLER ITERATIONS | TO INCREASE ACCURACY
    const float subDt = dt / static_cast<float>(subSteps);

    // THE SEQUENCE MATTERS HERE
    for (uint8_t s = 0; s < subSteps; s++)
    {
        // WE FIRST MOVE THE PARTICLES
        Solver::updateParticlesPositions(subDt);

        // CHECK IF ONE GOES OUT OF BOUNDS
        Solver::resolveBoundaryCollisions();

        // MAP IT TO THE GRID
        Solver::updateParticlesGrid();

        // AT THE VERY LAST, CHECK FOR THE COLLISIONS
        Solver::checkParticlesCollisions();
    }
}

void Solver::mousePush(const Vector2 &pos, const float radius)
{
    // for (int i = 0; i < currentParticlesCount; i++)
    // {
    //     Vector2 displacement = pos - particles.currentPositions[i];
    //     float distance = (displacement.x * displacement.x + displacement.y * displacement.y); // Normally with SQRT()

    //     if (distance < radius * radius)
    //     {
    //         distance = std::sqrt(distance);

    //         // Add Acceleration to the particles inside the given radius. move particles away from the mouse
    //         // Closer particles move more, farther particles experience less force
    //     }
    // }
}

void Solver::mousePull(const Vector2 &pos, const float radius)
{
    // for (int i = 0; i < currentParticlesCount; i++)
    // {
    //     Vector2 displacement = pos - particles.currentPositions[i];
    //     float distance = (displacement.x * displacement.x + displacement.y * displacement.y); // Normally with SQRT()

    //     if (distance < radius * radius)
    //     {
    //         distance = std::sqrt(distance);

    //         // Add Acceleration to the particles inside the given radius. Move particles closer to the mouse
    //         // Closer particles move more, farther particles experience less force
    //     }
    // }
}

// ---------------------------   Private Functions   ---------------------------

void Solver::updateParticlesPositions(const float &dt)
{
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // FIND THE DISPLACEMENT
        float dx = particles.posX[i] - particles.oldX[i];
        float dy = particles.posY[i] - particles.oldY[i];

        // ADD THE CONSTRAINTS (FORCES)
        particles.accY[i] += GRAVITY;

        // UPDATE THE OLD POSITION
        particles.oldX[i] = particles.posX[i];
        particles.oldY[i] = particles.posY[i];

        // UPDATE THE CURRENT POSITIOn
        particles.posX[i] = particles.posX[i] + dx + (particles.accX[i] * (dt * dt));
        particles.posY[i] = particles.posY[i] + dy + (particles.accY[i] * (dt * dt));

        // RESET THE CONSTRAINTS (FORCES)
        particles.accX[i] = 0.0f;
        particles.accY[i] = 0.0f;
    }
}

void Solver::updateParticlesGrid()
{
    // FILL THE GRID WITH -1 VALUE (DEFAULT)
    std::fill(gridStart.begin(), gridStart.end(), -1);

    // LOOP THROUGH THE PARTICLES
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // FIND WHERE THEY LIE IN 2D WORLD
        uint16_t x = particles.posX[i] / CELL_SIZE;
        uint16_t y = particles.posY[i] / CELL_SIZE;

        // CLAMP THE PARTICLE TO THE BOUNDARIES IF GOES OUT OF BOUND
        x = std::clamp(static_cast<int>(x), 0, static_cast<int>(GRID_WIDTH - 1));
        y = std::clamp(static_cast<int>(y), 0, static_cast<int>(GRID_HEIGHT - 1));

        // MAP TO 1D ARRAY
        uint16_t cellIndex = y * GRID_WIDTH + x;

        // STORES THE CELL_INDEX THE PARTICLE'S INDEX
        particleCellIDs[i] = cellIndex;
        particleIndices[i] = i;
    }

    // SORT THE ARRAY (USES 2-Pivots Quick Sort) | DOES NOT HURT PERFORMANCE IF THE ARRAY IS ALREADY (MOSTLY) SORTED
    std::sort(particleIndices.begin(), particleIndices.begin() + currentParticlesCount,
              [](uint16_t a, uint16_t b)
              {
                  return particleCellIDs[a] < particleCellIDs[b];
              });

    // AGAIN LOOP THROUGH
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // FIND THE CELL_ID AGAIN | BECAUSE WE SORTED :)
        uint16_t cellId = particleCellIDs[particleIndices[i]];

        // STORES i INTO THE gridStart[cellId] (IF IT IS NOT -1) | TO FIND FROM WHERE A CELL IS STARTING
        if (gridStart[cellId] == -1)
        {
            gridStart[cellId] = i;
        }
    }
}

void Solver::checkParticlesCollisions()
{
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // GET THE INFORMATION OF CURRENT PARTICLE
        uint16_t p1 = particleIndices[i];
        uint16_t cellIndex = particleCellIDs[p1];

        // FIND THE CELL COORDINATE
        uint16_t cellX = cellIndex % GRID_WIDTH;
        uint16_t cellY = cellIndex / GRID_WIDTH;

        // LOOP CHECK THE NEIGHBOURS (INCLUDING SELF)
        for (int8_t dx = -1; dx <= 1; dx++)
        {
            for (int8_t dy = -1; dy <= 1; dy++)
            {
                // FIND THE NEIGHBOUR'S 2D COORDINATES
                int16_t nX = cellX + dx;
                int16_t nY = cellY + dy;

                // SKIP IF OUT OF BOUNDS
                if (nX < 0 || nX >= GRID_WIDTH || nY < 0 || nY >= GRID_HEIGHT)
                {
                    continue;
                }

                // NEIGHBOUR CELL NUMBER ON 1D ARRAY
                int16_t neighborCell = nY * GRID_WIDTH + nX;

                // FIND THE STARTING POINT ALL THE NEIGHBOURING PARTICLES THAT ARE INSIDE IT
                int16_t k = gridStart[neighborCell];

                // -1 MEANS NO PARTICLE IS INSIDE
                if (k == -1)
                {
                    continue;
                }

                // LOOP THROUGH ALL NEIGHBOURS | WE ARE USING SORTED ARRAY, SO NEIGHBOURS ARE LOCATED SIDE BY SIDE
                while (k < currentParticlesCount)
                {
                    uint16_t p2 = particleIndices[k]; // GET THE INDEX OF ONE NEIGHBOURING PARTICLE

                    // BASE CONDITION: IF THE PARTICLE DOES NOT MATCHES THE CELL, WE KNOW THE CELL'S BLOCK IS FINISHED
                    if (particleCellIDs[p2] != neighborCell)
                    {
                        break;
                    }

                    // ONLY CHECK THE PARTICLES ONCE, AND DON'T CHECK THE PARTICLE WITH ITSELF
                    if (p1 < p2)
                    {
                        resolveParticlesCollisions(p1, p2);
                    }

                    // MOVE TO THE NEXT PARTICLE
                    k++;
                }
            }
        }
    }
}

void Solver::resolveParticlesCollisions(const uint16_t &p1, const uint16_t &p2)
{
    // FIND DISPLACEMENt
    float dx = particles.posX[p1] - particles.posX[p2];
    float dy = particles.posY[p1] - particles.posY[p2];

    // DISTANCE BETWEEN TWO PARTICLES
    float distSq = dx * dx + dy * dy;

    // PARTICLES SHOULD BE SEPERATED BY THE SUM OF THEIR RADII
    float minDst = particles.radius + particles.radius;

    if (distSq < (minDst * minDst) && distSq > 0.0001f)
    {
        // COMPLETE THE FORMULA
        float dist = std::sqrt(distSq);

        // NORMALIZE
        float nX = dx / dist;
        float nY = dy / dist;

        // PENETERATION DEPTH FROM EITHER SIDE
        float delta = 0.5f * (minDst - dist);

        // POSITION CORRECTION
        float moveX = nX * delta;
        float moveY = nY * delta;

        // PUSH APART
        particles.posX[p1] += moveX;
        particles.posY[p1] += moveY;

        particles.posX[p2] -= moveX;
        particles.posY[p2] -= moveY;

        // ADD (ENOUGH) ENERGY LOSS ALONGSIDE THE NORMALISED VECTOR
        particles.oldX[p1] += moveX * COLLISION_REACTION_LOSS;
        particles.oldY[p1] += moveY * COLLISION_REACTION_LOSS;

        particles.oldX[p2] -= moveX * COLLISION_REACTION_LOSS;
        particles.oldY[p2] -= moveY * COLLISION_REACTION_LOSS;
    }
}

void Solver::resolveBoundaryCollisions()
{
    for (size_t i = 0; i < currentParticlesCount; i++)
    {
        float dx = particles.posX[i] - particles.oldX[i];
        float dy = particles.posY[i] - particles.oldY[i];

        // GROUND - FLOOR
        if (particles.posY[i] > SCREEN_HEIGHT - particles.radius)
        {
            particles.posY[i] = SCREEN_HEIGHT - particles.radius;

            dx *= FRICTION;
            dy *= -DAMPENING;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }
        // TOP - CEILING
        else if (particles.posY[i] < particles.radius)
        {
            particles.posY[i] = particles.radius;

            dx *= FRICTION;
            dy *= -DAMPENING;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }

        // RIGHT - WALL
        if (particles.posX[i] > SCREEN_WIDTH - particles.radius)
        {
            particles.posX[i] = SCREEN_WIDTH - particles.radius;

            dx *= -DAMPENING;
            dy *= FRICTION;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }
        // LEFT - WALL
        else if (particles.posX[i] < particles.radius)
        {
            particles.posX[i] = particles.radius;

            dx *= -DAMPENING;
            dy *= FRICTION;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }
    }
}