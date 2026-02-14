#ifndef DEFINES_H
#define DEFINES_H

#include <iostream>
#include <cstdint>

#include "raylib.h"

// LIMIT OF 1 BYTE:  uint8_t  = 0 - 255 (INCLUSIVE)
// LIMIT OF 2 BYTES: uint16_t = 0 - 65,535 (INCLUSIVE)
// LIMIT OF 4 BYTES: uint32_t = 0 - 4,294,967,295 (INCLUSIVE)
// LIMIT OF 8 BYTES: uint64_t = 0 - 18,446,744,073,709,551,615 (INCLUSIVE)

constexpr uint16_t TOTAL_PARTICLE_COUNT = 40000;

constexpr uint16_t SCREEN_WIDTH = 1660;
constexpr uint16_t SCREEN_HEIGHT = 980;
constexpr uint16_t HALF_SCREEN_WIDTH = SCREEN_WIDTH >> 1;
constexpr uint16_t HALF_SCREEN_HEIGHT = SCREEN_HEIGHT >> 1;

constexpr float MAX_PARTICLE_RADIUS = 5.0f;
constexpr uint16_t CELL_SIZE = (uint16_t)(MAX_PARTICLE_RADIUS * 2);
constexpr uint16_t GRID_WIDTH = (SCREEN_WIDTH / CELL_SIZE) + 1;
constexpr uint16_t GRID_HEIGHT = (SCREEN_HEIGHT / CELL_SIZE) + 1;

constexpr float GRAVITY = 1000.0f;

inline Vector2 operator-(const Vector2 &vec1, const Vector2 &vec2)
{
    return {vec1.x - vec2.x, vec1.y - vec2.y};
}
inline Vector2 operator+(const Vector2 &vec1, const Vector2 &vec2)
{
    return {vec1.x + vec2.x, vec1.y + vec2.y};
}
inline Vector2 operator*(const Vector2 &vec, float scalar)
{
    return {vec.x * scalar, vec.y * scalar};
}

#endif