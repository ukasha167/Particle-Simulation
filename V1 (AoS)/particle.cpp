#ifndef PARTICLES_H
#define PARTICLES_H

#include "defines.h"
#include "raylib.h"

struct Particle
{
    Vector2 position = {0.0f, 0.0f};
    Vector2 velocity = {0.0f, 0.0f};

    static constexpr float radius = 5.0f;

    Particle()
    {
        position = {HALF_SCREEN_WIDTH, HALF_SCREEN_HEIGHT};
        velocity = {-350.0f, 200.0f};
    }

    explicit Particle(const Vector2 &pos, const Vector2 &vel)
        : position(pos), velocity(vel) {}

    explicit Particle(const Vector2 pos, const Vector2 vel)
        : position(pos), velocity(vel) {}

    void updatePosition(const float &dt)
    {
        velocity.y += GRAVITY * dt;

        // FIX MICRO-JITTERS
        if (velocity.x < 0.1f && velocity.x > -0.1f)
            velocity.x = 0.0f;
        if (velocity.y < 0.1f && velocity.y > -0.1f)
            velocity.y = 0.0f;

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
    }
};

#endif