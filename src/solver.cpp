#include "solver.hpp"

#include <cstdlib>

// ---------------------------   STORAGE   ---------------------------

float *Particle::posX = nullptr;
float *Particle::posY = nullptr;
float *Particle::velX = nullptr;
float *Particle::velY = nullptr;
Color *Particle::color = nullptr;

Particle Solver::particles;

Solver::ParticleBuffer Solver::bufferA;
Solver::ParticleBuffer Solver::bufferB;
bool Solver::usingBufferA = true;

std::array<uint32_t, CELL_RANGE_SIZE> Solver::cellRange;
std::array<uint32_t, TOTAL_PARTICLES_COUNT> Solver::particleCellIDs;

std::array<Color, 256> Solver::rainbowLUT;

// ---------------------------   LOCAL HELPERS   ---------------------------

namespace
{
    constexpr float BOUND_MIN_X = PARTICLE_RADIUS;
    constexpr float BOUND_MAX_X = static_cast<float>(SCREEN_WIDTH) - PARTICLE_RADIUS;
    constexpr float BOUND_MIN_Y = PARTICLE_RADIUS;
    constexpr float BOUND_MAX_Y = static_cast<float>(SCREEN_HEIGHT) - PARTICLE_RADIUS;

    // UNIFORM IN [-1, 1)
    inline float randSigned()
    {
        return (static_cast<float>(rand() % 2048) * (1.0f / 1024.0f)) - 1.0f;
    }

    // CLAMPS ONE PARTICLE BACK INSIDE THE DOMAIN AND SETTLES ITS WALL VELOCITY.
    // A HIT SLOWER THAN WALL_RESTING_SPEED STOPS DEAD RATHER THAN BOUNCING, WHICH
    // LETS A PILE COME TO REST ON THE FLOOR INSTEAD OF SIMMERING. AT ZERO THE FLOOR
    // NEVER GOES DEAD AND EVERY HIT KEEPS WALL_RESTITUTION OF ITS SPEED.
    inline void resolveBounds(float &x, float &y, float &vx, float &vy)
    {
        if (y > BOUND_MAX_Y)
        {
            y = BOUND_MAX_Y;
            if (vy > 0.0f)
            {
                vy = (vy < WALL_RESTING_SPEED) ? 0.0f : -vy * WALL_RESTITUTION;
            }
            vx *= WALL_FRICTION;
        }
        else if (y < BOUND_MIN_Y)
        {
            y = BOUND_MIN_Y;
            if (vy < 0.0f)
            {
                vy = (vy > -WALL_RESTING_SPEED) ? 0.0f : -vy * WALL_RESTITUTION;
            }
            vx *= WALL_FRICTION;
        }

        if (x > BOUND_MAX_X)
        {
            x = BOUND_MAX_X;
            if (vx > 0.0f)
            {
                vx = (vx < WALL_RESTING_SPEED) ? 0.0f : -vx * WALL_RESTITUTION;
            }
            vy *= WALL_FRICTION;
        }
        else if (x < BOUND_MIN_X)
        {
            x = BOUND_MIN_X;
            if (vx < 0.0f)
            {
                vx = (vx > -WALL_RESTING_SPEED) ? 0.0f : -vx * WALL_RESTITUTION;
            }
            vy *= WALL_FRICTION;
        }
    }

    // ONE CONTACT.
    //
    // THE RESPONSE HERE IS DELIBERATELY THE ORIGINAL VERLET ONE. THAT SOLVER KEPT
    // VELOCITY IMPLICITLY AS (pos - oldPos), SO SHOVING TWO OVERLAPPING PARTICLES
    // APART ALSO HANDED THEM THAT SHOVE AS SEPARATION SPEED, AND
    // COLLISION_REACTION_LOSS SKIMMED A FIFTH OF IT BACK OFF. THAT LITTLE SPRING IS
    // THE CHARACTER OF THE WHOLE SIMULATION: CONTACTS POP, A HEAP SLUMPS AND
    // SPREADS INSTEAD OF HOLDING A FACE, AND THE STREAM SPLASHES WHERE IT LANDS.
    //
    // THE TEXTBOOK ALTERNATIVE IS TO LEAVE VELOCITY ALONE DURING THE POSITION FIX
    // AND THEN CANCEL THE APPROACH SPEED WITH AN e = 0 IMPULSE. IT IS THE HONEST
    // WAY TO WRITE A RIGID CONTACT AND IT IS ALSO EXACTLY WHY THE SIMULATION STARTED
    // LOOKING LIKE DRY SAND: EVERY CONTACT WENT DEAD ON ARRIVAL. SO THE POSITION FIX
    // FEEDS VELOCITY, ON PURPOSE, AND COLLISION_REACTION_LOSS IS THE ONLY BRAKE.
    //
    // PARTICLE i IS PASSED IN REGISTERS RATHER THAN BY INDEX. IT TAKES PART IN EVERY
    // PAIR IN ITS INNER LOOP, AND SINCE THE COMPILER CANNOT PROVE i AND j NEVER
    // ALIAS, GOING THROUGH MEMORY WOULD FORCE FOUR RELOADS AND FOUR STORES ON EVERY
    // SINGLE CANDIDATE. THE CALLER LOADS IT ONCE AND WRITES IT BACK ONCE.
    template <bool ShockPropagation>
    inline void solveContact(float &posIX, float &posIY, float &velIX, float &velIY,
                             const uint32_t j,
                             float *__restrict px, float *__restrict py,
                             float *__restrict vx, float *__restrict vy)
    {
        const float dx = posIX - px[j];
        const float dy = posIY - py[j];
        const float distSq = dx * dx + dy * dy;

        if (distSq >= MIN_COLLISION_DIST_SQ || distSq < 1e-8f)
        {
            return;
        }

        const float dist = std::sqrt(distSq);
        const float invDist = 1.0f / dist;
        const float nx = dx * invDist;
        const float ny = dy * invDist;

        // HOW THE CORRECTION IS SHARED. NORMALLY HALF EACH, WHICH CONSERVES MOMENTUM
        // AND LETS BOTH PARTICLES SLIDE. IN A SHOCK PASS THE LOWER ONE IS PINNED AND
        // THE UPPER ONE ABSORBS EVERYTHING, WHICH IS WHAT BUILDS RIGID COLUMNS.
        float weightI = 0.5f;
        float weightJ = 0.5f;
        if constexpr (ShockPropagation)
        {
            const bool iIsLower = (posIY > py[j]);
            weightI = iIsLower ? 0.0f : 1.0f;
            weightJ = iIsLower ? 1.0f : 0.0f;
        }

        // 1. NON-PENETRATION.
        const float penetration = std::fmax(0.0f, MIN_COLLISION_DIST - dist - PENETRATION_SLOP);
        const float correction = POSITION_CORRECTION * penetration;

        const float moveIX = correction * nx * weightI;
        const float moveIY = correction * ny * weightI;
        const float moveJX = correction * nx * weightJ;
        const float moveJY = correction * ny * weightJ;

        posIX += moveIX;
        posIY += moveIY;
        px[j] -= moveJX;
        py[j] -= moveJY;

        // 2. THE SHOVE BECOMES SPEED, MINUS WHATEVER COLLISION_REACTION_LOSS TAKES.
        //    NOTE THAT NOTHING CAPS THIS AGAINST THE APPROACH SPEED, SO A CONTACT
        //    CAN LEAVE FASTER THAN IT ARRIVED. THAT IS THE BOUNCE, AND IT IS WANTED.
        velIX += moveIX * CONTACT_KICK;
        velIY += moveIY * CONTACT_KICK;
        vx[j] -= moveJX * CONTACT_KICK;
        vy[j] -= moveJY * CONTACT_KICK;

        // 3. TANGENTIAL FRICTION, COMPILED OUT ENTIRELY WHEN IT IS OFF. WITH IT ON A
        //    HEAP HOLDS AN ANGLE OF REPOSE; WITH IT OFF THE PARTICLES SLIDE OVER ONE
        //    ANOTHER AND THE MATERIAL FLOWS.
        if constexpr (PARTICLE_FRICTION > 0.0f)
        {
            const float relVx = velIX - vx[j];
            const float relVy = velIY - vy[j];
            const float relVn = relVx * nx + relVy * ny;

            const float tanVx = relVx - relVn * nx;
            const float tanVy = relVy - relVn * ny;
            const float fricX = PARTICLE_FRICTION * tanVx;
            const float fricY = PARTICLE_FRICTION * tanVy;

            velIX -= fricX * weightI;
            velIY -= fricY * weightI;
            vx[j] += fricX * weightJ;
            vy[j] += fricY * weightJ;
        }
    }
} // namespace

// ---------------------------   PUBLIC FUNCTIONS   ---------------------------

void Solver::preComputeInitialValues()
{
    computeColorValues();

    currentParticlesCount = 0;
    spawnColorCursor = 0;
    usingBufferA = true;
    bindBuffers();

    // AN EMPTY GRID: EVERY CELL RANGE IS [0, 0), SO THE FIRST SPAWN QUERY FINDS
    // NOTHING AND THE FIRST COLLISION SWEEP TOUCHES NOTHING.
    std::fill(cellRange.begin(), cellRange.end(), 0u);

    std::fill(bufferA.posX.begin(), bufferA.posX.end(), 0.0f);
    std::fill(bufferA.posY.begin(), bufferA.posY.end(), 0.0f);
    std::fill(bufferA.velX.begin(), bufferA.velX.end(), 0.0f);
    std::fill(bufferA.velY.begin(), bufferA.velY.end(), 0.0f);
}

uint32_t Solver::generateParticles(const uint32_t count)
{
    // THE NOZZLE CHOKES AT ITS PHYSICAL THROUGHPUT. ASKING FOR MORE THAN IT CAN
    // PASS USED TO STACK PARTICLES INSIDE ONE ANOTHER AT THE SPAWN POINT.
    uint32_t wanted = std::min(count, EMITTER_CAPACITY);
    wanted = std::min(wanted, TOTAL_PARTICLES_COUNT - currentParticlesCount);

    if (wanted == 0)
    {
        return 0;
    }

    // UNIT VECTOR ACROSS THE NOZZLE MOUTH
    constexpr float perpX = -EMITTER_DIR_Y;
    constexpr float perpY = EMITTER_DIR_X;

    // RINGS ARE SPREAD EVENLY OVER THE DISTANCE THE STREAM COVERS IN ONE FRAME,
    // SO CONSECUTIVE FRAMES BUTT UP AGAINST EACH OTHER AND THE JET READS AS ONE
    // CONTINUOUS BODY OF MATERIAL RATHER THAN A BURST OF CLUMPS.
    constexpr float ringStride = EMITTER_STEP / static_cast<float>(EMITTER_RINGS);
    constexpr float laneCentre = 0.5f * static_cast<float>(EMITTER_LANES - 1);

    float *__restrict px = Particle::posX;
    float *__restrict py = Particle::posY;
    float *__restrict vx = Particle::velX;
    float *__restrict vy = Particle::velY;

    uint32_t emitted = 0;

    for (uint32_t k = 0; k < wanted; k++)
    {
        const uint32_t lane = k % EMITTER_LANES;
        const uint32_t ring = k / EMITTER_LANES;

        // ODD RINGS ARE OFFSET BY HALF A LANE. A SQUARE LATTICE IS VISIBLE AS A GRID
        // OF DOTS FOR THE FIRST COUPLE OF HUNDRED PIXELS OUT OF THE NOZZLE AND READS
        // AS PRINTED RATHER THAN POURED; STAGGERING IT PACKS HEXAGONALLY, WHICH IS
        // WHAT LOOSE MATERIAL ACTUALLY DOES, AND BREAKS THE PATTERN UP.
        const float stagger = (ring & 1u) ? (0.5f * SPAWN_SPACING) : 0.0f;

        const float across = (static_cast<float>(lane) - laneCentre) * SPAWN_SPACING + stagger;
        const float along = static_cast<float>(ring) * ringStride;

        const float x = EMITTER_POS_X + perpX * across + EMITTER_DIR_X * along;
        const float y = EMITTER_POS_Y + perpY * across + EMITTER_DIR_Y * along;

        // IF THE PILE HAS BACKED UP INTO THE NOZZLE, DO NOT FIRE INTO IT.
        if (!isSpawnSlotFree(x, y))
        {
            continue;
        }

        const float speed = EMITTER_SPEED * (1.0f + EMITTER_SPEED_JITTER * randSigned());
        const float spread = EMITTER_SPREAD * randSigned();

        const uint32_t idx = currentParticlesCount + emitted;

        px[idx] = x;
        py[idx] = y;
        vx[idx] = EMITTER_DIR_X * speed + perpX * spread;
        vy[idx] = EMITTER_DIR_Y * speed + perpY * spread;
        Particle::color[idx] = rainbowLUT[spawnColorCursor & 255];

        spawnColorCursor++;
        emitted++;
    }

    currentParticlesCount += emitted;
    return emitted;
}

void Solver::updateSimulationState()
{
    for (uint8_t s = 0; s < SUB_STEPS; s++)
    {
        integrateAndBound(SUB_DT);
        buildGrid();

        // SYMMETRIC RELAXATION. MOMENTUM CONSERVING, BOTH PARTICLES FREE TO MOVE,
        // NOBODY PINNED. WITH SHOCK_PROPAGATION OFF THESE ARE THE ONLY CONTACT
        // PASSES THERE ARE, AND THEY CARRY THE WHOLE PILE.
        for (uint8_t iter = 0; iter < COLLISION_ITERATIONS; iter++)
        {
            solveContacts<false>();
        }

        // THE SHOCK PASS IS THE SAND SWITCH. SEE SHOCK_PROPAGATION IN defines.hpp.
        if constexpr (SHOCK_PROPAGATION)
        {
            // PIN THE FLOOR FIRST, SO THE BOTTOM LAYER IS ALREADY AT REST WHEN THE
            // PASS STARTS PROPAGATING SUPPORT UPWARDS FROM IT.
            applyBoundaries();
            solveContacts<true>();
        }

        // CONTACTS CAN SHOVE A PARTICLE THROUGH A WALL, SO THE DOMAIN IS THE LAST
        // CONSTRAINT ENFORCED AND IT IS ENFORCED ABSOLUTELY.
        applyBoundaries();
    }
}

void Solver::mousePush(const Vector2 &pos, const float radius)
{
    const float radiusSq = radius * radius;
    const float impulse = MOUSE_FORCE * SIMULATION_DT;

    float *__restrict px = Particle::posX;
    float *__restrict py = Particle::posY;
    float *__restrict vx = Particle::velX;
    float *__restrict vy = Particle::velY;

    for (uint32_t i = 0; i < currentParticlesCount; i++)
    {
        const float dx = px[i] - pos.x;
        const float dy = py[i] - pos.y;
        const float distSq = dx * dx + dy * dy;

        if (distSq < radiusSq && distSq > 1e-4f)
        {
            const float dist = std::sqrt(distSq);
            const float falloff = 1.0f - (dist / radius);
            const float kick = impulse * falloff / dist;

            vx[i] += dx * kick;
            vy[i] += dy * kick;
        }
    }
}

void Solver::mousePull(const Vector2 &pos, const float radius)
{
    const float radiusSq = radius * radius;
    const float impulse = MOUSE_FORCE * SIMULATION_DT;

    float *__restrict px = Particle::posX;
    float *__restrict py = Particle::posY;
    float *__restrict vx = Particle::velX;
    float *__restrict vy = Particle::velY;

    for (uint32_t i = 0; i < currentParticlesCount; i++)
    {
        const float dx = pos.x - px[i];
        const float dy = pos.y - py[i];
        const float distSq = dx * dx + dy * dy;

        if (distSq < radiusSq && distSq > 1e-4f)
        {
            const float dist = std::sqrt(distSq);
            const float falloff = 1.0f - (dist / radius);
            const float kick = impulse * falloff / dist;

            vx[i] += dx * kick;
            vy[i] += dy * kick;

            // EVERYTHING FALLING TOWARDS ONE POINT PICKS UP ENORMOUS SPEED AT THE
            // CENTRE. BLEED IT OFF SO THE CURSOR GATHERS A BLOB INSTEAD OF FIRING
            // ONE OUT THE OTHER SIDE.
            vx[i] *= MOUSE_PULL_DAMPING;
            vy[i] *= MOUSE_PULL_DAMPING;
        }
    }
}

// ---------------------------   PRIVATE FUNCTIONS   ---------------------------

void Solver::bindBuffers()
{
    ParticleBuffer &live = usingBufferA ? bufferA : bufferB;

    Particle::posX = live.posX.data();
    Particle::posY = live.posY.data();
    Particle::velX = live.velX.data();
    Particle::velY = live.velY.data();
    Particle::color = live.color.data();
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
        if (h < 1.0f / 6.0f)      { r = c; g = x; b = 0; }
        else if (h < 2.0f / 6.0f) { r = x; g = c; b = 0; }
        else if (h < 3.0f / 6.0f) { r = 0; g = c; b = x; }
        else if (h < 4.0f / 6.0f) { r = 0; g = x; b = c; }
        else if (h < 5.0f / 6.0f) { r = x; g = 0; b = c; }
        else                      { r = c; g = 0; b = x; }

        // CONVERT BACK TO 0-255 RANGE AND STORE IN THE LOOKUP TABLE
        rainbowLUT[i] = {
            (unsigned char)((r + m) * 255),
            (unsigned char)((g + m) * 255),
            (unsigned char)((b + m) * 255),
            255};
    }
}

void Solver::integrateAndBound(const float dt)
{
    const uint32_t count = currentParticlesCount;
    const float gravityStep = GRAVITY * dt;

    float *__restrict px = Particle::posX;
    float *__restrict py = Particle::posY;
    float *__restrict vx = Particle::velX;
    float *__restrict vy = Particle::velY;

    for (uint32_t i = 0; i < count; i++)
    {
        float velocityX = vx[i];
        float velocityY = vy[i] + gravityStep;

        // SAFETY VALVE. A PARTICLE THAT CROSSES MORE THAN A CELL IN ONE SUB-STEP CAN
        // TUNNEL STRAIGHT THROUGH A NEIGHBOUR BEFORE THE GRID EVER SEES THE CONTACT.
        const float speedSq = velocityX * velocityX + velocityY * velocityY;
        if (speedSq > MAX_SPEED_SQ)
        {
            const float scale = MAX_SPEED / std::sqrt(speedSq);
            velocityX *= scale;
            velocityY *= scale;
        }

        float x = px[i] + velocityX * dt;
        float y = py[i] + velocityY * dt;

        resolveBounds(x, y, velocityX, velocityY);

        px[i] = x;
        py[i] = y;
        vx[i] = velocityX;
        vy[i] = velocityY;
    }
}

void Solver::buildGrid()
{
    const uint32_t count = currentParticlesCount;

    std::fill(cellRange.begin(), cellRange.end(), 0u);

    const float *__restrict px = Particle::posX;
    const float *__restrict py = Particle::posY;
    uint32_t *__restrict ids = particleCellIDs.data();
    uint32_t *__restrict range = cellRange.data();

    // PASS 1: WHICH CELL, AND HOW MANY LAND IN IT.
    // THE HISTOGRAM IS WRITTEN AT (id + 2) SO THAT AFTER THE PREFIX SUM AND THE
    // SCATTER BELOW, cellRange[c] AND cellRange[c + 1] LAND EXACTLY ON THE START
    // AND END OF CELL c. THAT SAVES THE SEPARATE CURSOR ARRAY AND THE WHOLE-ARRAY
    // SHIFT THE OLD COUNTING SORT NEEDED TO REPAIR ITSELF.
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t cellX = static_cast<int32_t>(px[i] * INV_CELL_SIZE);
        int32_t cellY = static_cast<int32_t>(py[i] * INV_CELL_SIZE);

        cellX = std::clamp(cellX, 0, static_cast<int32_t>(GRID_WIDTH) - 1);
        cellY = std::clamp(cellY, 0, static_cast<int32_t>(GRID_HEIGHT) - 1);

        const uint32_t id = static_cast<uint32_t>(cellY + 1) * PADDED_GRID_WIDTH +
                            static_cast<uint32_t>(cellX + 1);

        ids[i] = id;
        range[id + 2]++;
    }

    // PASS 2: PREFIX SUM.
    for (uint32_t c = 1; c < CELL_RANGE_SIZE; c++)
    {
        range[c] += range[c - 1];
    }

    // PASS 3: SCATTER STRAIGHT INTO THE OTHER BUFFER SET.
    // READS ARE SEQUENTIAL AND WRITES ARE ALMOST SEQUENTIAL, BECAUSE THE PARTICLES
    // WERE ALREADY SORTED LAST SUB-STEP AND HAVE MOVED LESS THAN A PIXEL SINCE.
    // ONE PASS, NO PERMUTATION ARRAY, NO COPY BACK.
    const ParticleBuffer &src = usingBufferA ? bufferA : bufferB;
    ParticleBuffer &dst = usingBufferA ? bufferB : bufferA;

    for (uint32_t i = 0; i < count; i++)
    {
        const uint32_t slot = range[ids[i] + 1]++;

        dst.posX[slot] = src.posX[i];
        dst.posY[slot] = src.posY[i];
        dst.velX[slot] = src.velX[i];
        dst.velY[slot] = src.velY[i];
        dst.color[slot] = src.color[i];
    }

    usingBufferA = !usingBufferA;
    bindBuffers();
}

template <bool ShockPropagation>
void Solver::solveContacts()
{
    float *__restrict px = Particle::posX;
    float *__restrict py = Particle::posY;
    float *__restrict vx = Particle::velX;
    float *__restrict vy = Particle::velY;
    const uint32_t *__restrict range = cellRange.data();

    // SWEEP ROW BY ROW FROM THE FLOOR UPWARDS.
    //
    // THIS DIRECTION IS MANDATORY FOR THE SHOCK PASS: IT PINS THE LOWER PARTICLE OF
    // EVERY CONTACT, SO IT ONLY WORKS IF EVERYTHING BELOW HAS ALREADY BEEN SETTLED
    // THIS SWEEP. IT HELPS THE SYMMETRIC PASSES FOR THE SAME UNDERLYING REASON.
    //
    // GAUSS-SEIDEL CARRIES A CORRECTION ONE CONTACT FURTHER PER PASS. IN A PILE THE
    // SUPPORT COMES FROM THE GROUND, SO A TOP-DOWN SWEEP NEEDS ROUGHLY ONE PASS PER
    // STACKED LAYER BEFORE THE FLOOR'S PUSH REACHES THE TOP, AND A HUNDRED-DEEP PILE
    // SIMPLY NEVER CONVERGES. SWEEPING WITH THE SUPPORT DIRECTION CARRIES IT THROUGH
    // A WHOLE COLUMN IN A SINGLE PASS, FOR FREE.
    for (int32_t cellY = GRID_HEIGHT; cellY >= 1; --cellY)
    {
        const uint32_t rowBase = static_cast<uint32_t>(cellY) * PADDED_GRID_WIDTH;

        for (uint32_t cellX = 1; cellX <= GRID_WIDTH; ++cellX)
        {
            const uint32_t cell = rowBase + cellX;
            const uint32_t begin = range[cell];
            const uint32_t end = range[cell + 1];

            if (begin == end)
            {
                continue;
            }

            // EACH UNORDERED PAIR IS VISITED EXACTLY ONCE BY ONLY EVER LOOKING AT
            // NEIGHBOURS WITH A HIGHER CELL INDEX: THE CELL TO THE RIGHT AND THE
            // THREE BELOW. THE OTHER FOUR NEIGHBOURS FIND US INSTEAD. THAT HALVES
            // THE PAIR TESTS AND DROPS THE p1 < p2 BRANCH ENTIRELY.
            //
            // AND BECAUSE THE GRID IS ROW-MAJOR AND THE PARTICLES ARE SORTED BY CELL
            // INDEX, THIS CELL AND THE ONE TO ITS RIGHT ARE ONE CONTIGUOUS RUN, AND
            // SO ARE THE THREE BELOW. NINE SCATTERED LOOKUPS PER PARTICLE BECOME TWO
            // STRAIGHT-LINE SCANS PER CELL.
            const uint32_t rightEnd = range[cell + 2];
            const uint32_t belowBegin = range[cell + PADDED_GRID_WIDTH - 1];
            const uint32_t belowEnd = range[cell + PADDED_GRID_WIDTH + 2];

            for (uint32_t i = begin; i < end; ++i)
            {
                // NO j IN EITHER RUN BELOW IS EVER EQUAL TO i (THE FIRST STARTS AT
                // i + 1, THE SECOND IS A DIFFERENT CELL), SO KEEPING i IN LOCALS FOR
                // THE DURATION IS STILL EXACT GAUSS-SEIDEL, NOT AN APPROXIMATION.
                float posIX = px[i];
                float posIY = py[i];
                float velIX = vx[i];
                float velIY = vy[i];

                for (uint32_t j = i + 1; j < rightEnd; ++j)
                {
                    solveContact<ShockPropagation>(posIX, posIY, velIX, velIY, j, px, py, vx, vy);
                }

                for (uint32_t j = belowBegin; j < belowEnd; ++j)
                {
                    solveContact<ShockPropagation>(posIX, posIY, velIX, velIY, j, px, py, vx, vy);
                }

                px[i] = posIX;
                py[i] = posIY;
                vx[i] = velIX;
                vy[i] = velIY;
            }
        }
    }
}

void Solver::applyBoundaries()
{
    const uint32_t count = currentParticlesCount;

    float *__restrict px = Particle::posX;
    float *__restrict py = Particle::posY;
    float *__restrict vx = Particle::velX;
    float *__restrict vy = Particle::velY;

    for (uint32_t i = 0; i < count; i++)
    {
        float x = px[i];
        float y = py[i];
        float velocityX = vx[i];
        float velocityY = vy[i];

        resolveBounds(x, y, velocityX, velocityY);

        px[i] = x;
        py[i] = y;
        vx[i] = velocityX;
        vy[i] = velocityY;
    }
}

bool Solver::isSpawnSlotFree(const float x, const float y)
{
    if (currentParticlesCount == 0)
    {
        return true;
    }

    int32_t cellX = std::clamp(static_cast<int32_t>(x * INV_CELL_SIZE), 0, static_cast<int32_t>(GRID_WIDTH) - 1);
    int32_t cellY = std::clamp(static_cast<int32_t>(y * INV_CELL_SIZE), 0, static_cast<int32_t>(GRID_HEIGHT) - 1);

    const float *__restrict px = Particle::posX;
    const float *__restrict py = Particle::posY;

    for (int32_t dy = -1; dy <= 1; ++dy)
    {
        const uint32_t centre = static_cast<uint32_t>(cellY + 1 + dy) * PADDED_GRID_WIDTH +
                                static_cast<uint32_t>(cellX + 1);

        const uint32_t begin = cellRange[centre - 1];
        const uint32_t end = cellRange[centre + 2];

        for (uint32_t k = begin; k < end; ++k)
        {
            const float dx = px[k] - x;
            const float dy2 = py[k] - y;

            if (dx * dx + dy2 * dy2 < MIN_COLLISION_DIST_SQ)
            {
                return false;
            }
        }
    }

    return true;
}
