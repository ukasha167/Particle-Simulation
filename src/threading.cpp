#include "threading.hpp"

#include <cstdlib>
#include <thread>

#if defined(__APPLE__)
#include <pthread.h>
#include <sys/qos.h>
#include <sys/sysctl.h>
#endif

namespace
{
    std::thread pool[ThreadPool::MAX_THREADS];

    // HOW LONG A WORKER POLLS BEFORE IT GIVES THE CORE BACK. DISPATCHES INSIDE A
    // FRAME ARRIVE TENS OF MICROSECONDS APART SO THE POLL ALWAYS WINS THERE; THE
    // ONLY GAP LONG ENOUGH TO REACH THE BLOCKING PATH IS THE RENDER-AND-VSYNC
    // STRETCH AT THE END OF EACH FRAME, WHICH COSTS ONE WAKE-UP PER WORKER PER
    // FRAME AND KEEPS IDLE CPU (AND ON A LAPTOP, THE FANS) DOWN.
    constexpr uint32_t SPIN_BUDGET = 24000u;

    void applyThreadPriority()
    {
#if defined(__APPLE__)
        // macOS HAS NO AFFINITY API, BUT QOS IS A STRONG HINT TO THE SCHEDULER TO
        // KEEP THIS THREAD ON A PERFORMANCE CORE. WITHOUT IT THE WORKERS DRIFT
        // ONTO EFFICIENCY CORES AND EVERY BARRIER WAITS FOR THEM.
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
    }
}

uint32_t hw::suggestedThreadCount()
{
    if (const char *override = std::getenv("PARTICLE_THREADS"))
    {
        const int requested = std::atoi(override);
        if (requested > 0)
        {
            return static_cast<uint32_t>(requested);
        }
    }

#if defined(__APPLE__)
    int32_t cores = 0;
    size_t size = sizeof(cores);

    // PERFORMANCE CLUSTER ONLY. ABSENT ON INTEL MACS, WHERE WE FALL THROUGH.
    if (sysctlbyname("hw.perflevel0.physicalcpu", &cores, &size, nullptr, 0) == 0 && cores > 0)
    {
        return static_cast<uint32_t>(cores);
    }

    size = sizeof(cores);
    if (sysctlbyname("hw.physicalcpu", &cores, &size, nullptr, 0) == 0 && cores > 0)
    {
        return static_cast<uint32_t>(cores);
    }
#endif

    const uint32_t reported = std::thread::hardware_concurrency();
    return (reported > 0u) ? reported : 1u;
}

void ThreadPool::start(uint32_t threads)
{
    if (liveThreads > 1u)
    {
        return;
    }

    if (threads < 1u)
    {
        threads = 1u;
    }
    if (threads > MAX_THREADS)
    {
        threads = MAX_THREADS;
    }

    quitting.store(false, std::memory_order_relaxed);
    ticket.store(0u, std::memory_order_relaxed);

    for (uint32_t i = 0; i < threads; i++)
    {
        done[i].value.store(0u, std::memory_order_relaxed);
    }

    liveThreads = threads;
    applyThreadPriority(); // THE MAIN THREAD IS WORKER 0 AND WANTS THE SAME TREATMENT

    for (uint32_t i = 1; i < threads; i++)
    {
        pool[i] = std::thread(&ThreadPool::workerMain, i);
    }
}

void ThreadPool::stop()
{
    if (liveThreads <= 1u)
    {
        return;
    }

    quitting.store(true, std::memory_order_release);
    ticket.fetch_add(1u, std::memory_order_release);
    ticket.notify_all();

    for (uint32_t i = 1; i < liveThreads; i++)
    {
        if (pool[i].joinable())
        {
            pool[i].join();
        }
    }

    liveThreads = 1;
}

void ThreadPool::submit(RawJob job, void *ctx)
{
    currentJob = job;
    currentCtx = ctx;

    // THE RELEASE STORE PUBLISHES job, ctx AND EVERYTHING THE CALLER WROTE BEFORE
    // IT; THE WORKERS' ACQUIRE LOAD OF THE SAME WORD PICKS IT ALL UP.
    const uint32_t stamp = ticket.load(std::memory_order_relaxed) + 1u;
    ticket.store(stamp, std::memory_order_release);
    ticket.notify_all();

    job(0u, ctx); // THE MAIN THREAD PULLS ITS OWN WEIGHT

    for (uint32_t i = 1; i < liveThreads; i++)
    {
        uint32_t spins = 0;
        while (done[i].value.load(std::memory_order_acquire) != stamp)
        {
            hw::relax();
            if (++spins >= SPIN_BUDGET)
            {
                std::this_thread::yield();
                spins = 0;
            }
        }
    }
}

void ThreadPool::workerMain(const uint32_t index)
{
    applyThreadPriority();

    uint32_t seen = 0;

    while (true)
    {
        uint32_t stamp = ticket.load(std::memory_order_acquire);
        uint32_t spins = 0;

        while (stamp == seen)
        {
            if (spins < SPIN_BUDGET)
            {
                hw::relax();
                ++spins;
            }
            else
            {
                ticket.wait(seen, std::memory_order_relaxed);
            }

            stamp = ticket.load(std::memory_order_acquire);
        }

        seen = stamp;

        if (quitting.load(std::memory_order_acquire))
        {
            break;
        }

        currentJob(index, currentCtx);
        done[index].value.store(stamp, std::memory_order_release);
    }
}
