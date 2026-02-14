# Particle Simulation Engine (C++)

A deterministic, CPU-bound particle simulation written in modern C++, built from first principles with **zero external dependencies**, designed to prioritize **memory locality, predictability, and architectural clarity** over convenience abstractions.

This project is not a demo.
It is an exploration of *how simulations should be built when you actually care about what the machine is doing*.

---


## Project Structure

```text
.
├── CMakeLists.txt
├── src/
│   ├── main.cpp        # Entry point
│   ├── solver.cpp      # Physics / simulation step
│   ├── solver.h
│   ├── renderer.cpp    # Visualization layer
│   ├── renderer.h
│   ├── particle.cpp    # Particle state & behavior
│   ├── particle.h
│   └── defines.h       # Compile-time configuration & constants
```

### Why this structure matters

* **Clear ownership of responsibilities**
* **No god files**
* **Scales cleanly as features grow**
* **Mirrors real-world engine layouts**

This is a small codebase wearing a large-codebase skeleton — intentionally.

---

## Build System: CMake

This project uses **CMake** as a meta-build system.

### Why CMake?

* Cross-platform by design
* Decouples build logic from compiler specifics
* Scales from tiny projects to massive codebases
* Industry standard for C++ tooling

The goal is simple:

> Anyone, on any OS, with a compiler, should be able to build this.

---

## Prerequisites

### All Platforms

* **C++17 compatible compiler**
* **CMake ≥ 3.15**

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

No runtime arguments are required by default.
Simulation parameters are intentionally controlled at **compile time** via `defines.h`.

---

## Core Design Principles

### 1. No External Libraries (On Purpose)

No physics engines.
No math libraries.
No utility headers.

**Why?**

External libraries:

* Obscure memory layout
* Introduce hidden allocations
* Add abstraction penalties
* Make performance characteristics harder to reason about

This project chooses **clarity over convenience**.

Everything that happens here is visible, debuggable, and under direct control.

---

### 2. Static & Stack-Based Allocation Over Heap Allocation

You will notice a **deliberate avoidance of dynamic heap allocation** in hot paths.

**Why the heap was avoided:**

* Heap allocation involves:

  * System calls or allocator locks
  * Fragmentation risks
  * Pointer chasing
* Heap memory destroys cache locality
* Allocation patterns become non-deterministic under pressure

**What was chosen instead:**

* Stack allocation where lifetimes are obvious
* Fixed-size containers where bounds are known
* Contiguous memory layouts for particles

This results in:

* Fewer cache misses
* Predictable memory access patterns
* Easier reasoning about performance

---

### 3. Separation of Simulation and Rendering

The simulation **does not depend on rendering**.

The renderer:

* Reads simulation state
* Displays it
* Has zero authority over physics

The solver:

* Advances time
* Updates particle state
* Knows nothing about visuals

This separation allows:

* Headless simulation
* Deterministic testing
* Renderer replacement without touching physics
* Future parallelization

---

### 4. Data-Oriented Thinking

Particles are stored contiguously.
State updates are linear.
Memory access is predictable.

The design leans toward:

* Cache-friendly traversal
* Minimal branching
* Tight loops
* Explicit control flow

No virtual dispatch.
No polymorphic hierarchies.
No hidden indirection.

---

## Intended Extensions

This architecture cleanly supports:

* Multi-threaded solvers
* SIMD optimization
* Alternative renderers
* Headless batch simulation
* Data export for analysis

None of these require rewriting the core.

---

## Final Note

This repository reflects a specific philosophy:

> **Understanding beats convenience.
> Control beats abstraction.
> Structure beats shortcuts.**

If you are looking for a simulation you can *reason about down to the cache line* — welcome.
