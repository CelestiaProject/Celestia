// renderdialog.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// Based on the Qt interface
// Copyright (C) 2007-2008, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "renderdialog.h"

#include <imgui.h>

#include <celengine/render.h>
#include <celengine/starcolors.h>
#include <celengine/texmanager.h>
#include <celestia/celestiacore.h>
#include "helpers.h"

namespace celestia::sdl
{

namespace
{

void
texturesPanel(Renderer* renderer)
{
    ImGui::Text("Texture resolution");
    auto resolution = static_cast<int>(renderer->getResolution());
    int resolutionNew = resolution;

    ImGui::RadioButton("Low##lowResolution", &resolutionNew, static_cast<int>(engine::TextureResolution::lores));
    ImGui::RadioButton("Medium##medResolution", &resolutionNew, static_cast<int>(engine::TextureResolution::medres));
    ImGui::RadioButton("High##highResolution", &resolutionNew, static_cast<int>(engine::TextureResolution::hires));

    if (resolutionNew != resolution)
        renderer->setResolution(static_cast<engine::TextureResolution>(resolutionNew));
}

void
lightingPanel(Renderer* renderer)
{
    float ambient = renderer->getAmbientLightLevel();
    float ambientNew = ambient;

    float tintSaturation = renderer->getTintSaturation();
    float tintSaturationNew = tintSaturation;

    ImGui::DragFloat("Ambient light", &ambientNew, 0.01f, 0.0f, 1.0f, "%.2f");
    ImGui::DragFloat("Illumination tint", &tintSaturationNew, 0.01f, 0.0f, 1.0f, "%.2f");

    if (ambientNew != ambient)
        renderer->setAmbientLightLevel(ambientNew);

    if (tintSaturationNew != tintSaturation)
        renderer->setTintSaturation(tintSaturationNew);
}

void
starStylePanel(Renderer* renderer)
{
    auto starStyle = static_cast<int>(renderer->getStarStyle());
    int starStyleNew = starStyle;
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;
    auto starColors = static_cast<int>(renderer->getStarColorTable());
    int starColorsNew = starColors;

    ImGui::Text("Star style");
    ImGui::RadioButton("Points##starStylePoints", &starStyleNew, static_cast<int>(StarStyle::PointStars));
    ImGui::RadioButton("Fuzzy points", &starStyleNew, static_cast<int>(StarStyle::FuzzyPointStars));
    ImGui::RadioButton("Scaled discs", &starStyleNew, static_cast<int>(StarStyle::ScaledDiscStars));
    ImGui::Separator();
    enumCheckbox("Auto-magnitude", rfNew, RenderFlags::ShowAutoMag);

    constexpr std::array<const char*, 4> names =
    {
        "Classic colors",
        "Blackbody D65",
        "Blackbody (Sun white)",
        "Blackbody (Vega white)",
    };

    ImGui::Combo("Star colors", &starColorsNew, names.data(), static_cast<int>(names.size()));

    if (starStyleNew != starStyle)
        renderer->setStarStyle(static_cast<StarStyle>(starStyleNew));
    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);
    if (starColorsNew != starColors)
        renderer->setStarColorTable(static_cast<ColorTableType>(starColorsNew));
}

void
miscellaneousPanel(Renderer* renderer)
{
    RenderFlags rf = renderer->getRenderFlags();
    RenderFlags rfNew = rf;

#ifdef GL_ES
    ImGui::Text("Render path: OpenGL ES");
#else
    ImGui::Text("Render path: OpenGL");
#endif

    enumCheckbox("Anti-aliased lines", rf, RenderFlags::ShowSmoothLines);

    if (rfNew != rf)
        renderer->setRenderFlags(rfNew);
}

} // end unnamed namespace

void renderDialog(const CelestiaCore* appCore, bool* isOpen)
{
    if (!*isOpen)
        return;

    Renderer* renderer = appCore->getRenderer();

    if (ImGui::Begin("Render", isOpen))
    {
        if (ImGui::CollapsingHeader("Textures"))
            texturesPanel(renderer);

        if (ImGui::CollapsingHeader("Lighting"))
            lightingPanel(renderer);

        if (ImGui::CollapsingHeader("Star style"))
            starStylePanel(renderer);

        if (ImGui::CollapsingHeader("Miscellaneous"))
            miscellaneousPanel(renderer);
    }

    ImGui::End();
}

} // end namespace celestia::sdl
