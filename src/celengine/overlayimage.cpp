#include <algorithm>
#include <iostream>
#include <celmath/mathlib.h>
#include "overlayimage.h"
#include "rectangle.h"
#include "render.h"

using namespace celmath;

OverlayImage::OverlayImage(fs::path f, Renderer *r) :
    filename(std::move(f)),
    renderer(r)
{
    texture = std::unique_ptr<Texture>(LoadTextureFromFile(fs::path("images") / filename,
                                                           Texture::EdgeClamp,
                                                           Texture::NoMipMaps));
}

void OverlayImage::setColor(const Color& c)
{
    colors.fill(c);
}

void OverlayImage::setColor(std::array<Color, 4>& c)
{
    std::copy(c.begin(), c.end(), colors.begin());
}

void OverlayImage::render(float curr_time, int width, int height)
{
    if (renderer == nullptr || texture == nullptr || (curr_time >= start + duration))
        return;

    float xSize = texture->getWidth();
    float ySize = texture->getHeight();

    // center overlay image horizontally if offsetX = 0
    float left = (width * (1 + offsetX) - xSize)/2;
    // center overlay image vertically if offsetY = 0
    float bottom = (height * (1 + offsetY) - ySize)/2;

    if (fitscreen)
    {
        float coeffx = xSize / width;  // overlay pict width/view window width ratio
        float coeffy = ySize / height; // overlay pict height/view window height ratio
        xSize /= coeffx;               // new overlay picture width size to fit viewport
        ySize /= coeffy;               // new overlay picture height to fit viewport

        left = (width - xSize) / 2;    // to be sure overlay pict is centered in viewport
        bottom = 0;                    // overlay pict locked at bottom of screen
    }

    float alpha = 1.0f;
    if (curr_time > start + fadeafter)
    {
        alpha = std::clamp(start + duration - curr_time, 0.0f, 1.0f);
    }

    celestia::Rect r(left, bottom, xSize, ySize);
    r.tex = texture.get();
    for (size_t i = 0; i < colors.size(); i++)
    {
        r.colors[i] = Color(colors[i], colors[i].alpha() * alpha);
    }
    r.hasColors = true;
    renderer->drawRectangle(r, ShaderProperties::FisheyeOverrideModeDisabled, renderer->getOrthoProjectionMatrix());
}
