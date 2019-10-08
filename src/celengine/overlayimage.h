#pragma once

#include <array>
#include <memory>
#include <celcompat/filesystem.h>
#include "texture.h"

class Renderer;

class OverlayImage
{
 public:
    OverlayImage(fs::path, Renderer*);
    OverlayImage()               = delete;
    ~OverlayImage()              = default;
    OverlayImage(OverlayImage&)  = delete;
    OverlayImage(OverlayImage&&) = delete;

    void render(float, int, int);
    bool isNewImage(const fs::path& f) const
    {
        return filename != f;
    }
    void setStartTime(float t)
    {
        start = t;
    }
    void setDuration(float t)
    {
        duration = t;
    }
    void setFadeAfter(float t)
    {
        fadeafter = t;
    }
    void setOffset(float x, float y)
    {
        offsetX = x;
        offsetY = y;
    }
    void fitScreen(bool t)
    {
        fitscreen = t;
    }
    void setColor(const Color& c);
    void setColor(std::array<Color, 4>& c);

 private:
    float start     { 0.0f };
    float duration  { 0.0f };
    float fadeafter { 0.0f };
    float offsetX   { 0.0f };
    float offsetY   { 0.0f };
    bool  fitscreen { false };
    std::array<Color, 4> colors;

    fs::path filename;
    std::unique_ptr<Texture> texture;
    Renderer *renderer { nullptr };
};
