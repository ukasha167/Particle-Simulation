#include "renderer.h"

Texture2D Renderer::sprite;

uint8_t Renderer::loadAsset(const char *imageName, const uint8_t diameter)
{
    Image img = LoadImage(imageName); // DEFAULT IMAGE WIDTH IS 1024x1024

    if (diameter <= 0 || img.data == nullptr)
    {
        return 1;
    }

    // THE IMAGE IS QUITE LARGE, WE NEED TO RESIZE IT
    ImageResize(&img, diameter, diameter);

    // LOAD IT TO TEXTURE MEMORY (PROBABLY GPU)
    sprite = LoadTextureFromImage(img);
    SetTextureFilter(sprite, TEXTURE_FILTER_BILINEAR);

    UnloadImage(img);
    return 0;
}

void Renderer::drawParticles(const Particle &particles, const uint16_t currentParticlesCount)
{
    Rectangle source = {0.0f, 0.0f, (float)sprite.width, (float)sprite.height};

    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // WHERE AND HOW BIG TO RENDER
        Rectangle dest = {particles.posX[i], particles.posY[i], particles.rad[i] * 2.0f, particles.rad[i] * 2.0f};

        // CENTER OF THE PARTICLE
        Vector2 origin = {(float)particles.rad[i], (float)particles.rad[i]};

        DrawTexturePro(sprite, source, dest, origin, 0.0f, particles.color[i]);
    }
}

void Renderer::drawStats(const uint16_t fps, const uint16_t currentParticlesCount)
{
    DrawText(TextFormat("FPS: %d\nParticles: %d", fps, currentParticlesCount), 10, 10, 24, WHITE);
}

void Renderer::unloadAsset()
{
    if (sprite.id > 0)
    {
        UnloadTexture(sprite);
    }
}