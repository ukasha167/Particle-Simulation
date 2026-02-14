#include <raylib.h>
#include <array>
#include <cmath>
#include <algorithm>

#include "renderer.h"
#include "defines.h"
#include "particle.cpp"

Texture2D Renderer::sprite;

void Renderer::loadAsset(const char *imageName, const int diameter)
{
    Image img = LoadImage(imageName); // DEFAULT IMAGE WIDTH IS 1024x1024
    ImageResize(&img, diameter, diameter);

    sprite = LoadTextureFromImage(img);
    UnloadImage(img);

    Particle::radius = diameter >> 1;
    SetTextureFilter(sprite, TEXTURE_FILTER_BILINEAR);
}

void Renderer::drawParticles(const std::array<Vector2, TOTAL_PARTICLE_COUNT> &positions, const int &currentCount)
{
    for (uint16_t i = 0; i < currentCount; i++)
    {
        Vector2 drawPos = {
            positions[i].x - Particle::radius,
            positions[i].y - Particle::radius};

        DrawTextureV(sprite, drawPos, WHITE);
    }
}

void Renderer::drawStats(const int fps, const int &currentCount)
{
    DrawText(TextFormat("FPS: %d", fps), 10, 10, 24, WHITE);
    DrawText(TextFormat("Particles: %d", currentCount), 10, 35, 24, WHITE);
}

void Renderer::unloadAsset()
{
    if (sprite.id > 0)
    {
        UnloadTexture(sprite);
    }
}