#include <algorithm>
#include <celmath/mathlib.h>
#include "imageoverlay.h"
#include "rectangle.h"
#include "render.h"

ImageOverlay::ImageOverlay(const std::filesystem::path& f, Renderer *r) :
    texture(LoadTextureFromFile(f.is_relative() ? "images" / f : f,
                                Texture::EdgeClamp,
                                Texture::NoMipMaps)),
    renderer(r)
{
}

void ImageOverlay::setColor(const Color& c)
{
    colors.fill(c);
}

void ImageOverlay::setColor(std::array<Color, 4>& c)
{
    std::copy(c.begin(), c.end(), colors.begin());
}

void ImageOverlay::render(float curr_time, int width, int height)
{
    if (renderer == nullptr || texture == nullptr || (curr_time >= start + duration))
        return;

    // Honor explicit width/height set via setSize(); fall back to the
    // texture's intrinsic dimensions when either is non-positive.
    float xSize = overrideWidth  > 0.0f ? overrideWidth  : static_cast<float>(texture->getWidth());
    float ySize = overrideHeight > 0.0f ? overrideHeight : static_cast<float>(texture->getHeight());

    float fwidth  = static_cast<float>(width);
    float fheight = static_cast<float>(height);

    // center overlay image horizontally if offsetX = 0
    float left = (fwidth * (1.0f + offsetX) - xSize) / 2.0f;
    // center overlay image vertically if offsetY = 0
    float bottom = (fheight * (1.0f + offsetY) - ySize) / 2.0f;

    if (fitscreen)
    {
        float coeffx = xSize / fwidth;  // overlay pict width/view window width ratio
        float coeffy = ySize / fheight; // overlay pict height/view window height ratio
        xSize /= coeffx;                // new overlay picture width size to fit viewport
        ySize /= coeffy;                // new overlay picture height to fit viewport

        left = (fwidth - xSize) / 2.0f; // to be sure overlay pict is centered in viewport
        bottom = 0.0f;                  // overlay pict locked at bottom of screen
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
    renderer->drawRectangle(r, FisheyeOverrideMode::Disabled, renderer->getOrthoProjectionMatrix());
}
