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
    for (uint16_t i = 0; i < currentParticlesCount; i++)
    {
        // THE SOLVER CLASS HAS BEEN USING THE CENTER POSITION AS THE ORIGIN OF PARTICLE
        // AND SINCE WE'RE USING A CUSTOM TEXTURE WE NEED, WE DON'T NEED OUR TO START RENDERING FROM THE CENTER, WE WANT IT START RENDERING FROM THE TOP LEFT
        // SO WE CALCULATE THE RENDERING POINT BY SUBTRACTING THE RADIUS FROM THE XY ORIGIN
        DrawTextureV(sprite, {particles.posX[i] - particles.radius, particles.posY[i] - particles.radius}, particles.color[i]);
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