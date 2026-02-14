#include "solver.h"

// ---------------------------   Storage   ---------------------------

std::array<int16_t, TOTAL_CELLS> Solver::gridStart;
std::array<uint16_t, TOTAL_PARTICLES_COUNT> Solver::particleCellIDs;
std::array<uint16_t, TOTAL_PARTICLES_COUNT> Solver::particleIndices;
std::array<uint16_t, TOTAL_PARTICLES_COUNT> Solver::sortedParticleIndices;

std::array<Color, 256> Solver::rainbowLUT;

Particle Solver::particles;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::posX;
std::array<float, TOTAL_PARTICLES_COUNT> Particle::posY;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::oldX;
std::array<float, TOTAL_PARTICLES_COUNT> Particle::oldY;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::accX;
std::array<float, TOTAL_PARTICLES_COUNT> Particle::accY;

std::array<float, TOTAL_PARTICLES_COUNT> Particle::rad;
std::array<Color, TOTAL_PARTICLES_COUNT> Particle::color;

// ---------------------------   Public Functions   ---------------------------

void Solver::preComputeInitialValues()
{
    defineParticlesInitialParameters();
    computeColorValues();

    for (uint16_t i = 0; i < TOTAL_PARTICLES_COUNT; i++)
    {
        // FOR JITTERED START
        if (GENERATE_PARTICLES_PER_SECOND > 1)
        {
            // SUBTRACTING 1.0 TO GET RANGE OF - 1.0 to 1.0
            float r1 = ((rand() % 1000) / 500.0f) - 1.0f;
            float r2 = ((rand() % 1000) / 500.0f) - 1.0f;

            // INITAL POSITIONS
            particles.posX[i] = (SCREEN_WIDTH - 50.0f) + (r1 * initSpread);
            particles.posY[i] = 50.0f + (r2 * initSpread);

            // IMPLICITLY ADDING VELOCITY ALONG A DIRECTION
            particles.oldX[i] = particles.posX[i] - initVelocityX;
            particles.oldY[i] = particles.posY[i] - initVelocityY;
        }
        else
        {
            // FOR NON-JITTERED START

            // INITAL POSITIONS
            particles.posX[i] = (SCREEN_WIDTH - 50.0f);
            particles.posY[i] = 50.0f;

            // // IMPLICITLY ADDING VELOCITY ALONG A DIRECTION
            particles.oldX[i] = particles.posX[i] - initVelocityX;
            particles.oldY[i] = particles.posY[i] - initVelocityY;
        }

        // SETING THE ACCELERATIONS TO ZERO
        particles.accX[i] = 0.0f;
        particles.accY[i] = 0.0f;

        // SETTING THE PARTICLES RADII
        float t = i * 0.3f;
        float noise = ((rand() % 100) / 100.0f - 0.5f) * 0.2f; // SMALL OFFSET TO MAKE THEM ALIVE, (COMPLETELY OPTIONAL)
        particles.rad[i] = MIN_PARTICLE_RADIUS + (MAX_PARTICLE_RADIUS - MIN_PARTICLE_RADIUS) * (0.5f + 0.5f * sin(t) + noise);

        // PICK ONE VALUE FROM THE LUT
        particles.color[i] = rainbowLUT[i & 255]; // i & 256 IS SIMILAR TO i % 256 BUT FASTER AND SOMETIMES SAFER TOO.
        // HOWEVER IT IS NOT A SUBSTITUE OF %, IT ONLY WORKS WHEN THE DIVISOR IS POWER OF 2 LIKE 4, 8, 16, 32 ...

        // STORE THE PARTICLE INDICES
        particleIndices[i] = i;
    }
}

void Solver::generateParticles(const uint16_t count)
{
    /**
     * WE COULD USE THE BASE CONDITION TO EARLY OUT THE FUNCTION, BUT GIVEN THAT THIS FUNCTION WILL BE EXECUTED DURING MOST OF THE SIMULATION
     * WE CAN SKIP IT. AND THE EFFECT WOULD BE ZERO, IF NOTHING ELSE IN THE CODE BREAKS | HOPEFULY
        if (currentParticlesCount >= TOTAL_PARTICLES_COUNT)
        {
            return;
        }
    */

    // IF WE HAVE RAN OUT OF LIMIT, THE spawnLimit WILL SIMPLY TURN OUT TO BE ZERO
    uint16_t spawnLimit = std::min(static_cast<int>(count), static_cast<int>(TOTAL_PARTICLES_COUNT - currentParticlesCount));

    currentParticlesCount += spawnLimit;
}

void Solver::updateSimulationState(const float dt)
{
    // DIVIDING ONE ITERATION INTO SEVERAL SMALLER ITERATIONS | TO INCREASE THE ACCURACY
    // MOVING PARTICLES A SMALL DISTANCE MULTIPLE TIMES A FRAME IS BETTER THAN MOVING THEM BY A LARGE DISTANCE
    // BECAUSE WHEN THE PARTICLES STACK ON TOP OF EACH OTHER, A SINGLE MASSIVE MOTION BY ONE PARTICLE WILL CAUSE CHAOS

    const float subDt = dt / static_cast<float>(subSteps);

    // THE SEQUENCE MATTERS HERE
    for (uint8_t s = 0; s < subSteps; s++)
    {
        // WE FIRST CALCULATE AND MOVE THE PARTICLES
        Solver::updateParticlesPositions(subDt);

        // CHECK IF ONE GOES OUT OF BOUNDS
        Solver::resolveBoundaryCollisions();

        // MAP IT TO THE GRID
        Solver::updateParticlesGrid();

        // AT THE VERY LAST, CHECK FOR THE COLLISIONS WITH OTHERS
        Solver::checkParticlesCollisions();
    }
}

void Solver::mousePush(const Vector2 &pos, const float radius)
{
    // INTERACTION STRENGTH - TUNABLE
    // NEEDS TO BE HIGH BECAUSE ACCELERATION IS MULTIPLIED BY (dt * dt) LATER
    const float strength = 64000.0f;
    const float radiusSq = radius * radius;

    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // VECTOR FROM MOUSE TO PARTICLE (PUSH DIRECTION)
        float dx = particles.posX[i] - pos.x;
        float dy = particles.posY[i] - pos.y;

        float distSq = dx * dx + dy * dy;

        // ONLY AFFECT PARTICLES WITHIN THE RADIUS
        if (distSq < radiusSq && distSq > 0.0001f)
        {
            float dist = std::sqrt(distSq);

            // NORMALIZE THE DIRECTION VECTOR
            float nX = dx / dist;
            float nY = dy / dist;

            // CALCULATE FALLOFF FACTOR (0.0 to 1.0)
            // 1.0 AT CENTER, 0.0 AT EDGE
            float factor = 1.0f - (dist / radius);

            // APPLY FORCE TO ACCELERATION
            // WE ACCUMULATE THE FORCE SO IT BLENDS WITH GRAVITY
            particles.accX[i] += nX * strength * factor;
            particles.accY[i] += nY * strength * factor;
        }
    }
}

void Solver::mousePull(const Vector2 &pos, const float radius)
{
    const float strength = 64000.0f;
    const float radiusSq = radius * radius;

    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // VECTOR FROM PARTICLE TO MOUSE (PULL DIRECTION)
        float dx = pos.x - particles.posX[i];
        float dy = pos.y - particles.posY[i];

        float distSq = dx * dx + dy * dy;

        if (distSq < radiusSq && distSq > 0.0001f)
        {
            float dist = std::sqrt(distSq);

            // NORMALIZE
            float nX = dx / dist;
            float nY = dy / dist;

            // LINEAR FALLOFF
            float factor = 1.0f - (dist / radius);

            // APPLY ATTRACTION FORCE
            particles.accX[i] += nX * strength * factor;
            particles.accY[i] += nY * strength * factor;

            // WHEN PULLING, PARTICLES GAIN MASSIVE ENERGY AS THEY APPROACH THE CENTER
            // WE DAMPEN THEIR VELOCITY SLIGHTLY TO PREVENT THEM FROM SLINGSHOTTING TO INFINITY
            particles.oldX[i] = particles.oldX[i] + (particles.posX[i] - particles.oldX[i]) * 0.1f;
            particles.oldY[i] = particles.oldY[i] + (particles.posY[i] - particles.oldY[i]) * 0.1f;
        }
    }
}

// ---------------------------   Private Functions   ---------------------------

void Solver::defineParticlesInitialParameters()
{
    if (GENERATE_PARTICLES_PER_SECOND > 1)
    {
        initVelocityX = -2.0f * (GENERATE_PARTICLES_PER_SECOND * 0.5);
        initVelocityY = 1.0f * (GENERATE_PARTICLES_PER_SECOND * 0.5);
        initSpread = 20.0f * (GENERATE_PARTICLES_PER_SECOND * 0.5);
    }
    else
    {
        initVelocityX = -2.0f;
        initVelocityY = 0.0f;
        initSpread = 0.0f;
    }
}

void Solver::computeColorValues()
{
    // WE ARE GENERATING A RAINBOW LOOKUP TABLE (LUT) USING HSV COLOR SPACE
    // HSV ALLOWS US TO ROTATE THROUGH COLORS SMOOTHLY BY CHANGING 'H' (HUE)
    for (int i = 0; i < 256; i++)
    {
        // NORMALIZE i TO 0.0 - 1.0 RANGE
        float h = i / 256.0f;
        float s = 0.6f; // SATURATION: KEEP IT MODERATE FOR PASTEL LOOK
        float v = 1.0f; // VALUE: MAXIMUM BRIGHTNESS

        // STANDARD HSV TO RGB CONVERSION FORMULA
        // 'C' IS CHROMA (COLOR INTENSITY)
        float c = v * s;

        // 'X' IS THE INTERMEDIATE COMPONENT FOR THE SECOND LARGEST COLOR CHANNEL
        // IT CREATES THE "SLOPES" IN THE COLOR GRAPH
        float x = c * (1 - fabsf(fmodf(h * 6.0f, 2.0f) - 1));

        // 'M' IS USED TO MATCH THE VALUE (BRIGHTNESS) REQUIREMENT
        float m = v - c;

        float r, g, b;

        // DETERMINE WHICH SECTOR OF THE COLOR WHEEL WE ARE IN (0 TO 6)
        // AND ASSIGN RGB VALUES ACCORDINGLY
        if (h < 1.0f / 6.0f)
        {
            r = c;
            g = x;
            b = 0;
        }
        else if (h < 2.0f / 6.0f)
        {
            r = x;
            g = c;
            b = 0;
        }
        else if (h < 3.0f / 6.0f)
        {
            r = 0;
            g = c;
            b = x;
        }
        else if (h < 4.0f / 6.0f)
        {
            r = 0;
            g = x;
            b = c;
        }
        else if (h < 5.0f / 6.0f)
        {
            r = x;
            g = 0;
            b = c;
        }
        else
        {
            r = c;
            g = 0;
            b = x;
        }

        // CONVERT BACK TO 0-255 RANGE AND STORE IN THE LOOKUP TABLE
        rainbowLUT[i] = {
            (unsigned char)((r + m) * 255),
            (unsigned char)((g + m) * 255),
            (unsigned char)((b + m) * 255),
            255};
    }
}

void Solver::sortParticlesByCell()
{
    // CLEAR THE GRID COUNTERS
    // WE REUSE 'gridStart' TO STORE THE COUNT OF PARTICLES PER CELL TEMPORARILY
    std::fill(gridStart.begin(), gridStart.end(), 0);

    // COUNT PARTICLES IN EACH CELL
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        uint16_t cell = particleCellIDs[i];
        gridStart[cell]++;
    }

    // CONVERT COUNTS TO PARTIAL SUMS (PREFIX SUM)
    // THIS TELLS US WHERE EACH CELL'S BLOCK START IN THE SORTED ARRAY
    uint16_t sum = 0;
    for (uint16_t c = 0; c < TOTAL_CELLS; c++)
    {
        uint16_t count = gridStart[c];
        gridStart[c] = sum;
        sum += count;
    }

    // PLACE PARTICLES INTO THE SORTED ARRAY
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        uint16_t cell = particleCellIDs[i];

        // GET THE TARGET INDEX FROM gridStart, THEN INCREMENT IT
        // THIS ENSURES THE NEXT PARTICLE IN THIS CELL GOES RIGHT AFTER THIS ONE
        uint16_t dst = gridStart[cell]++;

        sortedParticleIndices[dst] = i;
    }

    // COPY SORTED INDICES BACK TO THE MAIN ARRAY
    // MEMCPY IS OFTEN FASTER THAN A LOOP FOR PRIMITIVE TYPES
    std::memcpy(particleIndices.data(), sortedParticleIndices.data(), currentParticlesCount * sizeof(uint16_t));

    // REPAIR THE GRID START POSITIONS
    // BECAUSE WE INCREMENTED 'gridStart' IN PREVIOUS STEP, IT NOW POINTS TO THE END OF EACH CELL'S BLOCK
    // (WHICH IS EFFECTIVELY THE START OF THE *NEXT* CELL'S BLOCK)
    // WE SHIFT EVERYTHING RIGHT BY ONE TO RESTORE THE ORIGINAL START INDICES
    for (uint16_t c = TOTAL_CELLS - 1; c > 0; c--)
    {
        gridStart[c] = gridStart[c - 1];
    }
    gridStart[0] = 0; // THE FIRST CELL ALWAYS STARTS AT INDEX 0
}

void Solver::updateParticlesPositions(const float dt)
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

        // CLAMP THE PARTICLE TO THE BOUNDARIES IF ONE GOES OUT OF BOUND
        x = std::clamp(static_cast<int>(x), 0, static_cast<int>(GRID_WIDTH - 1));
        y = std::clamp(static_cast<int>(y), 0, static_cast<int>(GRID_HEIGHT - 1));

        // MAP TO 1D ARRAY
        uint16_t cellIndex = y * GRID_WIDTH + x;

        // STORES THE CELL_INDEX OF THE PARTICLE
        particleCellIDs[i] = cellIndex;
    }

    // SORT THE ARRAY, SO THE CHECK COLLISION CAN WORK FASTER
    sortParticlesByCell();
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

                // MAP THE NEIGHBOUR'S CELL NUMBER ON 1D ARRAY
                int16_t neighborCell = nY * GRID_WIDTH + nX;

                // FIND THE STARTING POINT OF ALL THE NEIGHBOURING PARTICLES THAT ARE INSIDE THE NEIGHBOURING CELL
                int16_t k = gridStart[neighborCell];

                // -1 MEANS EMPTY CELL (NO PARTICLE IS INSIDE)
                if (k == -1)
                {
                    continue;
                }

                // LOOP THROUGH ALL NEIGHBOURS | WE ARE USING SORTED ARRAY, SO NEIGHBOURS ARE LOCATED SIDE BY SIDE
                while (k < currentParticlesCount)
                {
                    uint16_t p2 = particleIndices[k]; // GET THE INDEX OF ONE NEIGHBOURING PARTICLE

                    // BASE CONDITION: IF THE PARTICLE DOES NO LONGER MATCHES THE CELL, WE KNOW THE CELL'S BLOCK IS FINISHED AND ANOTHER BLOCK IS STARTD
                    if (particleCellIDs[p2] != neighborCell)
                    {
                        break;
                    }

                    // ONLY CHECK THE PARTICLES ONCE, AND DON'T CHECK THE PARTICLE WITH ITSELF
                    if (p1 < p2)
                    {
                        resolveParticlesCollisions(p1, p2);
                    }

                    // MOVE TO THE NEXT NEIGHBOUR PARTICLE
                    k++;
                }
            }
        }
    }
}

void Solver::resolveParticlesCollisions(const uint16_t p1, const uint16_t p2)
{
    // FIND DISPLACEMENt
    float dx = particles.posX[p1] - particles.posX[p2];
    float dy = particles.posY[p1] - particles.posY[p2];

    // SQUARED DISTANCE BETWEEN TWO PARTICLES
    float distSq = dx * dx + dy * dy;

    // PARTICLES SHOULD BE SEPERATED BY THE SUM OF THEIR RADII (RADII ARE THE SAME)
    float minDst = particles.rad[p1] + particles.rad[p2];

    if (distSq < (minDst * minDst) && distSq > 0.0001f)
    {
        // COMPLETE THE FORMULA
        // SINCE THE SQRT() IS EXPENSIVE, WE ONLY COMPUTE IT IF WE KNOW FOR SURE THE COLLISION HAS OCCURED
        float dist = std::sqrtf(distSq);

        // NORMALIZE
        float nX = dx / dist;
        float nY = dy / dist;

        // PENETERATION DEPTH FROM EITHER SIDE
        float delta = 0.5f * (minDst - dist);
        // SINCE THE RADII ARE THE SAME, EACH PARTICLE WOULD BE EQUALLY PENERATED INSIDE OTHER,
        // SO WE MULTIPLY IT BY 0.5 TO FIND THE PENETERATION DEPTH OF ONE PARTICLE, WE COULD ALSO USE DIVISION BY 2, BUT MULTIPLICATION IS FASTER

        // FIND HOW MUCH TO CORRECT THE POSITION
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
        if (particles.posY[i] > SCREEN_HEIGHT - particles.rad[i])
        {
            // CLAMP THE PARTICLE INSIDE THE BOUNDARY
            particles.posY[i] = SCREEN_HEIGHT - particles.rad[i];

            // CALCULATE THE RESISTIVE FORCES
            dx *= FRICTION;
            dy *= -DAMPENING;

            // ADD THE RESISTIVE FORCES
            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }
        // TOP - CEILING
        else if (particles.posY[i] < particles.rad[i])
        {
            particles.posY[i] = particles.rad[i];

            dx *= FRICTION;
            dy *= -DAMPENING;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }

        // RIGHT - WALL
        if (particles.posX[i] > SCREEN_WIDTH - particles.rad[i])
        {
            particles.posX[i] = SCREEN_WIDTH - particles.rad[i];

            dx *= -DAMPENING;
            dy *= FRICTION;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }
        // LEFT - WALL
        else if (particles.posX[i] < particles.rad[i])
        {
            particles.posX[i] = particles.rad[i];

            dx *= -DAMPENING;
            dy *= FRICTION;

            particles.oldX[i] = particles.posX[i] - dx;
            particles.oldY[i] = particles.posY[i] - dy;
        }
    }
}