// galaxy.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel, Fridger Schrempp, and Toti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <fmt/printf.h>

#include <celmath/intersect.h>
#include <celmath/ray.h>
#include <celutil/gettext.h>
#include "galaxy.h"
#include "galaxyform.h"
#include "render.h"

using namespace std::literals::string_view_literals;

using celestia::engine::GalacticFormManager;

struct GalaxyTypeName
{
    const char* name;
    GalaxyType type;
};

constexpr std::array GalaxyTypeNames =
{
    GalaxyTypeName{ "Irr", GalaxyType::Irr },
    GalaxyTypeName{ "S0",  GalaxyType::S0 },
    GalaxyTypeName{ "Sa",  GalaxyType::Sa },
    GalaxyTypeName{ "Sb",  GalaxyType::Sb },
    GalaxyTypeName{ "Sc",  GalaxyType::Sc },
    GalaxyTypeName{ "SBa", GalaxyType::SBa },
    GalaxyTypeName{ "SBb", GalaxyType::SBb },
    GalaxyTypeName{ "SBc", GalaxyType::SBc },
    GalaxyTypeName{ "E0",  GalaxyType::E0 },
    GalaxyTypeName{ "E1",  GalaxyType::E1 },
    GalaxyTypeName{ "E2",  GalaxyType::E2 },
    GalaxyTypeName{ "E3",  GalaxyType::E3 },
    GalaxyTypeName{ "E4",  GalaxyType::E4 },
    GalaxyTypeName{ "E5",  GalaxyType::E5 },
    GalaxyTypeName{ "E6",  GalaxyType::E6 },
    GalaxyTypeName{ "E7",  GalaxyType::E7 },
};

float Galaxy::lightGain = 0.0f;

float Galaxy::getDetail() const
{
    return detail;
}

void Galaxy::setDetail(float d)
{
    detail = d;
}

const char* Galaxy::getType() const
{
    return GalaxyTypeNames[static_cast<std::size_t>(type)].name;
}

void Galaxy::setType(const std::string& typeStr)
{
    type = GalaxyType::Irr;
    auto iter = std::find_if(std::begin(GalaxyTypeNames), std::end(GalaxyTypeNames),
                             [&](const GalaxyTypeName& g) { return g.name == typeStr; });
    if (iter != std::end(GalaxyTypeNames))
        type = iter->type;
}

void Galaxy::setForm(const fs::path& customTmpName, const fs::path& resDir)
{
    if (customTmpName.empty())
    {
        form = static_cast<int>(type);
    }
    else
    {
        if (fs::path fullName = resDir / customTmpName; fs::exists(fullName))
            form = GalacticFormManager::get()->getCustomForm(fullName);
        else
            form = GalacticFormManager::get()->getCustomForm(fs::path("models") / customTmpName);
    }
}

std::string Galaxy::getDescription() const
{
    return fmt::sprintf(_("Galaxy (Hubble type: %s)"), getType());
}

const char* Galaxy::getObjTypeName() const
{
    return "galaxy";
}

int Galaxy::getFormId() const
{
    return form;
}

bool Galaxy::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    const auto* galacticForm = GalacticFormManager::get()->getForm(form);
    if (galacticForm == nullptr || !isVisible())
        return false;

    // The ellipsoid should be slightly larger to compensate for the fact
    // that blobs are considered points when galaxies are built, but have size
    // when they are drawn.
    float yscale = (type > GalaxyType::Irr && type < GalaxyType::E0)
        ? kMaxSpiralThickness
        : galacticForm->scale.y() + kRadiusCorrection;
    Eigen::Vector3d ellipsoidAxes(getRadius()*(galacticForm->scale.x() + kRadiusCorrection),
                                  getRadius()* yscale,
                                  getRadius()*(galacticForm->scale.z() + kRadiusCorrection));
    Eigen::Matrix3d rotation = getOrientation().cast<double>().toRotationMatrix();

    return celmath::testIntersection(
        celmath::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - getPosition(), ray.direction()),
                              rotation),
        celmath::Ellipsoidd(ellipsoidAxes),
        distanceToPicker,
        cosAngleToBoundCenter);
}

bool Galaxy::load(const AssociativeArray* params, const fs::path& resPath)
{
    setDetail(params->getNumber<float>("Detail").value_or(1.0f));

    if (const auto* typeName = params->getString("Type"); typeName == nullptr)
        setType({});
    else
        setType(*typeName);


    if (auto customTmpName = params->getPath("CustomTemplate"sv); customTmpName.has_value())
        setForm(customTmpName.value(), resPath);
    else
        setForm({});

    return DeepSkyObject::load(params, resPath);
}

GalaxyType Galaxy::getGalaxyType() const
{
    return type;
}

float Galaxy::getBrightnessCorrection(const Eigen::Vector3f &offset) const
{
    Eigen::Quaternionf orientation = getOrientation().conjugate();

    // corrections to avoid excessive brightening if viewed e.g. edge-on
    float brightness_corr = 1.0f;
    if (type < GalaxyType::E0 || type > GalaxyType::E3) // all galaxies, except ~round elliptics
    {
        float cosi = (orientation * Eigen::Vector3f::UnitY()).dot(offset) / offset.norm();
        brightness_corr = std::max(0.2f, std::sqrt(std::abs(cosi)));
    }
    if (type > GalaxyType::E3) // only elliptics with higher ellipticities
    {
        float cosi = (orientation * Eigen::Vector3f::UnitX()).dot(offset) / offset.norm();
        brightness_corr = std::max(0.45f, brightness_corr * std::abs(cosi));
    }

    float btot = (type == GalaxyType::Irr || type >= GalaxyType::E0) ? 2.5f : 5.0f;
    return (4.0f * lightGain + 1.0f) * btot * brightness_corr;
}

std::uint64_t Galaxy::getRenderMask() const
{
    return Renderer::ShowGalaxies;
}

unsigned int Galaxy::getLabelMask() const
{
    return Renderer::GalaxyLabels;
}

void Galaxy::increaseLightGain()
{
    lightGain = std::min(1.0f, lightGain + 0.05f);
}

void Galaxy::decreaseLightGain()
{
    lightGain = std::max(0.0f, lightGain - 0.05f);
}

float Galaxy::getLightGain()
{
    return lightGain;
}

void Galaxy::setLightGain(float lg)
{
    lightGain = std::clamp(lg, 0.0f, 1.0f);
}

std::ostream& operator<<(std::ostream& s, const GalaxyType& sc)
{
    return s << GalaxyTypeNames[static_cast<std::size_t>(sc)].name;
}
