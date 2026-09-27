#  CPU Particle Simulation (Optimized)

A high-performance physics engine written in C++.
This project is a case study in **Data-Oriented Design**. My first version held about 700 particles at 60 FPS. Restructuring memory access got one thread to 17,000, and spreading the solver across 4 threads took it to **100,000 particles at 60 FPS** on an M1 Air.

### The Numbers

I rewrote my simulation engine from scratch. Same machine. Same target FPS. Drastically different results.

| Metric | Old Version | Single Thread | **Current** |
| --- | --- | --- | --- |
| **Particle Count** | ~700 | 17,000+ | **100,000** |
| **FPS** | 60 | 60 | **60** |
| **CPU Threads** | 1 | 1 | **4** |
| **Particles vs. Old** | 1x | 24x | **~140x** |

**Note:** The old version which I compared to, is not included in this repository. However I can explain why that was so slow:

*  It was rendered in SFML with not VertexBuffers, which means only one particle was drawn at a time. Raylib, however uses batch rendering by default.
*  It used Euler Integeration which is unstable at larger number of particles, and also introduced extra energy.
*  Algorithms of choice were very inefficient.
*  Data was not handled well, which caused severed cache misses.

### The "Hard Mode" Constraints

To ensure this was a test of raw engineering efficiency, I imposed strict rules:

*  **CPU Only, 4 Threads:** The solver runs on 4 worker threads, one per performance core of the M1.
*  **No GPU Compute:** No Compute Shaders or CUDA. Pure CPU physics.
*  **No Cheats:** Every particle checks for collisions with relevant neighbors. 18 sub-steps per frame.

### The Optimization Strategy

This isn't just "faster code." It is a fundamental shift in architecture.

### 1. Data-Oriented Design (SoA)

Instead of an `Array of Structures` (AoS), I utilized a `Structure of Arrays` (SoA).

* **Result:** Positions and velocities are split into contiguous arrays.
* **Benefit:** Predictable memory access patterns that maximize cache-line utilization and allow for compiler auto-vectorization.

### 2. The "No Heap" Manifesto

`new` and `malloc` are banned in the hot loop.

* All data lives in **static, fixed-size arrays**.
* **Zero allocator overhead.**
* **Zero pointer chasing.**
* **Zero heap fragmentation.**
* Far better cache behavior than scattered heap objects.

### 3. Spatial Hashing & Counting Sort

The naive approach checks every particle against every other particle **O(N^2)**.

* **Solution:** A Uniform Grid partitions the world into a discrete coordinate system.
* **The Trick:** I use **Counting Sort O(Number of elements + Grid cells)** to sort particles by their cell index every frame, This reorganizes the particle indices into contiguous memory blocks corresponding to their grid location.
* **Benefit:** Instead of jumping randomly through memory to check distant neighbors, the collision solver walks through linear blocks of data **O(N)**. This eliminates CPU cache thrashing and allows the processor to stream neighbor data at maximum bandwidth. 

**In short:** The collision algorithm that i'm using, behaves like a LinkedList of particles. And the **Counting Sort** is just making sure that the related particles' indices are contiguous like Array.

### 4. Explicit Velocity

Earlier versions used Verlet integration, where velocity is implied by the previous position. That made every collision fix also change the velocity, whether I wanted it to or not.

The current solver keeps velocity in its own arrays and uses semi-implicit Euler: add gravity to the velocity, then move the position by the velocity. Unlike the plain Euler in my first version, it doesn't pump extra energy into the system. The contact solver still feeds part of each position fix back into velocity, because that spring is what makes piles pop and splash, but one constant (`COLLISION_REACTION_LOSS`) now decides how much.

### 5. Multithreading

The solver runs on 4 threads, one per performance core of the M1.

* **Sized to the fast cores.** Every pass ends in a barrier, so the pool runs at the speed of its slowest thread. An efficiency core does this work at about a third of the speed, so the pool leaves them out (`hw.perflevel0.physicalcpu`) and they stay free for the OS and the renderer. Set `PARTICLE_THREADS` to try other counts.
* **A spinning pool.** The solver hands out work about fifty times per frame. A normal `condition_variable` pool takes 10-50 µs to wake up, so the workers spin on one atomic instead and only sleep when nothing has arrived for a while.
* **A parallel counting sort.** Each thread counts its own slice of particles into a private histogram, so no atomics are needed. The prefix sum gives every thread its own write cursors, and the result is byte-for-byte the same as the single-threaded sort.
* **Row bands for contacts.** The grid is cut into two bands of rows per thread, sized by the work in each row (the sum of n² over its cells) instead of the particle count. Odd bands run in one pass and even bands in the next, so two threads never touch neighbouring rows at the same time.

### Project Structure


```text
├── src/
│   ├── main.cpp            # Entry point & Loop
│   ├── solver.hpp/cpp      # The Physics Engine (Grid, Counting Sort, Contacts)
│   ├── threading.hpp/cpp   # Spinning Thread Pool
│   ├── renderer.hpp/cpp    # Visualization (Raylib)
│   ├── particle.hpp        # SoA Data Structures
│   └── defines.hpp         # Compile time parameters
├── CMakeLists.txt          # Build File
```

There are multiple versions of the project (V1-V7), The CMake will only compile the code inside the src folder.
The src folder holds the current version, which is newer than V7: it adds the thread pool. If you wish to run any older version, you can do that by:

Manually setting up the CMake
Or
Copy/Over wite all of the files into the src folder

## Prerequisites

* **C++23 compatible compiler**
* **CMake ≥ 3.25**

### Linux

```bash
sudo apt install build-essential cmake
```

### macOS

```bash
brew install cmake
```
(Xcode Command Line Tools required)

### Windows

* Visual Studio 2022 or newer
  (Install “Desktop development with C++”)
* CMake (bundled with VS or installed separately)


## Building the Project

### 1. Clone the Repository

```bash
git clone https://github.com/ukasha167/Particle-Simulation.git
cd Particle-Simulation
```

### 2. Create a Build Directory

```bash
mkdir build
cd build
```

### 3. Generate Build Files

```bash
cmake ..
```
This will download the raylib. It might look stuck for sometime if you have slow internet, but wait for a while it will eventually download :D 
(The actual size is ~100MB)

### 4. Compile

```bash
cmake --build .
```

This produces a **native binary** for your platform.


## Running the Simulation

From the `build` directory:

### Linux / macOS

```bash
./main
```

### Windows

```powershell
main.exe
```


### Future Roadmap

Multithreading took it from 17k on one thread to 100k on four. The next step:

* **Compute Shaders:** Moving the integration to the GPU for 1M+ particles.


### License

MIT License. Feel free to fork, learn, and optimize.