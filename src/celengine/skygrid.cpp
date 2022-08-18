// skygrid.cpp
//
// Celestial longitude/latitude grids.
//
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <celcompat/numbers.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include "render.h"
#include "vecgl.h"
#include "skygrid.h"

using namespace Eigen;
using namespace std;
using namespace celmath;
using namespace celestia;
using celestia::engine::LineRenderer;

LineRenderer *SkyGrid::g_gridRenderer = nullptr;
LineRenderer *SkyGrid::g_crossRenderer = nullptr;

// #define DEBUG_LABEL_PLACEMENT

// The maximum number of parallels or meridians that will be visible
constexpr double MAX_VISIBLE_ARCS = 10.0;

// Number of line segments used to approximate one arc of the celestial sphere
constexpr int ARC_SUBDIVISIONS = 100;

// Size of the cross indicating the north and south poles
const double POLAR_CROSS_SIZE = 0.01;

// Grid line spacing tables
constexpr int MSEC = 1;
constexpr int SEC = 1000;
constexpr int MIN = 60 * SEC;
constexpr int DEG = 60 * MIN;
constexpr int HR  = 60 * MIN;

constexpr int HOUR_MIN_SEC_TOTAL = 24 * HR;
constexpr int DEG_MIN_SEC_TOTAL  = 180 * DEG;

static const int HOUR_MIN_SEC_SPACING[] =
{
    2*HR,
    1*HR,
    30*MIN,
    15*MIN,
    10*MIN,
    5*MIN,
    3*MIN,
    2*MIN,
    1*MIN,
    30*SEC,
    15*SEC,
    10*SEC,
    5*SEC,
    3*SEC,
    2*SEC,
    1*SEC,
    500*MSEC,
    200*MSEC,
    100*MSEC
};

static const int DEG_MIN_SEC_SPACING[]  =
{
    30*DEG,
    15*DEG,
    10*DEG,
    5*DEG,
    3*DEG,
    2*DEG,
    1*DEG,
    30*MIN,
    15*MIN,
    10*MIN,
    5*MIN,
    3*MIN,
    2*MIN,
    1*MIN,
    30*SEC,
    15*SEC,
    10*SEC,
    5*SEC,
    3*SEC,
    2*SEC,
    1*SEC,
    500*MSEC,
    200*MSEC,
    100*MSEC
};


// Alternate spacing tables
#if 0
// Max step between spacings is 5x; all spacings are
// integer multiples of subsequent spacings.
static const int HOUR_MIN_SEC_SPACING[] =
{
    2*HR,   1*HR,  30*MIN, 10*MIN,  5*MIN,
    1*MIN, 30*SEC, 10*SEC,  5*SEC,  1*SEC,
    500*MSEC, 100*MSEC
};

static const int DEG_MIN_SEC_SPACING[]  =
{
    30*DEG, 10*DEG,  5*DEG,  1*DEG, 30*MIN,
    10*MIN,  5*MIN,  1*MIN, 30*SEC, 10*SEC,
    5*SEC,  1*SEC, 500*MSEC, 100*MSEC
};
#endif

#if 0
// Max step between spacings is 3x
static const int HOUR_MIN_SEC_STEPS[] =
{
    2*HR,   1*HR,  30*MIN, 10*MIN,  5*MIN, 2*MIN+30*SEC,
    1*MIN, 30*SEC, 10*SEC,  5*SEC, 2*SEC+500*MSEC, 1*SEC,
    500*MSEC, 200*MSEC, 100*MSEC, 50*MSEC, 20*MSEC, 10*MSEC
};

static const int DEG_MIN_SEC_STEPS[]  =
{
    30*DEG, 10*DEG,  5*DEG, 2*DEG+30*MIN, 1*DEG, 30*MIN,
    10*MIN,  5*MIN, 2*MIN+30*SEC, 1*MIN, 30*SEC, 10*SEC,
    5*SEC, 2*SEC+500*MSEC, 1*SEC, 500*MSEC, 200*MSEC, 100*MSEC,
    50*MSEC, 20*MSEC, 10*MSEC
};
#endif


static Vector3d
toStandardCoords(const Vector3d& v)
{
    return Vector3d(v.x(), -v.z(), v.y());
}

static Vector3d
toCelestiaCoords(const Vector3d& v)
{
    return Vector3d(v.x(), v.z(), -v.y());
}

// Compute the difference between two angles in [-PI, PI]
template<class T> static T
angleDiff(T a, T b)
{
    using celestia::numbers::pi_v;
    T diff = std::fabs(a - b);
    if (diff > pi_v<T>)
        return (T) (2.0 * pi_v<T> - diff);
    else
        return diff;
}


static void updateAngleRange(double a, double b, double* maxDiff, double* minAngle, double* maxAngle)
{
    if (angleDiff(a, b) > *maxDiff)
    {
        *maxDiff = angleDiff(a, b);
        *minAngle = a;
        *maxAngle = b;
    }
}


// Get the horizontal alignment for the coordinate label along the specified frustum plane
static Renderer::LabelAlignment
getCoordLabelHAlign(int planeIndex)
{
    switch (planeIndex)
    {
    case 2:
        return Renderer::AlignLeft;
    case 3:
        return Renderer::AlignRight;
    default:
        return Renderer::AlignCenter;
    }
}


// Get the vertical alignment for the coordinate label along the specified frustum plane
static Renderer::LabelVerticalAlignment
getCoordLabelVAlign(int planeIndex)
{
    return planeIndex == 1 ? Renderer::VerticalAlignTop : Renderer::VerticalAlignBottom;
}


// Get the a string with a label for the specified latitude. Both
// the latitude and latitudeStep are given in milliarcseconds.
string
SkyGrid::latitudeLabel(int latitude, int latitudeStep) const
{
    // Produce a sexigesimal string
    ostringstream out;
    if (latitude < 0)
        out << '-';
    out << std::abs(latitude / DEG) << UTF8_DEGREE_SIGN;
    if (latitudeStep % DEG != 0)
    {
        out << ' ' << setw(2) << setfill('0') << std::abs((latitude / MIN) % 60) << '\'';
        if (latitudeStep % MIN != 0)
        {
            out << ' ' << setw(2) << setfill('0') << std::abs((latitude / SEC) % 60);
            if (latitudeStep % SEC != 0)
                out << '.' << setw(3) << setfill('0') << latitude % SEC;
            out << '"';
        }
    }

    return out.str();
}


// Get the a string with a label for the specified longitude. Both
// the longitude and longitude are given in milliarcseconds.
string
SkyGrid::longitudeLabel(int longitude, int longitudeStep) const
{
    int totalUnits = HOUR_MIN_SEC_TOTAL;
    int baseUnit = HR;
    const char* baseUnitSymbol = "h";
    char minuteSymbol = 'm';
    char secondSymbol = 's';

    if (m_longitudeUnits == LongitudeDegrees)
    {
        totalUnits = DEG_MIN_SEC_TOTAL * 2;
        baseUnit = DEG;
        baseUnitSymbol = UTF8_DEGREE_SIGN;
        minuteSymbol = '\'';
        secondSymbol = '"';
    }

    // Produce a sexigesimal string
    ostringstream out;
    if (longitude < 0)
        longitude += totalUnits;

    // Reverse the labels if the longitude increases clockwise (e.g. for
    // horizontal coordinate grids, where azimuth is defined to increase
    // eastward from due north.
    if (m_longitudeDirection == IncreasingClockwise)
        longitude = (totalUnits - longitude) % totalUnits;

    out << longitude / baseUnit << baseUnitSymbol;
    if (longitudeStep % baseUnit != 0)
    {
        out << ' ' << setw(2) << setfill('0') << (longitude / MIN) % 60 << minuteSymbol;
        if (longitudeStep % MIN != 0)
        {
            out << ' ' << setw(2) << setfill('0') << (longitude / SEC) % 60;
            if (longitudeStep % SEC != 0)
                out << '.' << setw(3) << setfill('0') << longitude % SEC;
            out << secondSymbol;
        }
    }

    return out.str();
}


// Compute the angular step between parallels
int
SkyGrid::parallelSpacing(double idealSpacing) const
{
    // We want to use parallels and meridian spacings that are nice multiples of hours, degrees,
    // minutes, or seconds. Choose spacings from a table. We take the table entry that gives
    // the spacing closest to but not less than the ideal spacing.
    int spacing = DEG_MIN_SEC_TOTAL;

    // Scan the tables to find the best spacings
    unsigned int tableSize = sizeof(DEG_MIN_SEC_SPACING) / sizeof(DEG_MIN_SEC_SPACING[0]);
    for (unsigned int i = 0; i < tableSize; i++)
    {
        if (celestia::numbers::pi * (double) DEG_MIN_SEC_SPACING[i] / (double) DEG_MIN_SEC_TOTAL < idealSpacing)
            break;
        spacing = DEG_MIN_SEC_SPACING[i];
    }

    return spacing;
}


// Compute the angular step between meridians
int
SkyGrid::meridianSpacing(double idealSpacing) const
{
    const int* spacingTable = HOUR_MIN_SEC_SPACING;
    unsigned int tableSize = sizeof(HOUR_MIN_SEC_SPACING) / sizeof(HOUR_MIN_SEC_SPACING[0]);
    int totalUnits = HOUR_MIN_SEC_TOTAL;

    // Use degree spacings if the latitude units are degrees instead of hours
    if (m_longitudeUnits == LongitudeDegrees)
    {
        spacingTable = DEG_MIN_SEC_SPACING;
        tableSize = sizeof(DEG_MIN_SEC_SPACING) / sizeof(DEG_MIN_SEC_SPACING[0]);
        totalUnits = DEG_MIN_SEC_TOTAL * 2;
    }

    int spacing = totalUnits;

    for (unsigned int i = 0; i < tableSize; i++)
    {
        if (2 * celestia::numbers::pi * (double) spacingTable[i] / (double) totalUnits < idealSpacing)
            break;
        spacing = spacingTable[i];
    }

    return spacing;
}


void
SkyGrid::render(Renderer& renderer,
                const Observer& observer,
                int windowWidth,
                int windowHeight)
{
    // 90 degree rotation about the x-axis used to transform coordinates
    // to Celestia's system.
    Quaterniond xrot90 = XRotation(-celestia::numbers::pi / 2.0);

    double vfov = observer.getFOV();
    double viewAspectRatio = (double) windowWidth / (double) windowHeight;

    // Calculate the cosine of half the maximum field of view. We'll use this for
    // fast testing of marker visibility. The stored field of view is the
    // vertical field of view; we want the field of view as measured on the
    // diagonal between viewport corners.
    double h = tan(vfov / 2);
    double w = h * viewAspectRatio;
    double diag = sqrt(1.0 + square(h) + square(h * viewAspectRatio));
    double cosHalfFov = 1.0 / diag;
    double halfFov = acos(cosHalfFov);

    auto polarCrossSize = (float) (POLAR_CROSS_SIZE * halfFov);

    // We want to avoid drawing more of the grid than we have to. The following code
    // determines the region of the grid intersected by the view frustum. We're
    // interested in the minimum and maximum phi and theta of the visible patch
    // of the celestial sphere.

    // Find the minimum and maximum theta (longitude) by finding the smallest
    // longitude range containing all corners of the view frustum.

    // View frustum corners
    Vector3d c0(-w, -h, -1.0);
    Vector3d c1( w, -h, -1.0);
    Vector3d c2(-w,  h, -1.0);
    Vector3d c3( w,  h, -1.0);

    Quaterniond cameraOrientation = observer.getOrientation();
    Matrix3d r = (cameraOrientation * xrot90 * m_orientation.conjugate() * xrot90.conjugate()).toRotationMatrix().transpose();

    // Transform the frustum corners by the camera and grid
    // rotations.
    c0 = toStandardCoords(r * c0);
    c1 = toStandardCoords(r * c1);
    c2 = toStandardCoords(r * c2);
    c3 = toStandardCoords(r * c3);

    double thetaC0 = atan2(c0.y(), c0.x());
    double thetaC1 = atan2(c1.y(), c1.x());
    double thetaC2 = atan2(c2.y(), c2.x());
    double thetaC3 = atan2(c3.y(), c3.x());

    // Compute the minimum longitude range containing the corners; slightly
    // tricky because of the wrapping at PI/-PI.
    double minTheta = thetaC0;
    double maxTheta = thetaC1;
    double maxDiff = 0.0;
    updateAngleRange(thetaC0, thetaC1, &maxDiff, &minTheta, &maxTheta);
    updateAngleRange(thetaC0, thetaC2, &maxDiff, &minTheta, &maxTheta);
    updateAngleRange(thetaC0, thetaC3, &maxDiff, &minTheta, &maxTheta);
    updateAngleRange(thetaC1, thetaC2, &maxDiff, &minTheta, &maxTheta);
    updateAngleRange(thetaC1, thetaC3, &maxDiff, &minTheta, &maxTheta);
    updateAngleRange(thetaC2, thetaC3, &maxDiff, &minTheta, &maxTheta);

    if (std::fabs(maxTheta - minTheta) < celestia::numbers::pi)
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
    Vector3d frustumNormal[4];
    frustumNormal[0] = Vector3d( 0.0,  1.0, -h);
    frustumNormal[1] = Vector3d( 0.0, -1.0, -h);
    frustumNormal[2] = Vector3d( 1.0,  0.0, -w);
    frustumNormal[3] = Vector3d(-1.0,  0.0, -w);

    for (int i = 0; i < 4; i++)
    {
        frustumNormal[i] = toStandardCoords(r * frustumNormal[i].normalized());
    }

    Vector3d viewCenter(-Vector3d::UnitZ());
    viewCenter = toStandardCoords(r * viewCenter);

    double centerDec;
    if (fabs(viewCenter.z()) < 1.0)
        centerDec = std::asin(viewCenter.z());
    else if (viewCenter.z() < 0.0)
        centerDec = -celestia::numbers::pi / 2.0;
    else
        centerDec = celestia::numbers::pi / 2.0;

    double minDec = centerDec - halfFov;
    double maxDec = centerDec + halfFov;

    if (maxDec >= celestia::numbers::pi / 2.0)
    {
        // view cone contains north pole
        maxDec = celestia::numbers::pi / 2.0;
        minTheta = -celestia::numbers::pi;
        maxTheta = celestia::numbers::pi;
    }
    else if (minDec <= -celestia::numbers::pi / 2.0)
    {
        // view cone contains south pole
        minDec = -celestia::numbers::pi / 2.0;
        minTheta = -celestia::numbers::pi;
        maxTheta = celestia::numbers::pi;
    }

    double idealParallelSpacing = 2.0 * halfFov / MAX_VISIBLE_ARCS;
    double idealMeridianSpacing = idealParallelSpacing;

    // Adjust the spacing between meridians based on how close the view direction
    // is to the poles; the density of meridians increases as we approach the pole,
    // so we want to increase the angular distance between meridians.
#if 1
    // Choose spacing based on the minimum declination (closest to zero)
    double minAbsDec = std::min(std::fabs(minDec), std::fabs(maxDec));
    if (minDec * maxDec <= 0.0f) // Check if min and max straddle the equator
        minAbsDec = 0.0f;
    idealMeridianSpacing /= cos(minAbsDec);
#else
    // Choose spacing based on the maximum declination (closest to pole)
    double maxAbsDec = std::max(std::fabs(minDec), std::fabs(maxDec));
    idealMeridianSpacing /= max(cos(PI / 2.0 - 5.0 * idealParallelSpacing), cos(maxAbsDec));
#endif

    int totalLongitudeUnits = HOUR_MIN_SEC_TOTAL;
    if (m_longitudeUnits == LongitudeDegrees)
        totalLongitudeUnits = DEG_MIN_SEC_TOTAL * 2;

    int raIncrement  = meridianSpacing(idealMeridianSpacing);
    int decIncrement = parallelSpacing(idealParallelSpacing);

    int startRa  = (int) std::ceil (totalLongitudeUnits * (minTheta / (celestia::numbers::pi * 2.0)) / (double) raIncrement) * raIncrement;
    int endRa    = (int) std::floor(totalLongitudeUnits * (maxTheta / (celestia::numbers::pi * 2.0)) / (double) raIncrement) * raIncrement;
    int startDec = (int) std::ceil (DEG_MIN_SEC_TOTAL  * (minDec / celestia::numbers::pi) / (double) decIncrement) * decIncrement;
    int endDec   = (int) std::floor(DEG_MIN_SEC_TOTAL  * (maxDec / celestia::numbers::pi) / (double) decIncrement) * decIncrement;

    // Get the orientation at single precision
    Quaterniond q = xrot90 * m_orientation * xrot90.conjugate();
    Quaternionf orientationf = q.cast<float>();

    // Radius of sphere is arbitrary, with the constraint that it shouldn't
    // intersect the near or far plane of the view frustum.
    Matrix4f m = renderer.getModelViewMatrix() *
                 vecgl::rotate((xrot90 * m_orientation.conjugate() * xrot90.conjugate()).cast<float>()) *
                 vecgl::scale(1000.0f);
    Matrices matrices = {&renderer.getProjectionMatrix(), &m};

    double arcStep = (maxTheta - minTheta) / (double) ARC_SUBDIVISIONS;
    double theta0 = minTheta;

    int count = 0;
    if (g_gridRenderer == nullptr)
        g_gridRenderer = new LineRenderer(renderer, 1.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Stream);
    g_gridRenderer->startUpdate();

    for (int dec = startDec; dec <= endDec; dec += decIncrement)
    {
        count++;
        double phi = celestia::numbers::pi * (double) dec / (double) DEG_MIN_SEC_TOTAL;
        double cosPhi = cos(phi);
        double sinPhi = sin(phi);

        for (int j = 0; j <= ARC_SUBDIVISIONS; j++)
        {
            double theta = theta0 + j * arcStep;
            auto x = (float) (cosPhi * std::cos(theta));
            auto y = (float) (cosPhi * std::sin(theta));
            auto z = (float) sinPhi;
            g_gridRenderer->addVertex(x, z, -y);  // convert to Celestia coords
        }

        // Place labels at the intersections of the view frustum planes
        // and the parallels.
        Vector3d center(0.0, 0.0, sinPhi);
        Vector3d axis0(cosPhi, 0.0, 0.0);
        Vector3d axis1(0.0, cosPhi, 0.0);
        for (int k = 0; k < 4; k += 2)
        {
            Vector3d isect0(Vector3d::Zero());
            Vector3d isect1(Vector3d::Zero());

            if (planeCircleIntersection(frustumNormal[k], center, axis0, axis1,
                                        &isect0, &isect1))
            {
                string labelText = latitudeLabel(dec, decIncrement);

                Vector3f p0(toCelestiaCoords(isect0).cast<float>());
                Vector3f p1(toCelestiaCoords(isect1).cast<float>());

#ifdef DEBUG_LABEL_PLACEMENT
                glPointSize(5.0);
                glBegin(GL_POINTS);
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3fv(p0.data());
                glVertex3fv(p1.data());
                glColor(m_lineColor);
                glEnd();
#endif

                Matrix3f m = observer.getOrientationf().toRotationMatrix();
                p0 = orientationf.conjugate() * p0;
                p1 = orientationf.conjugate() * p1;
                Renderer::LabelAlignment hAlign = getCoordLabelHAlign(k);
                Renderer::LabelVerticalAlignment vAlign = getCoordLabelVAlign(k);

                if ((m * p0).z() < 0.0)
                {
                    renderer.addBackgroundAnnotation(nullptr, labelText, m_labelColor, p0, hAlign, vAlign);
                }

                if ((m * p1).z() < 0.0)
                {
                    renderer.addBackgroundAnnotation(nullptr, labelText, m_labelColor, p1, hAlign, vAlign);
                }
            }
        }
    }

    // Draw the meridians

    // Render meridians only to the last latitude circle; this looks better
    // than spokes radiating from the pole.
    double maxMeridianAngle = celestia::numbers::pi / 2.0 * (1.0 - 2.0 * (double) decIncrement / (double) DEG_MIN_SEC_TOTAL);
    minDec = std::max(minDec, -maxMeridianAngle);
    maxDec = std::min(maxDec,  maxMeridianAngle);
    arcStep = (maxDec - minDec) / (double) ARC_SUBDIVISIONS;
    double phi0 = minDec;

    double cosMaxMeridianAngle = cos(maxMeridianAngle);

    for (int ra = startRa; ra <= endRa; ra += raIncrement)
    {
        count++;
        double theta = 2.0 * celestia::numbers::pi * (double) ra / (double) totalLongitudeUnits;
        double cosTheta = cos(theta);
        double sinTheta = sin(theta);

        for (int j = 0; j <= ARC_SUBDIVISIONS; j++)
        {
            double phi = phi0 + j * arcStep;
            auto x = (float) (cos(phi) * cosTheta);
            auto y = (float) (cos(phi) * sinTheta);
            auto z = (float) sin(phi);
            g_gridRenderer->addVertex(x, z, -y);  // convert to Celestia coords
        }

        // Place labels at the intersections of the view frustum planes
        // and the meridians.
        Vector3d center(Vector3d::Zero());
        Vector3d axis0(cosTheta, sinTheta, 0.0);
        Vector3d axis1(Vector3d::UnitZ());
        for (int k = 1; k < 4; k += 2)
        {
            Vector3d isect0(Vector3d::Zero());
            Vector3d isect1(Vector3d::Zero());

            if (planeCircleIntersection(frustumNormal[k], center, axis0, axis1,
                                        &isect0, &isect1))
            {
                string labelText = longitudeLabel(ra, raIncrement);

                Vector3f p0(toCelestiaCoords(isect0).cast<float>());
                Vector3f p1(toCelestiaCoords(isect1).cast<float>());

#ifdef DEBUG_LABEL_PLACEMENT
                glPointSize(5.0);
                glBegin(GL_POINTS);
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3fv(p0.data());
                glVertex3fv(p1.data());
                glColor(m_lineColor);
                glEnd();
#endif

                Matrix3f m = observer.getOrientationf().toRotationMatrix();
                p0 = orientationf.conjugate() * p0;
                p1 = orientationf.conjugate() * p1;
                Renderer::LabelAlignment hAlign = getCoordLabelHAlign(k);
                Renderer::LabelVerticalAlignment vAlign = getCoordLabelVAlign(k);

                if ((m * p0).z() < 0.0 && axis0.dot(isect0) >= cosMaxMeridianAngle)
                {
                    renderer.addBackgroundAnnotation(nullptr, labelText, m_labelColor, p0, hAlign, vAlign);
                }

                if ((m * p1).z() < 0.0 && axis0.dot(isect1) >= cosMaxMeridianAngle)
                {
                    renderer.addBackgroundAnnotation(nullptr, labelText, m_labelColor, p1, hAlign, vAlign);
                }
            }
        }
    }

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    renderer.setPipelineState(ps);

    for (int offset = 0, i = 0; i < count; i++)
    {
        g_gridRenderer->render(matrices, m_lineColor, ARC_SUBDIVISIONS + 1, offset);
        offset += ARC_SUBDIVISIONS + 1;
    }

    // Draw crosses indicating the north and south poles
    if (g_crossRenderer == nullptr)
        g_crossRenderer = new LineRenderer(renderer, 1.0f, LineRenderer::PrimType::Lines, LineRenderer::StorageType::Stream);
    g_crossRenderer->startUpdate();
    g_crossRenderer->addVertex(-polarCrossSize,  1.0f,  0.0f);
    g_crossRenderer->addVertex( polarCrossSize,  1.0f,  0.0f);
    g_crossRenderer->addVertex( 0.0f,            1.0f, -polarCrossSize);
    g_crossRenderer->addVertex( 0.0f,            1.0f,  polarCrossSize);
    g_crossRenderer->addVertex(-polarCrossSize, -1.0f,  0.0f);
    g_crossRenderer->addVertex( polarCrossSize, -1.0f,  0.0f);
    g_crossRenderer->addVertex( 0.0f,           -1.0f, -polarCrossSize);
    g_crossRenderer->addVertex( 0.0f,           -1.0f,  polarCrossSize);
    g_crossRenderer->render(matrices, m_lineColor, 8);

    g_gridRenderer->clear();
    g_crossRenderer->clear();
    g_gridRenderer->finish();
    g_crossRenderer->finish();
}

void
SkyGrid::deinit()
{
    delete g_gridRenderer;
    g_gridRenderer = nullptr;
    delete g_crossRenderer;
    g_crossRenderer = nullptr;
}
