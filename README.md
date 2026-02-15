#  CPU Particle Simulation (Optimized)

A high-performance, deterministic physics engine written in C++.
This project is a case study in **Data-Oriented Design**. By restructuring memory access and algorithmic complexity, I achieved a **24x performance increase** over the initial implementation.

---

##  The Numbers

I rewrote my simulation engine from scratch. Same machine. Same target FPS. Drastically different results.

| Metric | Old Version | **New Version** |
| --- | --- | --- |
| **Particle Count** | ~700 | **17,000+** |
| **FPS** | 60 | **60** |
| **Speedup** | 1x | **24x** |

### The "Hard Mode" Constraints

To ensure this was a test of raw engineering efficiency, I imposed strict rules:

*  **No Multithreading:** Everything runs on a single CPU core.
* ❌ **No GPU Compute:** No Compute Shaders or CUDA. Pure CPU physics.
* ❌ **No Cheats:** Every particle checks for collisions with relevant neighbors. 8 sub-steps per frame.

---

##  The Optimization Strategy

This isn't just "faster code." It is a fundamental shift in architecture.

### 1. Data-Oriented Design (SoA)

Instead of an `Array of Structures` (AoS) where a `Particle` object holds its own position, velocity, and color, I utilize a `Structure of Arrays` (SoA).

* **Result:** Positions, velocities, and accelerations are split into contiguous arrays.
* **Benefit:** Predictable memory access patterns that maximize cache-line utilization and allow for compiler auto-vectorization.

### 2. The "No Heap" Manifesto

`new` and `malloc` are banned in the hot loop.

* All data lives in **static, fixed-size arrays**.
* **Zero allocator overhead.**
* **Zero pointer chasing.**
* **Zero heap fragmentation.**
* Far better cache behavior than scattered heap objects.

### 3. Spatial Hashing & Counting Sort

The naive approach checks every particle against every other particle ().

* **Solution:** A Uniform Grid partitions the world.
* **The Trick:** I use **Counting Sort ()** to sort particles by their cell index every frame.
* **Benefit:** Collision checks only "walk" linear memory to find neighbors. We stop thrashing the cache and start streaming it.

### 4. Verlet Integration

Replaced Euler integration with Verlet.

* **Benefit:** Extremely stable at high forces and larger timesteps, reducing the need for expensive corrective passes.

---

##  Project Structure

The codebase is kept intentionally flat to avoid "header spaghetti."

```text
├── src/
│   ├── main.cpp         # Entry point & Loop
│   ├── solver.cpp       # The Physics Engine (Verlet + Grid)
│   ├── renderer.cpp     # Visualization (SFML/Raylib)
│   └── particle_data.h  # SoA Data Structures

```

---

## Prerequisites

### All Platforms

* **C++11 compatible compiler**
* **CMake ≥ 3.11**

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

---

## Building the Project

### 1. Clone the Repository

```bash
git clone ukasha167/Particle-Simulation
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

### 4. Compile

```bash
cmake --build .
```

This produces a **native binary** for your platform.

---

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

---

##  Future Roadmap

While the current version hits 17k on a single thread, the architecture is ready for the next level:

* **Multithreading:** The grid-based collision system is naturally parallelizable. 
* **Compute Shaders:** Moving the Verlet integration to the GPU for 1M+ particles.

---

##  License

MIT License. Feel free to fork, learn, and optimize.
