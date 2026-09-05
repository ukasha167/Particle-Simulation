#include "renderer.hpp"

#include <rlgl.h>

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

void Renderer::drawParticles(const Particle &particles, const uint32_t currentParticlesCount)
{
    if (sprite.id == 0)
    {
        return;
    }

    // ONE BATCH FOR THE WHOLE FIELD.
    //
    // DrawTextureV() BOTTOMS OUT IN DrawTexturePro(), WHICH OPENS AND CLOSES ITS OWN
    // rlBegin/rlEnd AND RE-BINDS THE TEXTURE FOR *EVERY SINGLE PARTICLE*. THE QUADS
    // IT EMITS ARE IDENTICAL TO THESE ONES, SO WE EMIT THEM OURSELVES AND PAY THAT
    // SETUP COST ONCE FOR THE FRAME INSTEAD OF THIRTY-TWO THOUSAND TIMES.
    // rlVertex3f() FLUSHES THE BATCH ON ITS OWN WHEN THE VERTEX BUFFER FILLS, SO
    // THERE IS NO LIMIT ON HOW MANY QUADS MAY GO INTO ONE rlBegin BLOCK.
    const float *__restrict posX = particles.posX;
    const float *__restrict posY = particles.posY;
    const Color *__restrict color = particles.color;

    rlSetTexture(sprite.id);
    rlBegin(RL_QUADS);

    rlNormal3f(0.0f, 0.0f, 1.0f); // NORMAL POINTING TOWARDS THE VIEWER

    for (uint32_t i = 0; i < currentParticlesCount; i++)
    {
        const Color tint = color[i];
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        const float left = posX[i] - Particle::radius;
        const float top = posY[i] - Particle::radius;
        const float right = left + PARTICLE_DIAMETER;
        const float bottom = top + PARTICLE_DIAMETER;

        rlTexCoord2f(0.0f, 0.0f); rlVertex2f(left, top);
        rlTexCoord2f(0.0f, 1.0f); rlVertex2f(left, bottom);
        rlTexCoord2f(1.0f, 1.0f); rlVertex2f(right, bottom);
        rlTexCoord2f(1.0f, 0.0f); rlVertex2f(right, top);
    }

    rlEnd();
    rlSetTexture(0);
}

void Renderer::drawStats(const uint16_t fps, const uint32_t currentParticlesCount)
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
