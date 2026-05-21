#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

#include "texture.h"

class Renderer;

class ImageOverlay
{
 public:
    // Opaque handle assigned by OverlayManager at insertion time. Returned to
    // celx callers so they can remove a specific image later. 0 is reserved
    // as "no id."
    using Id = std::uint64_t;

    ImageOverlay(const std::filesystem::path&, Renderer*);
    ImageOverlay()               = delete;
    ~ImageOverlay()              = default;
    ImageOverlay(ImageOverlay&)  = delete;
    ImageOverlay(ImageOverlay&&) = delete;

    void render(float, int, int);
    // True once the image has fully faded out and no longer renders;
    // mirrors the early-return check inside render().
    bool isExpired(float curr_time) const
    {
        return curr_time >= start + duration;
    }
    Id id() const { return imageId; }
    void setId(Id i) { imageId = i; }
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
    // Override the image's drawn size in pixels. A non-positive value
    // means "use the texture's intrinsic size for that axis." When
    // fitscreen is true the rendered image is rescaled to fit the
    // viewport, so explicit sizes only affect non-fitscreen rendering.
    void setSize(float w, float h)
    {
        overrideWidth  = w;
        overrideHeight = h;
    }
    void setColor(const Color& c);
    void setColor(std::array<Color, 4>& c);

 private:
    float start          { 0.0f };
    float duration       { 0.0f };
    float fadeafter      { 0.0f };
    float offsetX        { 0.0f };
    float offsetY        { 0.0f };
    // <=0 means "use the texture's intrinsic size for that axis."
    float overrideWidth  { 0.0f };
    float overrideHeight { 0.0f };
    bool  fitscreen      { false };
    Id    imageId        { 0 };
    std::array<Color, 4> colors;

    std::unique_ptr<Texture> texture;
    Renderer *renderer { nullptr };
};
