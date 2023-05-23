// qlobular.cpp
//
// Copyright (C) 2008, Celestia Development Team
// Initial code by Dr. Fridger Schrempp <fridger.schrempp@desy.de>
//
// Simulation of globular clusters, theoretical framework by
// Ivan King, Astron. J. 67 (1962) 471; ibid. 71 (1966) 64
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cmath>

#include <fmt/printf.h>

#include <celengine/hash.h>
#include <celengine/render.h>
#include <celmath/ellipsoid.h>
#include <celmath/intersect.h>
#include <celmath/randutils.h>
#include <celmath/ray.h>
#include <celutil/gettext.h>
#include "globular.h"

namespace
{

constexpr float kRadiusCorrection = 0.025f;

unsigned int cSlot(float conc)
{
    // map the physical range of c, minC <= c <= maxC,
    // to 8 integers (bin numbers), 0 < cSlot <= 7:
    conc = std::clamp(conc, Globular::MinC, Globular::MaxC);
    return static_cast<unsigned int>(std::floor((conc - Globular::MinC) / Globular::BinWidth));
}

} // end unnamed namespace

Globular::Globular()
{
    recomputeTidalRadius();
}

const char* Globular::getType() const
{
    return "Globular";
}

void Globular::setType(const std::string& /*typeStr*/)
{
}

float Globular::getHalfMassRadius() const
{
    // Aproximation to the half-mass radius r_h [ly]
    // (~ 20% accuracy)

    return std::tan(celmath::degToRad(r_c / 60.0f)) * static_cast<float>(getPosition().norm()) * std::pow(10.0f, 0.6f * c - 0.4f);
}

std::string Globular::getDescription() const
{
   return fmt::sprintf(_("Globular (core radius: %4.2f', King concentration: %4.2f)"), r_c, c);
}

DeepSkyObjectType Globular::getObjType() const
{
    return DeepSkyObjectType::Globular;
}

bool Globular::pick(const Eigen::ParametrizedLine<double, 3>& ray,
                    double& distanceToPicker,
                    double& cosAngleToBoundCenter) const
{
    if (!isVisible())
        return false;
    /*
     * The selection sphere should be slightly larger to compensate for the fact
     * that blobs are considered points when globulars are built, but have size
     * when they are drawn.
     */
    Eigen::Vector3d p = getPosition();
    return celmath::testIntersection(celmath::transformRay(Eigen::ParametrizedLine<double, 3>(ray.origin() - p, ray.direction()),
                                                           getOrientation().cast<double>().toRotationMatrix()),
                                     celmath::Sphered(getRadius() * (1.0f + kRadiusCorrection)),
                                     distanceToPicker,
                                     cosAngleToBoundCenter);
}

bool Globular::load(const AssociativeArray* params, const fs::path& resPath)
{
    // Load the basic DSO parameters first
    if (!DeepSkyObject::load(params, resPath))
        return false;

    if (auto detailVal = params->getNumber<float>("Detail"); detailVal.has_value())
        detail = *detailVal;

    if (auto coreRadius = params->getAngle<float>("CoreRadius", 1.0 / MINUTES_PER_DEG); coreRadius.has_value())
        r_c = *coreRadius;

    if (auto king = params->getNumber<float>("KingConcentration"); king.has_value())
        c = *king;

    formIndex = cSlot(c);
    recomputeTidalRadius();

    return true;
}

std::uint64_t Globular::getRenderMask() const
{
    return Renderer::ShowGlobulars;
}

unsigned int Globular::getLabelMask() const
{
    return Renderer::GlobularLabels;
}

void Globular::recomputeTidalRadius()
{
    // Convert the core radius from arcminutes to light years
    // Compute the tidal radius in light years

    float coreRadiusLy = std::tan(celmath::degToRad(r_c / 60.0f)) * static_cast<float>(getPosition().norm());
    tidalRadius = coreRadiusLy * std::pow(10.0f, c);
}

int Globular::getFormId() const
{
    return formIndex;
}

float Globular::getDetail() const
{
    return detail;
}

void Globular::setDetail(float _detail)
{
    detail = _detail;
}
