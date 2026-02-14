#ifndef DEFINES_H
#define DEFINES_H

#include <iostream>

// LIMIT OF 2 BYTES: uint16_t = 0 - 65,535 (INCLUSIVE)
// LIMIT OF 4 BYTES: uint32_t = 0 - 4,294,967,295 (INCLUSIVE)
// LIMIT OF 8 BYTES: uint64_t = 0 - 18,446,744,073,709,551,615 (INCLUSIVE)

// SCREEN RELATED
constexpr uint16_t SCREEN_WIDTH = 1660;
constexpr uint16_t SCREEN_HEIGHT = 980;
// constexpr uint16_t SCREEN_WIDTH = 600;
// constexpr uint16_t SCREEN_HEIGHT = 400;

constexpr uint16_t HALF_SCREEN_WIDTH = SCREEN_WIDTH >> 1;
constexpr uint16_t HALF_SCREEN_HEIGHT = SCREEN_HEIGHT >> 1;

// PARTICLES RELATED
constexpr uint16_t TOTAL_PARTICLE_COUNT = 65534;
constexpr uint16_t MAX_PARTICLE_RADIUS = 15;
constexpr uint16_t CELL_SIZE = MAX_PARTICLE_RADIUS * 2;
constexpr uint16_t GRID_WIDTH = (SCREEN_WIDTH / CELL_SIZE) + 1;
constexpr uint16_t GRID_HEIGHT = (SCREEN_HEIGHT / CELL_SIZE) + 1;

// RESISTIVE FORCES
constexpr uint16_t GRAVITY = 650;
constexpr float DAMPENING = 0.75f;
constexpr float FRICTION = 0.9f;

// HELPING MATHEMATICAL VALUES
constexpr float PI_VALUE = 3.141592653F;

#endif