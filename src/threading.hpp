#ifndef THREADING_H
#define THREADING_H

#include <atomic>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace hw
{
    // TELLS THE CORE THAT THIS LOOP IS A SPIN, NOT REAL WORK. IT DRAINS THE
    // SPECULATION THAT A TIGHT POLL OTHERWISE BUILDS UP AND, ON SMT PARTS, HANDS
    // THE PIPELINE TO THE OTHER THREAD FOR A FEW CYCLES.
    inline void relax()
    {
#if defined(__aarch64__) || defined(_M_ARM64)
        __asm__ __volatile__("yield" ::: "memory");
#elif defined(_MSC_VER)
        _mm_pause();
#elif defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#else
        std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
    }

    // HOW MANY THREADS THIS MACHINE SHOULD ACTUALLY RUN.
    //
    // NOT hardware_concurrency(). EVERY PASS IN THE SOLVER ENDS IN A BARRIER, SO A
    // POOL IS ONLY AS FAST AS ITS SLOWEST MEMBER. ON A HYBRID CPU AN EFFICIENCY
    // CORE CHEWS THROUGH THIS WORKLOAD AT ROUGHLY A THIRD THE RATE OF A
    // PERFORMANCE CORE, SO GIVING IT AN EQUAL SHARE MAKES EVERY FAST CORE SIT AND
    // WAIT FOR IT AND THE WHOLE FRAME RUNS AT THE SLOW CLUSTER'S PACE. SIZING TO
    // THE FAST CLUSTER IS FASTER *AND* LEAVES THE SLOW ONE FOR THE OS, THE GL
    // DRIVER AND RAYLIB'S OWN THREADS.
    //
    // OVERRIDE WITH THE PARTICLE_THREADS ENVIRONMENT VARIABLE TO MEASURE SCALING.
    uint32_t suggestedThreadCount();
}

// A FIXED POOL OF SPINNING WORKERS WITH NO QUEUE, NO ALLOCATION AND NO MUTEX ON
// THE FAST PATH.
//
// THE SOLVER DISPATCHES AROUND FIFTY TIMES PER FRAME, SO DISPATCH LATENCY IS A
// FIRST-CLASS COST HERE: A CONVENTIONAL condition_variable POOL COSTS 10-50 us TO
// WAKE AND WOULD SPEND MORE TIME WAKING UP THAN WORKING. WORKERS INSTEAD POLL ONE
// TICKET WORD AND ONLY FALL BACK TO A REAL BLOCKING WAIT ONCE A DISPATCH HAS NOT
// ARRIVED FOR A WHILE, WHICH IN PRACTICE ONLY HAPPENS WHILE THE MAIN THREAD IS
// RENDERING AND WAITING ON VSYNC.
// ONE PER WORKER, ONE PER CACHE LINE. A SINGLE SHARED "HOW MANY ARE DONE"
// COUNTER WOULD BE A CONTENDED READ-MODIFY-WRITE ON EVERY BARRIER; THESE ARE
// WRITTEN BY EXACTLY ONE CORE EACH AND ONLY READ BY THE MAIN THREAD.
struct alignas(64) ThreadDoneFlag
{
    std::atomic<uint32_t> value{0};
};

class ThreadPool
{
public:
    static constexpr uint32_t MAX_THREADS = 32;

    // BRINGS UP (threads - 1) WORKERS. THE CALLING THREAD IS WORKER 0 AND TAKES
    // PART IN EVERY DISPATCH, SO NO CORE IS EVER PARKED WAITING ON ITSELF.
    static void start(uint32_t threads);
    static void stop();

    static uint32_t threadCount() { return liveThreads; }

    // RUNS body(workerIndex) ON EVERY WORKER AND RETURNS ONLY ONCE ALL OF THEM
    // HAVE FINISHED. body IS PASSED BY ADDRESS, SO NOTHING IS COPIED OR ALLOCATED.
    template <typename Body>
    static void dispatch(const Body &body)
    {
        if (liveThreads <= 1u)
        {
            body(0u);
            return;
        }

        submit([](uint32_t worker, void *ctx) { (*static_cast<const Body *>(ctx))(worker); },
               const_cast<void *>(static_cast<const void *>(&body)));
    }

    // RUNS body(begin, end) OVER A CONTIGUOUS SLICE OF [0, items) PER WORKER,
    // OR ONCE ON THE CALLING THREAD IF THERE IS NOT ENOUGH WORK TO PAY FOR THE
    // BARRIER.
    template <typename Body>
    static void parallelFor(const uint32_t items, const uint32_t minParallel, const Body &body)
    {
        if (liveThreads <= 1u || items < minParallel)
        {
            if (items > 0u)
            {
                body(0u, items);
            }
            return;
        }

        dispatch([items, &body](uint32_t worker) {
            uint32_t begin;
            uint32_t end;
            sliceOf(items, worker, liveThreads, begin, end);
            if (begin < end)
            {
                body(begin, end);
            }
        });
    }

    // SPLITS [0, items) INTO CONTIGUOUS RUNS WHOSE EDGES LAND ON 64-BYTE
    // BOUNDARIES FOR 4-BYTE ELEMENTS.
    //
    // THE ALIGNMENT IS THE WHOLE POINT. AN ARBITRARY SPLIT LEAVES ONE CACHE LINE
    // STRADDLING EVERY BOUNDARY, WRITTEN BY TWO DIFFERENT CORES; THAT LINE THEN
    // BOUNCES BETWEEN THEIR L1s FOR THE ENTIRE LOOP. SNAPPING TO 16 ELEMENTS
    // MEANS EVERY LINE OF EVERY PARTICLE ARRAY HAS EXACTLY ONE OWNER.
    static void sliceOf(const uint32_t items, const uint32_t worker, const uint32_t workers,
                        uint32_t &begin, uint32_t &end)
    {
        constexpr uint32_t GRAIN = 16u; // 64 BYTES / sizeof(float)

        const uint32_t blocks = (items + GRAIN - 1u) / GRAIN;
        const uint32_t share = blocks / workers;
        const uint32_t extra = blocks % workers;

        const uint32_t firstBlock = worker * share + (worker < extra ? worker : extra);
        const uint32_t lastBlock = firstBlock + share + (worker < extra ? 1u : 0u);

        const uint32_t rawBegin = firstBlock * GRAIN;
        const uint32_t rawEnd = lastBlock * GRAIN;

        begin = (rawBegin < items) ? rawBegin : items;
        end = (rawEnd < items) ? rawEnd : items;
    }

private:
    using RawJob = void (*)(uint32_t worker, void *ctx);

    static void submit(RawJob job, void *ctx);
    static void workerMain(uint32_t index);

    alignas(64) inline static std::atomic<uint32_t> ticket{0};
    alignas(64) inline static RawJob currentJob = nullptr;
    inline static void *currentCtx = nullptr;
    inline static std::atomic<bool> quitting{false};
    inline static uint32_t liveThreads = 1;
    inline static ThreadDoneFlag done[MAX_THREADS];
};

#endif
