#include "renderer.h"

Texture2D Renderer::sprite;

void Renderer::loadAsset(const char *imageName, const uint8_t &diameter)
{
    Image img = LoadImage(imageName); // DEFAULT IMAGE WIDTH IS 1024x1024
    ImageResize(&img, diameter, diameter);

    sprite = LoadTextureFromImage(img);
    UnloadImage(img);

    SetTextureFilter(sprite, TEXTURE_FILTER_BILINEAR);
}

void Renderer::drawParticles(const stdArray &posX, const stdArray &posY, const uint16_t &currentParticlesCount)
{
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        DrawTextureV(sprite, {posX[i] - Particle::radius, posY[i] - Particle::radius}, WHITE);
    }
}

void Renderer::drawStats(const uint16_t fps, const uint16_t &currentParticlesCount)
{
    DrawText(TextFormat("FPS: %d", fps), 10, 10, 24, WHITE);
    DrawText(TextFormat("Particles: %d", currentParticlesCount), 10, 35, 24, WHITE);
}

void Renderer::unloadAsset()
{
    if (sprite.id > 0)
    {
        UnloadTexture(sprite);
    }
}