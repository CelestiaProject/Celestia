// skygridrenderer.cpp
//
// Celestial longitude/latitude grid renderer.
//
// Extracted from skygrid.cpp
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "skygridrenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#include <Eigen/Geometry>
#include <fmt/format.h>

#include <celcompat/numbers.h>
#include <celengine/render.h>
#include <celengine/skygrid.h>
#include <celmath/mathlib.h>
#include <celmath/geomutil.h>
#include <celmath/vecgl.h>
#include <celutil/color.h>
#include "linerenderer.h"

using namespace std::string_view_literals;

namespace celestia::render
{

namespace
{

// The maximum number of parallels or meridians that will be visible
constexpr double MAX_VISIBLE_ARCS = 10.0;

// Number of line segments used to approximate one arc of the celestial sphere
constexpr int ARC_SUBDIVISIONS = 100;

// Size of the cross indicating the north and south poles
constexpr double POLAR_CROSS_SIZE = 0.01;

// Grid line spacing tables
constexpr int MSEC = 1;
constexpr int SEC = 1000;
constexpr int MIN = 60 * SEC;
constexpr int DEG = 60 * MIN;
constexpr int HR  = 60 * MIN;

constexpr int HOUR_MIN_SEC_TOTAL = 24 * HR;
constexpr int DEG_MIN_SEC_TOTAL  = 180 * DEG;

constexpr std::array<int, 19> HOUR_MIN_SEC_SPACING
{
    100*MSEC,
    200*MSEC,
    500*MSEC,
    1*SEC,
    2*SEC,
    3*SEC,
    5*SEC,
    10*SEC,
    15*SEC,
    30*SEC,
    1*MIN,
    2*MIN,
    3*MIN,
    5*MIN,
    10*MIN,
    15*MIN,
    30*MIN,
    1*HR,
    2*HR,
};

constexpr std::array<int, 24> DEG_MIN_SEC_SPACING
{
    100*MSEC,
    200*MSEC,
    500*MSEC,
    1*SEC,
    2*SEC,
    3*SEC,
    5*SEC,
    10*SEC,
    15*SEC,
    30*SEC,
    1*MIN,
    2*MIN,
    3*MIN,
    5*MIN,
    10*MIN,
    15*MIN,
    30*MIN,
    1*DEG,
    2*DEG,
    3*DEG,
    5*DEG,
    10*DEG,
    15*DEG,
    30*DEG,
};

Eigen::Vector3d
toStandardCoords(const Eigen::Vector3d& v)
{
    return Eigen::Vector3d(v.x(), -v.z(), v.y());
}

Eigen::Vector3d
toCelestiaCoords(const Eigen::Vector3d& v)
{
    return Eigen::Vector3d(v.x(), v.z(), -v.y());
}

// Compute the difference between two angles in [-PI, PI]
double
angleDiff(double a, double b)
{
    double diff = std::fabs(a - b);
    if (diff > numbers::pi)
        return 2.0 * numbers::pi - diff;
    else
        return diff;
}

void
updateAngleRange(double a, double b, double& maxDiff, double& minAngle, double& maxAngle)
{
    if (angleDiff(a, b) > maxDiff)
    {
        maxDiff = angleDiff(a, b);
        minAngle = a;
        maxAngle = b;
    }
}

// Compute the angular step between parallels
int
parallelSpacing(double idealSpacing)
{
    // We want to use parallels and meridian spacings that are nice multiples of hours, degrees,
    // minutes, or seconds. Choose spacings from a table. We take the table entry that gives
    // the spacing closest to but not less than the ideal spacing.

    auto target = static_cast<int>(idealSpacing * static_cast<double>(DEG_MIN_SEC_TOTAL) * numbers::inv_pi);
    auto it = std::lower_bound(DEG_MIN_SEC_SPACING.begin(), DEG_MIN_SEC_SPACING.end(), target);
    return it == DEG_MIN_SEC_SPACING.end() ? DEG_MIN_SEC_TOTAL : *it;
}

// Compute the angular step between meridians
int
meridianSpacing(double idealSpacing, engine::SkyGrid::LongitudeUnits longitudeUnits)
{
    util::array_view<int> spacingTable;
    int totalUnits;

    // Use degree spacings if the latitude units are degrees instead of hours
    if (longitudeUnits == engine::SkyGrid::LongitudeDegrees)
    {
        spacingTable = DEG_MIN_SEC_SPACING;
        totalUnits = DEG_MIN_SEC_TOTAL * 2;
    }
    else
    {
        spacingTable = HOUR_MIN_SEC_SPACING;
        totalUnits = HOUR_MIN_SEC_TOTAL;
    }

    auto target = static_cast<int>(idealSpacing * static_cast<double>(totalUnits) * 0.5 * numbers::inv_pi);
    auto it = std::lower_bound(spacingTable.begin(), spacingTable.end(), target);
    return it == spacingTable.end() ? totalUnits : *it;
}

// Get the horizontal alignment for the coordinate label along the specified frustum plane
Renderer::LabelHorizontalAlignment
getCoordLabelHAlign(int planeIndex)
{
    switch (planeIndex)
    {
    case 2:
        return Renderer::LabelHorizontalAlignment::Start;
    case 3:
        return Renderer::LabelHorizontalAlignment::End;
    default:
        return Renderer::LabelHorizontalAlignment::Center;
    }
}

// Get the vertical alignment for the coordinate label along the specified frustum plane
Renderer::LabelVerticalAlignment
getCoordLabelVAlign(int planeIndex)
{
    return planeIndex == 1 ? Renderer::LabelVerticalAlignment::Top : Renderer::LabelVerticalAlignment::Bottom;
}

// Get the a string with a label for the specified latitude. Both
// the latitude and latitudeStep are given in milliarcseconds.
std::string
latitudeLabel(int latitude, int latitudeStep)
{
    using namespace std::string_literals;

    // Produce a sexigesimal string
    std::string result;
    if (latitude == 0)
    {
        result = "0°"s;
    }
    else
    {
        char signChar;
        if (latitude > 0)
        {
            signChar = '+';
        }
        else
        {
            signChar = '-';
            latitude = -latitude;
        }

        result = fmt::format("{}{}°", signChar, latitude / DEG);
    }

    if (latitudeStep % DEG == 0)
        return result;

    fmt::format_to(std::back_inserter(result), " {:02}′", (latitude / MIN) % 60);
    if (latitudeStep % MIN == 0)
        return result;

    fmt::format_to(std::back_inserter(result), " {:02}", (latitude / SEC) % 60);
    if (latitudeStep % SEC != 0)
        fmt::format_to(std::back_inserter(result), ".{:03}", latitude % SEC);

    result.append("″"sv);
    return result;
}

// Get the a string with a label for the specified longitude. Both
// the longitude and longitude are given in milliarcseconds.
std::string
longitudeLabel(int longitude, int longitudeStep,
               engine::SkyGrid::LongitudeUnits longitudeUnits,
               engine::SkyGrid::LongitudeDirection longitudeDirection)
{
    int totalUnits = HOUR_MIN_SEC_TOTAL;
    int baseUnit = HR;
    std::string_view baseUnitSymbol = "°"sv;
    std::string_view minuteSymbol = "m"sv;
    std::string_view secondSymbol = "s"sv;

    if (longitudeUnits == engine::SkyGrid::LongitudeDegrees)
    {
        totalUnits = DEG_MIN_SEC_TOTAL * 2;
        baseUnit = DEG;
        baseUnitSymbol = "°"sv;
        minuteSymbol = "′"sv;
        secondSymbol = "″"sv;
    }

    // Produce a sexigesimal string
    if (longitude < 0)
        longitude += totalUnits;

    // Reverse the labels if the longitude increases clockwise (e.g. for
    // horizontal coordinate grids, where azimuth is defined to increase
    // eastward from due north.
    if (longitudeDirection == engine::SkyGrid::IncreasingClockwise)
        longitude = (totalUnits - longitude) % totalUnits;

    std::string result = fmt::format("{}{}", longitude / baseUnit, baseUnitSymbol);
    if (longitudeStep % baseUnit == 0)
        return result;

    fmt::format_to(std::back_inserter(result), " {:02}{}", (longitude / MIN) % 60, minuteSymbol);
    if (longitudeStep % MIN == 0)
        return result;

    fmt::format_to(std::back_inserter(result), " {:02}", (longitude / SEC) % 60);
    if (longitudeStep % SEC != 0)
        fmt::format_to(std::back_inserter(result), ".{:03}", longitude % SEC);

    result.append(secondSymbol);
    return result;
}

} // end unnamed namespace

struct SkyGridRenderer::RenderInfo
{
    RenderInfo(double, double, const Eigen::Quaterniond&, const engine::SkyGrid&);

    double minDec;
    double maxDec;
    double minTheta;
    double maxTheta;
    double idealParallelSpacing;
    double idealMeridianSpacing;
    Eigen::Quaternionf orientationf;
    std::array<Eigen::Vector3d, 4> frustumNormal;
    float polarCrossSize;
};

SkyGridRenderer::RenderInfo::RenderInfo(double vfov, double viewAspectRatio,
                                        const Eigen::Quaterniond& cameraOrientation,
                                        const engine::SkyGrid& grid)
{
    // Calculate the cosine of half the maximum field of view. We'll use this for
    // fast testing of marker visibility. The stored field of view is the
    // vertical field of view; we want the field of view as measured on the
    // diagonal between viewport corners.
    double h = std::tan(vfov / 2);
    double w = h * viewAspectRatio;
    double diag = std::sqrt(1.0 + math::square(h) + math::square(h * viewAspectRatio));
    double cosHalfFov = 1.0 / diag;
    double halfFov = std::acos(cosHalfFov);

    polarCrossSize = static_cast<float>(POLAR_CROSS_SIZE * halfFov);

    // We want to avoid drawing more of the grid than we have to. The following code
    // determines the region of the grid intersected by the view frustum. We're
    // interested in the minimum and maximum phi and theta of the visible patch
    // of the celestial sphere.

    // Find the minimum and maximum theta (longitude) by finding the smallest
    // longitude range containing all corners of the view frustum.

    // View frustum corners
    Eigen::Vector3d c0(-w, -h, -1.0);
    Eigen::Vector3d c1( w, -h, -1.0);
    Eigen::Vector3d c2(-w,  h, -1.0);
    Eigen::Vector3d c3( w,  h, -1.0);


    // 90 degree rotation about the x-axis used to transform coordinates
    // to Celestia's system.
    Eigen::Matrix3d r = (cameraOrientation *
                         math::XRot90Conjugate<double> *
                         grid.orientation.conjugate() *
                         math::XRot90<double>).toRotationMatrix().transpose();

    // Transform the frustum corners by the camera and grid
    // rotations.
    c0 = toStandardCoords(r * c0);
    c1 = toStandardCoords(r * c1);
    c2 = toStandardCoords(r * c2);
    c3 = toStandardCoords(r * c3);

    double thetaC0 = std::atan2(c0.y(), c0.x());
    double thetaC1 = std::atan2(c1.y(), c1.x());
    double thetaC2 = std::atan2(c2.y(), c2.x());
    double thetaC3 = std::atan2(c3.y(), c3.x());

    // Compute the minimum longitude range containing the corners; slightly
    // tricky because of the wrapping at PI/-PI.
    minTheta = thetaC0;
    maxTheta = thetaC1;
    double maxDiff = 0.0;
    updateAngleRange(thetaC0, thetaC1, maxDiff, minTheta, maxTheta);
    updateAngleRange(thetaC0, thetaC2, maxDiff, minTheta, maxTheta);
    updateAngleRange(thetaC0, thetaC3, maxDiff, minTheta, maxTheta);
    updateAngleRange(thetaC1, thetaC2, maxDiff, minTheta, maxTheta);
    updateAngleRange(thetaC1, thetaC3, maxDiff, minTheta, maxTheta);
    updateAngleRange(thetaC2, thetaC3, maxDiff, minTheta, maxTheta);

    if (std::fabs(maxTheta - minTheta) < numbers::pi)
    {
        if (minTheta > maxTheta)
            std::swap(minTheta, maxTheta);
    }
    else
    {
        if (maxTheta > minTheta)
            std::swap(minTheta, maxTheta);
    }
    maxTheta = minTheta + maxDiff;

    // Calculate the normals to the view frustum planes; we'll use these to
    // when computing intersection points with the parallels and meridians of the
    // grid. Coordinate labels will be drawn at the intersection points.
    frustumNormal[0] = Eigen::Vector3d( 0.0,  1.0, -h);
    frustumNormal[1] = Eigen::Vector3d( 0.0, -1.0, -h);
    frustumNormal[2] = Eigen::Vector3d( 1.0,  0.0, -w);
    frustumNormal[3] = Eigen::Vector3d(-1.0,  0.0, -w);

    for (auto& fn : frustumNormal)
    {
        fn = toStandardCoords(r * fn.normalized());
    }

    Eigen::Vector3d viewCenter(-Eigen::Vector3d::UnitZ());
    viewCenter = toStandardCoords(r * viewCenter);

    double centerDec;
    if (std::fabs(viewCenter.z()) < 1.0)
        centerDec = std::asin(viewCenter.z());
    else if (viewCenter.z() < 0.0)
        centerDec = -numbers::pi * 0.5;
    else
        centerDec = numbers::pi * 0.5;

    minDec = centerDec - halfFov;
    maxDec = centerDec + halfFov;

    if (maxDec >= numbers::pi * 0.5)
    {
        // view cone contains north pole
        maxDec = numbers::pi * 0.5;
        minTheta = -numbers::pi;
        maxTheta = numbers::pi;
    }
    else if (minDec <= -numbers::pi * 0.5)
    {
        // view cone contains south pole
        minDec = -numbers::pi * 0.5;
        minTheta = -numbers::pi;
        maxTheta = numbers::pi;
    }

    idealParallelSpacing = 2.0 * halfFov / MAX_VISIBLE_ARCS;
    idealMeridianSpacing = idealParallelSpacing;

    // Adjust the spacing between meridians based on how close the view direction
    // is to the poles; the density of meridians increases as we approach the pole,
    // so we want to increase the angular distance between meridians.
    // Choose spacing based on the minimum declination (closest to zero)
    double minAbsDec = std::min(std::fabs(minDec), std::fabs(maxDec));
    if (minDec * maxDec <= 0.0f) // Check if min and max straddle the equator
        minAbsDec = 0.0f;
    idealMeridianSpacing /= std::cos(minAbsDec);

    // Get the orientation at single precision
    Eigen::Quaterniond q = math::XRot90Conjugate<double> * grid.orientation * math::XRot90<double>;
    orientationf = q.cast<float>();
}

SkyGridRenderer::SkyGridRenderer(Renderer& renderer) :
    m_gridRenderer(std::make_unique<LineRenderer>(renderer, 1.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Stream)),
    m_crossRenderer(std::make_unique<LineRenderer>(renderer, 1.0f, LineRenderer::PrimType::Lines, LineRenderer::StorageType::Stream)),
    m_renderer(renderer)
{
}

SkyGridRenderer::~SkyGridRenderer() = default;

void
SkyGridRenderer::render(const engine::SkyGrid& grid, float zoom) const
{
    auto vfov = static_cast<double>(m_renderer.getProjectionMode()->getFOV(zoom));
    double viewAspectRatio = static_cast<double>(m_renderer.getWindowWidth()) / static_cast<double>(m_renderer.getWindowHeight());
    Eigen::Quaterniond cameraOrientation = m_renderer.getCameraOrientation();

    RenderInfo renderInfo(vfov, viewAspectRatio, cameraOrientation, grid);

    int decIncrement = parallelSpacing(renderInfo.idealParallelSpacing);
    Eigen::Matrix3f cameraMatrix = cameraOrientation.cast<float>().toRotationMatrix();

    m_gridRenderer->startUpdate();

    int count = drawParallels(renderInfo, cameraMatrix, grid.labelColor, decIncrement) +
                drawMeridians(renderInfo, cameraMatrix, grid, decIncrement);

    // Radius of sphere is arbitrary, with the constraint that it shouldn't
    // intersect the near or far plane of the view frustum.
    Eigen::Matrix4f m = m_renderer.getModelViewMatrix() *
                        math::rotate((math::XRot90Conjugate<double> *
                                      grid.orientation.conjugate() *
                                      math::XRot90<double>).cast<float>()) *
                        math::scale(1000.0f);
    Matrices matrices = {&m_renderer.getProjectionMatrix(), &m};

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    m_renderer.setPipelineState(ps);

    for (int offset = 0, i = 0; i < count; ++i)
    {
        m_gridRenderer->render(matrices, grid.lineColor, ARC_SUBDIVISIONS + 1, offset);
        offset += ARC_SUBDIVISIONS + 1;
    }

    // Draw crosses indicating the north and south poles
    m_crossRenderer->startUpdate();
    m_crossRenderer->addVertex(-renderInfo.polarCrossSize,  1.0f,  0.0f);
    m_crossRenderer->addVertex( renderInfo.polarCrossSize,  1.0f,  0.0f);
    m_crossRenderer->addVertex( 0.0f,                       1.0f, -renderInfo.polarCrossSize);
    m_crossRenderer->addVertex( 0.0f,                       1.0f,  renderInfo.polarCrossSize);
    m_crossRenderer->addVertex(-renderInfo.polarCrossSize, -1.0f,  0.0f);
    m_crossRenderer->addVertex( renderInfo.polarCrossSize, -1.0f,  0.0f);
    m_crossRenderer->addVertex( 0.0f,                      -1.0f, -renderInfo.polarCrossSize);
    m_crossRenderer->addVertex( 0.0f,                      -1.0f,  renderInfo.polarCrossSize);
    m_crossRenderer->render(matrices, grid.lineColor, 8);

    m_gridRenderer->clear();
    m_crossRenderer->clear();
    m_gridRenderer->finish();
    m_crossRenderer->finish();
}

int
SkyGridRenderer::drawParallels(const RenderInfo& renderInfo,
                               const Eigen::Matrix3f& cameraMatrix,
                               const Color& labelColor,
                               int decIncrement) const
{
    double arcStep = (renderInfo.maxTheta - renderInfo.minTheta) / static_cast<double>(ARC_SUBDIVISIONS);
    double theta0 = renderInfo.minTheta;

    auto startDec = static_cast<int>(std::ceil (DEG_MIN_SEC_TOTAL * (renderInfo.minDec * numbers::inv_pi) / static_cast<double>(decIncrement))) * decIncrement;
    auto endDec   = static_cast<int>(std::floor(DEG_MIN_SEC_TOTAL * (renderInfo.maxDec * numbers::inv_pi) / static_cast<double>(decIncrement))) * decIncrement;

    int count = 0;
    for (int dec = startDec; dec <= endDec; dec += decIncrement)
    {
        ++count;
        double phi = numbers::pi * static_cast<double>(dec) / static_cast<double>(DEG_MIN_SEC_TOTAL);
        double cosPhi;
        double sinPhi;
        math::sincos(phi, sinPhi, cosPhi);

        for (int j = 0; j <= ARC_SUBDIVISIONS; ++j)
        {
            double theta = theta0 + j * arcStep;
            double cosTheta;
            double sinTheta;
            math::sincos(theta, sinTheta, cosTheta);
            auto x = static_cast<float>(cosPhi * cosTheta);
            auto y = static_cast<float>(cosPhi * sinTheta);
            auto z = static_cast<float>(sinPhi);
            m_gridRenderer->addVertex(x, z, -y);  // convert to Celestia coords
        }

        // Place labels at the intersections of the view frustum planes
        // and the parallels.
        Eigen::Vector3d center(0.0, 0.0, sinPhi);
        Eigen::Vector3d axis0(cosPhi, 0.0, 0.0);
        Eigen::Vector3d axis1(0.0, cosPhi, 0.0);
        for (int k = 0; k < 4; k += 2)
        {
            Eigen::Vector3d isect0(Eigen::Vector3d::Zero());
            Eigen::Vector3d isect1(Eigen::Vector3d::Zero());

            if (!math::planeCircleIntersection(renderInfo.frustumNormal[k], center, axis0, axis1, &isect0, &isect1))
                continue;

            std::string labelText = latitudeLabel(dec, decIncrement);

            Eigen::Vector3f p0(toCelestiaCoords(isect0).cast<float>());
            Eigen::Vector3f p1(toCelestiaCoords(isect1).cast<float>());

            p0 = renderInfo.orientationf.conjugate() * p0;
            p1 = renderInfo.orientationf.conjugate() * p1;
            Renderer::LabelHorizontalAlignment hAlign = getCoordLabelHAlign(k);
            Renderer::LabelVerticalAlignment vAlign = getCoordLabelVAlign(k);

            if ((cameraMatrix * p0).z() < 0.0)
                m_renderer.addBackgroundAnnotation(nullptr, labelText, labelColor, p0, hAlign, vAlign);

            if ((cameraMatrix * p1).z() < 0.0)
                m_renderer.addBackgroundAnnotation(nullptr, labelText, labelColor, p1, hAlign, vAlign);
        }
    }

    return count;
}

int
SkyGridRenderer::drawMeridians(const RenderInfo& renderInfo,
                               const Eigen::Matrix3f& cameraMatrix,
                               const engine::SkyGrid& grid,
                               int decIncrement) const
{
    int totalLongitudeUnits = grid.longitudeUnits == engine::SkyGrid::LongitudeDegrees ? (DEG_MIN_SEC_TOTAL * 2) : HOUR_MIN_SEC_TOTAL;
    int raIncrement  = meridianSpacing(renderInfo.idealMeridianSpacing, grid.longitudeUnits);
    auto startRa  = static_cast<int>(std::ceil (totalLongitudeUnits * (renderInfo.minTheta * 0.5 * numbers::inv_pi) / static_cast<double>(raIncrement))) * raIncrement;
    auto endRa    = static_cast<int>(std::floor(totalLongitudeUnits * (renderInfo.maxTheta * 0.5 * numbers::inv_pi) / static_cast<double>(raIncrement))) * raIncrement;

    // Render meridians only to the last latitude circle; this looks better
    // than spokes radiating from the pole.
    double maxMeridianAngle = numbers::pi * 0.5 * (1.0 - 2.0 * static_cast<double>(decIncrement) / static_cast<double>(DEG_MIN_SEC_TOTAL));
    double minDec = std::max(renderInfo.minDec, -maxMeridianAngle);
    double maxDec = std::min(renderInfo.maxDec,  maxMeridianAngle);
    double arcStep = (maxDec - minDec) / static_cast<double>(ARC_SUBDIVISIONS);
    double phi0 = minDec;

    double cosMaxMeridianAngle = std::cos(maxMeridianAngle);

    int count = 0;
    for (int ra = startRa; ra <= endRa; ra += raIncrement)
    {
        ++count;
        double theta = 2.0 * numbers::pi * (double) ra / (double) totalLongitudeUnits;
        double cosTheta;
        double sinTheta;
        math::sincos(theta, sinTheta, cosTheta);

        for (int j = 0; j <= ARC_SUBDIVISIONS; ++j)
        {
            double phi = phi0 + j * arcStep;
            double cosPhi;
            double sinPhi;
            math::sincos(phi, sinPhi, cosPhi);
            auto x = static_cast<float>(cosPhi * cosTheta);
            auto y = static_cast<float>(cosPhi * sinTheta);
            auto z = static_cast<float>(sinPhi);
            m_gridRenderer->addVertex(x, z, -y);  // convert to Celestia coords
        }

        // Place labels at the intersections of the view frustum planes
        // and the meridians.
        Eigen::Vector3d center(Eigen::Vector3d::Zero());
        Eigen::Vector3d axis0(cosTheta, sinTheta, 0.0);
        Eigen::Vector3d axis1(Eigen::Vector3d::UnitZ());
        for (int k = 1; k < 4; k += 2)
        {
            Eigen::Vector3d isect0(Eigen::Vector3d::Zero());
            Eigen::Vector3d isect1(Eigen::Vector3d::Zero());

            if (!math::planeCircleIntersection(renderInfo.frustumNormal[k], center, axis0, axis1, &isect0, &isect1))
                continue;

            std::string labelText = longitudeLabel(ra, raIncrement, grid.longitudeUnits, grid.longitudeDirection);

            Eigen::Vector3f p0(toCelestiaCoords(isect0).cast<float>());
            Eigen::Vector3f p1(toCelestiaCoords(isect1).cast<float>());

            p0 = renderInfo.orientationf.conjugate() * p0;
            p1 = renderInfo.orientationf.conjugate() * p1;
            Renderer::LabelHorizontalAlignment hAlign = getCoordLabelHAlign(k);
            Renderer::LabelVerticalAlignment vAlign = getCoordLabelVAlign(k);

            if ((cameraMatrix * p0).z() < 0.0 && axis0.dot(isect0) >= cosMaxMeridianAngle)
                m_renderer.addBackgroundAnnotation(nullptr, labelText, grid.labelColor, p0, hAlign, vAlign);

            if ((cameraMatrix * p1).z() < 0.0 && axis0.dot(isect1) >= cosMaxMeridianAngle)
                m_renderer.addBackgroundAnnotation(nullptr, labelText, grid.labelColor, p1, hAlign, vAlign);
        }
    }

    return count;
}

} // end namespace celestia::render
