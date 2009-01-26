// skygrid.cpp
//
// Celestial longitude/latitude grids.
//
// Copyright (C) 2008-2009, the Celestia Development Team
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
#include "celutil/util.h"
#include "render.h"
#include "vecgl.h"
#include "skygrid.h"

using namespace std;


// #define DEBUG_LABEL_PLACEMENT

// The maximum number of parallels or meridians that will be visible
const double MAX_VISIBLE_ARCS = 10.0;

// Number of line segments used to approximate one arc of the celestial sphere
const int ARC_SUBDIVISIONS = 100;

// Size of the cross indicating the north and south poles
const double POLAR_CROSS_SIZE = 0.01;

// Grid line spacing tables
static const int MSEC = 1;
static const int SEC = 1000;
static const int MIN = 60 * SEC;
static const int DEG = 60 * MIN;
static const int HR  = 60 * MIN;

static const int HOUR_MIN_SEC_TOTAL = 24 * HR;
static const int DEG_MIN_SEC_TOTAL  = 180 * DEG;

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


SkyGrid::SkyGrid() :
    m_orientation(1.0),
    m_lineColor(Color::White),
    m_labelColor(Color::White),
    m_longitudeUnits(LongitudeHours),
    m_longitudeDirection(IncreasingCounterclockwise)
{
}


SkyGrid::~SkyGrid()
{
}


static Vec3d 
toStandardCoords(const Vec3d& v)
{
    return Vec3d(v.x, -v.z, v.y);
}

// Compute the difference between two angles in [-PI, PI]
template<class T> static T
angleDiff(T a, T b)
{
    T diff = std::fabs(a - b);
    if (diff > PI)
        return (T) (2.0 * PI - diff);
    else
        return diff;
}

template<class T> static T
min4(T a, T b, T c, T d)
{
    return std::min(a, std::min(b, std::min(c, d)));
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
    if (planeIndex == 1)
        return Renderer::VerticalAlignTop;
    else
        return Renderer::VerticalAlignBottom;
}


// Find the intersection of a circle and the plane with the specified normal and
// containing the origin. The circle is defined parametrically by:
// center + cos(t)*u + sin(t)*u
// u and v are orthogonal vectors with magnitudes equal to the radius of the
// circle.
// Return true if there are two solutions.
template<class T> static bool planeCircleIntersection(const Vector3<T>& planeNormal,
                                                      const Vector3<T>& center,
                                                      const Vector3<T>& u,
                                                      const Vector3<T>& v,
                                                      Vector3<T>* sol0,
                                                      Vector3<T>* sol1)
{
    // Any point p on the plane must satisfy p*N = 0. Thus the intersection points
    // satisfy (center + cos(t)U + sin(t)V)*N = 0
    // This simplifies to an equation of the form:
    // a*cos(t)+b*sin(t)+c = 0, with a=N*U, b=N*V, and c=N*center
    T a = u * planeNormal;
    T b = v * planeNormal;
    T c = center * planeNormal;
    
    // The solution is +-acos((-ac +- sqrt(a^2+b^2-c^2))/(a^2+b^2))
    T s = a * a + b * b;
    if (s == 0.0)
    {
        // No solution; plane containing circle is parallel to test plane
        return false;
    }
    
    if (s - c * c <= 0)
    {
        // One or no solutions; no need to distinguish between these
        // cases for our purposes.
        return false;
    }
    
    // No need to actually call acos to get the solution, since we're just
    // going to plug it into sin and cos anyhow.
    T r = b * std::sqrt(s - c * c);
    T cosTheta0 = (-a * c + r) / s;
    T cosTheta1 = (-a * c - r) / s;
    T sinTheta0 = std::sqrt(1 - cosTheta0 * cosTheta0);
    T sinTheta1 = std::sqrt(1 - cosTheta1 * cosTheta1);
    
    *sol0 = center + cosTheta0 * u + sinTheta0 * v;
    *sol1 = center + cosTheta1 * u + sinTheta1 * v;
    
    // Check that we've chosen a solution that produces a point on the
    // plane. If not, we need to use the -acos solution.
    if (fabs(*sol0 * planeNormal) > 1.0e-8)
    {
        *sol0 = center + cosTheta0 * u - sinTheta0 * v;
    }
    
    if (fabs(*sol1 * planeNormal) > 1.0e-8)
    {
        *sol1 = center + cosTheta1 * u - sinTheta1 * v;
    }
    
    return true;
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
        if (PI * (double) DEG_MIN_SEC_SPACING[i] / (double) DEG_MIN_SEC_TOTAL < idealSpacing)
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
        if (2 * PI * (double) spacingTable[i] / (double) totalUnits < idealSpacing)
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
    Quatd xrot90 = Quatd::xrotation(-PI / 2.0);

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
    
    float polarCrossSize = (float) (POLAR_CROSS_SIZE * halfFov);

    // We want to avoid drawing more of the grid than we have to. The following code
    // determines the region of the grid intersected by the view frustum. We're
    // interested in the minimum and maximum phi and theta of the visible patch
    // of the celestial sphere.

    // Find the minimum and maximum theta (longitude) by finding the smallest
    // longitude range containing all corners of the view frustum.

    // View frustum corners
    Vec3d c0(-w, -h, -1.0);
    Vec3d c1( w, -h, -1.0);
    Vec3d c2(-w,  h, -1.0);
    Vec3d c3( w,  h, -1.0);

    Quatd cameraOrientation = observer.getOrientation();
    Mat3d r = (cameraOrientation * xrot90 * ~m_orientation * ~xrot90).toMatrix3();

    // Transform the frustum corners by the camera and grid
    // rotations.
    c0 = toStandardCoords(c0 * r);
    c1 = toStandardCoords(c1 * r);
    c2 = toStandardCoords(c2 * r);
    c3 = toStandardCoords(c3 * r);

    double thetaC0 = atan2(c0.y, c0.x);
    double thetaC1 = atan2(c1.y, c1.x);
    double thetaC2 = atan2(c2.y, c2.x);
    double thetaC3 = atan2(c3.y, c3.x);

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

    if (std::fabs(maxTheta - minTheta) < PI)
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
    Vec3d frustumNormal[4];    
    frustumNormal[0] = Vec3d( 0,  1, -h);
    frustumNormal[1] = Vec3d( 0, -1, -h);
    frustumNormal[2] = Vec3d( 1,  0, -w);
    frustumNormal[3] = Vec3d(-1,  0, -w);
    
    {
        for (int i = 0; i < 4; i++)
        {
            frustumNormal[i].normalize();
            frustumNormal[i] = toStandardCoords(frustumNormal[i] * r);
        }
    }

    Vec3d viewCenter(0.0, 0.0, -1.0);
    viewCenter = toStandardCoords(viewCenter * r);

    double centerDec;
    if (fabs(viewCenter.z) < 1.0)
        centerDec = std::asin(viewCenter.z);
    else if (viewCenter.z < 0.0)
        centerDec = -PI / 2.0;
    else
        centerDec = PI / 2.0;
    
    double minDec = centerDec - halfFov;
    double maxDec = centerDec + halfFov;

    if (maxDec >= PI / 2.0)
    {
        // view cone contains north pole
        maxDec = PI / 2.0;
        minTheta = -PI;
        maxTheta = PI;
    }
    else if (minDec <= -PI / 2.0)
    {
        // view cone contains south pole
        minDec = -PI / 2.0;
        minTheta = -PI;
        maxTheta = PI;
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

    int startRa  = (int) std::ceil (totalLongitudeUnits * (minTheta / (PI * 2.0f)) / (float) raIncrement) * raIncrement;
    int endRa    = (int) std::floor(totalLongitudeUnits * (maxTheta / (PI * 2.0f)) / (float) raIncrement) * raIncrement;
    int startDec = (int) std::ceil (DEG_MIN_SEC_TOTAL  * (minDec / PI) / (float) decIncrement) * decIncrement;
    int endDec   = (int) std::floor(DEG_MIN_SEC_TOTAL  * (maxDec / PI) / (float) decIncrement) * decIncrement;

    // Get the orientation at single precision
    Quatd q = xrot90 * m_orientation * ~xrot90;
    Quatf orientationf((float) q.w, (float) q.x, (float) q.y, (float) q.z);

    glColor(m_lineColor);

    // Render the parallels
    glPushMatrix();
    glRotate(xrot90 * ~m_orientation * ~xrot90);

    // Radius of sphere is arbitrary, with the constraint that it shouldn't
    // intersect the near or far plane of the view frustum.
    glScalef(1000.0f, 1000.0f, 1000.0f);

    double arcStep = (maxTheta - minTheta) / (double) ARC_SUBDIVISIONS;
    double theta0 = minTheta;
    for (int dec = startDec; dec <= endDec; dec += decIncrement)
    {
        double phi = PI * (double) dec / (double) DEG_MIN_SEC_TOTAL;
        double cosPhi = cos(phi);
        double sinPhi = sin(phi);

        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= ARC_SUBDIVISIONS; j++)
        {
            double theta = theta0 + j * arcStep;
            float x = (float) (cosPhi * std::cos(theta));
            float y = (float) (cosPhi * std::sin(theta));
            float z = (float) sinPhi;
            glVertex3f(x, z, -y);  // convert to Celestia coords
        }
        glEnd();
        
        // Place labels at the intersections of the view frustum planes
        // and the parallels.
        Vec3d center(0.0, 0.0, sinPhi);
        Vec3d axis0(cosPhi, 0.0, 0.0);
        Vec3d axis1(0.0, cosPhi, 0.0);
        for (int k = 0; k < 4; k += 2)
        {
            Vec3d isect0(0.0, 0.0, 0.0);
            Vec3d isect1(0.0, 0.0, 0.0);
            Renderer::LabelAlignment hAlign = getCoordLabelHAlign(k);
            Renderer::LabelVerticalAlignment vAlign = getCoordLabelVAlign(k);

            if (planeCircleIntersection(frustumNormal[k], center, axis0, axis1,
                                        &isect0, &isect1))
            {
                string labelText = latitudeLabel(dec, decIncrement);

                Point3f p0((float) isect0.x, (float) isect0.z, (float) -isect0.y);
                Point3f p1((float) isect1.x, (float) isect1.z, (float) -isect1.y);

#ifdef DEBUG_LABEL_PLACEMENT
                glPointSize(5.0);
                glBegin(GL_POINTS);
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3f(p0.x, p0.y, p0.z);
                glVertex3f(p1.x, p1.y, p1.z);
                glColor(m_lineColor);
                glEnd();
#endif
                
                Mat3f m = conjugate(observer.getOrientationf()).toMatrix3();
                p0 = p0 * orientationf.toMatrix3();
                p1 = p1 * orientationf.toMatrix3();
                
                if ((p0 * m).z < 0.0)
                {
                    renderer.addBackgroundAnnotation(NULL, labelText, m_labelColor, p0, hAlign, vAlign);
                }
                
                if ((p1 * m).z < 0.0)
                {
                    renderer.addBackgroundAnnotation(NULL, labelText, m_labelColor, p1, hAlign, vAlign);
                }                
            }
        }
    }

    // Draw the meridians

    // Render meridians only to the last latitude circle; this looks better
    // than spokes radiating from the pole.
    double maxMeridianAngle = PI / 2.0 * (1.0 - 2.0 * (double) decIncrement / (double) DEG_MIN_SEC_TOTAL);
    minDec = std::max(minDec, -maxMeridianAngle);
    maxDec = std::min(maxDec,  maxMeridianAngle);
    arcStep = (maxDec - minDec) / (double) ARC_SUBDIVISIONS;
    double phi0 = minDec;

    double cosMaxMeridianAngle = cos(maxMeridianAngle);

    for (int ra = startRa; ra <= endRa; ra += raIncrement)
    {

        double theta = 2.0 * PI * (double) ra / (double) totalLongitudeUnits;
        double cosTheta = cos(theta);
        double sinTheta = sin(theta);

        glBegin(GL_LINE_STRIP);
        for (int j = 0; j <= ARC_SUBDIVISIONS; j++)
        {
            double phi = phi0 + j * arcStep;
            float x = (float) (cos(phi) * cosTheta);
            float y = (float) (cos(phi) * sinTheta);
            float z = (float) sin(phi);
            glVertex3f(x, z, -y);  // convert to Celestia coords
        }
        glEnd();
        
        // Place labels at the intersections of the view frustum planes
        // and the meridians.
        Vec3d center(0.0, 0.0, 0.0);
        Vec3d axis0(cosTheta, sinTheta, 0.0);
        Vec3d axis1(0.0, 0.0, 1.0);
        for (int k = 1; k < 4; k += 2)
        {
            Vec3d isect0(0.0, 0.0, 0.0);
            Vec3d isect1(0.0, 0.0, 0.0);
            Renderer::LabelAlignment hAlign = getCoordLabelHAlign(k);
            Renderer::LabelVerticalAlignment vAlign = getCoordLabelVAlign(k);

            if (planeCircleIntersection(frustumNormal[k], center, axis0, axis1,
                                        &isect0, &isect1))
            {
                string labelText = longitudeLabel(ra, raIncrement);

                Point3f p0((float) isect0.x, (float) isect0.z, (float) -isect0.y);
                Point3f p1((float) isect1.x, (float) isect1.z, (float) -isect1.y);

#ifdef DEBUG_LABEL_PLACEMENT
                glPointSize(5.0);
                glBegin(GL_POINTS);
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
                glVertex3f(p0.x, p0.y, p0.z);
                glVertex3f(p1.x, p1.y, p1.z);
                glColor(m_lineColor);
                glEnd();
#endif

                Mat3f m = conjugate(observer.getOrientationf()).toMatrix3();
                p0 = p0 * orientationf.toMatrix3();
                p1 = p1 * orientationf.toMatrix3();
                
                if ((p0 * m).z < 0.0 && axis0 * isect0 >= cosMaxMeridianAngle)
                {
                    renderer.addBackgroundAnnotation(NULL, labelText, m_labelColor, p0, hAlign, vAlign);
                }
                
                if ((p1 * m).z < 0.0 && axis0 * isect1 >= cosMaxMeridianAngle)
                {
                    renderer.addBackgroundAnnotation(NULL, labelText, m_labelColor, p1, hAlign, vAlign);
                }               
            }
        }        
    }
    
    // Draw crosses indicating the north and south poles
    glBegin(GL_LINES);
    glVertex3f(-polarCrossSize, 1.0f,  0.0f);
    glVertex3f( polarCrossSize, 1.0f,  0.0f);
    glVertex3f(0.0f, 1.0f, -polarCrossSize);
    glVertex3f(0.0f, 1.0f,  polarCrossSize);
    glVertex3f(-polarCrossSize, -1.0f,  0.0f);
    glVertex3f( polarCrossSize, -1.0f,  0.0f);
    glVertex3f(0.0f, -1.0f, -polarCrossSize);
    glVertex3f(0.0f, -1.0f,  polarCrossSize);
    glEnd();
    
    glPopMatrix();
}
