#ifndef SOLVER_H
#define SOLVER_H

#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>

#include "defines.hpp"
#include "particle.hpp"
#include "threading.hpp"
#include "raylib.h"

class Solver
{
public:
    static Particle particles;

    inline static uint32_t currentParticlesCount = 0;

public:
    // BUILDS THE COLOR TABLE, CLEARS THE PARTICLE STATE AND BRINGS THE WORKER
    // POOL UP. MUST BE PAIRED WITH shutdown().
    static void preComputeInitialValues();
    static void shutdown();

    // SPAWNS UP TO 'count' PARTICLES. THE NOZZLE CHOKES AT EMITTER_CAPACITY AND
    // SKIPS ANY LATTICE SLOT THAT IS ALREADY OCCUPIED, SO THIS NEVER CREATES AN
    // OVERLAPPING PAIR. RETURNS HOW MANY WERE ACTUALLY EMITTED.
    static uint32_t generateParticles(const uint32_t count);

    // RUNS ONE FRAME ON A FIXED TIME STEP. DELIBERATELY TAKES NO dt.
    static void updateSimulationState();

    static void mousePush(const Vector2 &pos, const float radius);
    static void mousePull(const Vector2 &pos, const float radius);

    static uint32_t threadCount() { return ThreadPool::threadCount(); }

private:
    // BELOW THIS MANY PARTICLES A BARRIER COSTS MORE THAN THE WORK IT SPLITS, SO
    // THE PASS JUST RUNS ON THE CALLING THREAD.
    static constexpr uint32_t PARALLEL_MIN_PARTICLES = 4096;

    // ONE FULL SET OF PARTICLE FIELDS. TWO OF THESE EXIST AND THE SOLVER PING-PONGS
    // BETWEEN THEM EVERY TIME IT REORDERS PARTICLES INTO SPATIAL ORDER.
    struct ParticleBuffer
    {
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> posX;
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> posY;
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> velX;
        alignas(64) std::array<float, TOTAL_PARTICLES_COUNT> velY;
        alignas(64) std::array<Color, TOTAL_PARTICLES_COUNT> color;
    };

    // A CONTIGUOUS RUN OF GRID ROWS, INCLUSIVE AT BOTH ENDS, OWNED BY ONE THREAD
    // FOR THE DURATION OF ONE CONTACT SWEEP.
    struct RowBand
    {
        int32_t first;
        int32_t last;
    };

    static ParticleBuffer bufferA;
    static ParticleBuffer bufferB;
    static bool usingBufferA;

    // CELL c OCCUPIES SORTED PARTICLE INDICES [cellRange[c], cellRange[c + 1]).
    // ONE ARRAY GIVES BOTH THE START AND THE END OF EVERY CELL, AND BECAUSE THE
    // GRID IS ROW-MAJOR, THREE HORIZONTALLY ADJACENT CELLS ARE ONE CONTIGUOUS RUN.
    alignas(64) static std::array<uint32_t, CELL_RANGE_SIZE> cellRange;
    alignas(64) static std::array<uint32_t, TOTAL_PARTICLES_COUNT> particleCellIDs;

    static std::array<Color, 256> rainbowLUT;
    inline static uint32_t spawnColorCursor = 0;

    // ---------------------------   PARALLEL SORT SCRATCH   ---------------------------

    // ONE PRIVATE HISTOGRAM PER THREAD, EACH CELL_RANGE_SIZE ENTRIES LONG AND
    // 64-BYTE ALIGNED. A SHARED HISTOGRAM WOULD NEED AN ATOMIC INCREMENT PER
    // PARTICLE; PRIVATE ONES NEED NONE AT ALL. AFTER THE PREFIX SUM THE SAME
    // MEMORY IS REWRITTEN IN PLACE AS EACH THREAD'S PRIVATE WRITE CURSORS, WHICH
    // IS WHAT MAKES THE SCATTER PARALLEL *AND* KEEPS ITS OUTPUT BIT-FOR-BIT
    // IDENTICAL TO THE SERIAL COUNTING SORT.
    static uint32_t *cellCursors;
    static uint32_t cursorStride;

    static std::array<uint32_t, ThreadPool::MAX_THREADS + 1> chunkBase;

    // ---------------------------   CONTACT SWEEP PLAN   ---------------------------

    // PER-ROW WORK ESTIMATE, SUM OF n^2 OVER THE ROW'S CELLS. PAIR TESTS IN A CELL
    // SCALE WITH THE SQUARE OF ITS OCCUPANCY, SO SPLITTING ON PARTICLE COUNT ALONE
    // HANDS THE DENSE BANDS TOO MUCH WORK AND EVERY OTHER CORE WAITS AT THE
    // BARRIER. THIS IS FILLED IN FOR FREE BY THE PREFIX-SUM PASS.
    static std::array<uint64_t, PADDED_GRID_HEIGHT> rowWork;

    // TWO BANDS PER THREAD: THE ODD-INDEXED ONES ARE SWEPT IN ONE PASS AND THE
    // EVEN-INDEXED ONES IN THE NEXT, SO NEITHER PASS EVER HAS TWO NEIGHBOURING
    // BANDS RUNNING AT THE SAME TIME AND BOTH PASSES KEEP EVERY CORE BUSY.
    static std::array<RowBand, ThreadPool::MAX_THREADS * 2> bands;
    static uint32_t bandCount;

    static void bindBuffers();
    static void computeColorValues();

    // SettleFirst = true FUSES THE PREVIOUS SUB-STEP'S WALL PASS INTO THE HEAD OF
    // THIS ONE. BOTH ARE PURELY PER-PARTICLE, SO THE FUSED LOOP IS EXACTLY THE OLD
    // ORDERING WITH ONE FEWER SWEEP OVER HALF A MEGABYTE.
    template <bool SettleFirst>
    static void integrateAndBound(const float dt);

    static void buildGrid();

    // CARVES THE GRID INTO TWO INTERLEAVED ROW BANDS PER THREAD.
    // CALLED ONCE PER GRID REBUILD.
    static void planBands();

    // SWEEPS ROWS [rowTo, rowFrom] FROM THE BOTTOM UPWARDS. BOTH ENDS INCLUSIVE.
    template <bool Brace>
    static void sweepRows(const int32_t rowFrom, const int32_t rowTo);

    // Brace = false: symmetric relaxation, momentum conserving, springy.
    // Brace = true:  the lower particle is pinned vertically so the floor's support
    //                climbs a whole column in one sweep. Horizontal stays symmetric
    //                so this adds support without adding an angle of repose.
    template <bool Brace>
    static void solveContacts();

    static void applyBoundaries();

    // TRUE IF NOTHING ALREADY SITS WITHIN ONE DIAMETER OF (x, y). USES THE GRID
    // LEFT OVER FROM THE PREVIOUS FRAME, WHICH IS STILL EXACT BECAUSE NOTHING HAS
    // MOVED SINCE.
    static bool isSpawnSlotFree(const float x, const float y);
};

#endif
