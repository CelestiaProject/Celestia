// qtinfopanel.cpp
//
// Copyright (C) 2008, Celestia Development Team
// celestia-developers@lists.sourceforge.net
//
// Information panel for Qt4 UI.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celengine/astro.h"
#include "celutil/utf8.h"
#include "celengine/universe.h"
#include <QTextBrowser>
#include "qtinfopanel.h"


// TODO: This should be moved to astro.cpp
struct OrbitalElements
{
    double semimajorAxis;
    double eccentricity;
    double inclination;
    double longAscendingNode;
    double argPericenter;
    double meanAnomaly;
    double period;
};

static void CalculateOsculatingElements(const Orbit& orbit,
                                        double t,
                                        double dt,
                                        OrbitalElements* elements);


InfoPanel::InfoPanel(const QString& title, QWidget* parent) :
    QDockWidget(title, parent),
    textBrowser(NULL)
{
    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    setWidget(textBrowser);
}


InfoPanel::~InfoPanel()
{
}


void InfoPanel::buildInfoPage(Selection sel,
                              Universe* universe,
                              double tdb)
{
    QString pageText;
    QTextStream stream(&pageText, QIODevice::WriteOnly);

    pageHeader(stream);

    if (sel.body() != NULL)
    {
        buildSolarSystemBodyPage(sel.body(), tdb, stream);
    }
    else if (sel.star() != NULL)
    {
        buildStarPage(sel.star(), universe, tdb, stream);
    }
    else if (sel.deepsky() != NULL)
    {
        buildDSOPage(sel.deepsky(), universe, stream);
    }
    else
    {
        stream << "Error: no object selected!\n";
    }

    pageFooter(stream);

    textBrowser->setHtml(pageText);
}


void InfoPanel::pageHeader(QTextStream& stream)
{
    stream << "<html><head><title>Info</title></head>\n";
    stream << "<body>\n";
}


void InfoPanel::pageFooter(QTextStream& stream)
{
    stream << "</body></html>\n";
}


static QString boldText(const QString& s)
{
    return QString("<b>") + s + QString("</b>");
}


static QString anchor(const QString& href, const QString& text)
{
    return QString("<a href=\"") + href + QString("\">") + text + QString("</a>");
}


void InfoPanel::buildSolarSystemBodyPage(const Body* body,
                                         double t,
                                         QTextStream& stream)
{
    stream << "<h1>" << body->getName().c_str() << "</h1><br>\n";

    if (!body->getInfoURL().empty())
    {
        const char* infoURL = body->getInfoURL().c_str();
        stream << tr("Web info: ") << anchor(infoURL, infoURL) << "<br>\n";
    }

    stream << "<br>\n";

    bool isArtificial = body->getClassification() == Body::Spacecraft;

    QString units(tr("km"));
    double radius = body->getRadius();
    if (radius < 1.0)
    {
        units = tr("m");
        radius *= 1000.0;
    }

    if (body->isEllipsoid())
        stream << boldText(tr("Equatorial radius: "));
    else
        stream << boldText(tr("Size: "));

    stream << radius << " " << units << "<br>";

#if 0
    if (body->getOblateness() != 0.0f && body->isEllipsoid())
    {
        stream << boldText(tr("Oblateness: ")) << body->getOblateness() << "<br>\n";
    }
#endif

    double orbitalPeriod = 0.0;
    if (body->getOrbit(t)->isPeriodic())
        orbitalPeriod = body->getOrbit(t)->getPeriod();

    // Show rotation information for natural, periodic rotators
    if (body->getRotationModel(t)->isPeriodic() && !isArtificial)
    {
        double rotPeriod = body->getRotationModel(t)->getPeriod();

        double dayLength = 0.0;
        if (body->getOrbit(t)->isPeriodic())
        {
            double siderealDaysPerYear = orbitalPeriod / rotPeriod;
            double solarDaysPerYear = siderealDaysPerYear - 1.0;
            if (solarDaysPerYear > 0.0001)
            {
                dayLength = orbitalPeriod / (siderealDaysPerYear - 1.0);
            }
        }

        if (rotPeriod < 2.0)
        {
            rotPeriod *= 24.0;
            dayLength *= 24.0;
            units = tr("hours");
        }
        else
        {
            units = tr("days");
        }

        stream << boldText(tr("Sidereal rotation period: ")) << rotPeriod << " " << units << "<br>\n";
        if (dayLength != 0.0)
        {
            stream << boldText(tr("Length of day: ")) << dayLength << " " << units << "<br>\n";
        }
    }

    // Orbit information
    if (orbitalPeriod != 0.0 || 1)
    {
        if (orbitalPeriod == 0.0)
            orbitalPeriod = 365.25;

        OrbitalElements elements;
        CalculateOsculatingElements(*body->getOrbit(t),
                                    t,
                                    orbitalPeriod * 1.0e-6,
                                    &elements);

        if (orbitalPeriod < 365.25 * 2.0)
        {
            units = tr("days");
        }
        else
        {
            units = tr("years");
            orbitalPeriod /= 365.25;
        }

        stream << "<br><big><b>" << tr("Orbit information") << "</b></big><br>\n";
        stream << tr("Osculating elements for") << " " << astro::TDBtoUTC(t).toCStr() << "<br>\n";
        stream << "<i>[ Orbit reference plane info goes here ]</i><br>\n";
        stream << boldText(tr("Period: ")) << orbitalPeriod << " " << units << "<br>\n";

        double sma = elements.semimajorAxis;
        if (sma > 2.5e7)
        {
            units = tr("AU");
            sma = astro::kilometersToAU(sma);
        }
        else
        {
            units = tr("km");
        }

        stream << boldText(tr("Semi-major axis: ")) << sma << " " << units << "<br>\n";
        stream << boldText(tr("Eccentricity: ")) << elements.eccentricity << "<br>\n";
        stream << boldText(tr("Inclination: ")) << radToDeg(elements.inclination) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(tr("Pericenter distance: ")) << sma * (1 - elements.eccentricity) << " " << units << "<br>\n";
        stream << boldText(tr("Apocenter distance: ")) << sma * (1 + elements.eccentricity) << " " << units << "<br>\n";

#if 1
        stream << boldText(tr("Ascending node: ")) << radToDeg(elements.longAscendingNode) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(tr("Argument of periapsis: ")) << radToDeg(elements.argPericenter) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(tr("Mean anomaly: ")) << radToDeg(elements.meanAnomaly) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(tr("Period (calculated): ")) << elements.period << "d<br>\n";
#endif
    }
}


Vec3d celToJ2000Ecliptic(const Point3d& p)
{
    return Vec3d(p.x, -p.z, p.y);
}


Vec3d rectToSpherical(const Vec3d& v)
{
    double r = v.length();
    double theta = atan2(v.y, v.x);
    if (theta < 0)
        theta = theta + 2 * PI;
    double phi = asin(v.z / r);

    return Vec3d(theta, phi, r);
}


void InfoPanel::buildStarPage(const Star* star, const Universe* universe, double tdb, QTextStream& stream)
{
    string name = universe->getStarCatalog()->getStarNameList(*star);
    stream << "<b>" << QString::fromUtf8(name.c_str(), name.length()) << "</b><br>\n";
    
    Vec3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(star->getPosition(tdb)));
    Vec3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x), hours, minutes, seconds);
    stream << "RA: " << hours << "h " << abs(minutes) << "m " << abs(seconds) << "s<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y), degrees, minutes, seconds);
    stream << "Dec: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";

}


void InfoPanel::buildDSOPage(const DeepSkyObject* dso,
                             const Universe* universe,
                             QTextStream& stream)
{
    string name = universe->getDSOCatalog()->getDSOName(dso);
    stream << "<h1>" << name.c_str() << "</h1><br>\n";
    
    Vec3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(ptFromEigen(dso->getPosition())));
    Vec3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x), hours, minutes, seconds);
    stream << "RA: " << hours << "h " << abs(minutes) << "m " << abs(seconds) << "s<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y), degrees, minutes, seconds);
    stream << "Dec: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";

    Vec3d galPos = astro::equatorialToGalactic(eqPos);
    sph = rectToSpherical(galPos);
    
    astro::decimalToDegMinSec(radToDeg(sph.x), degrees, minutes, seconds);
    stream << "L: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";
    astro::decimalToDegMinSec(radToDeg(sph.y), degrees, minutes, seconds);
    stream << "B: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";
}




static void StateVectorToElements(const Point3d& position,
                                  const Vec3d& v,
                                  double GM,
                                  OrbitalElements* elements)
{
    Vec3d R = position - Point3d(0.0, 0.0, 0.0);
    Vec3d L = R ^ v;
    double magR = R.length();
    double magL = L.length();
    double magV = v.length();
    L *= (1.0 / magL);

    Vec3d W = L ^ (R / magR);

    // Compute the semimajor axis
    double a = 1.0 / (2.0 / magR - square(magV) / GM);

    // Compute the eccentricity
    double p = square(magL) / GM;
    double q = R * v;
    double ex = 1.0 - magR / a;
    double ey = q / sqrt(a * GM);
    double e = sqrt(ex * ex + ey * ey);

    // Compute the mean anomaly
    double E = atan2(ey, ex);
    double M = E - e * sin(E);

    // Compute the inclination
    double cosi = L * Vec3d(0, 1.0, 0);
    double i = 0.0;
    if (cosi < 1.0)
        i = acos(cosi);

    // Compute the longitude of ascending node
    double Om = atan2(L.x, L.z);

    // Compute the argument of pericenter
    Vec3d U = R / magR;
    double s_nu = (v * U) * sqrt(p / GM);
    double c_nu = (v * W) * sqrt(p / GM) - 1;
    s_nu /= e;
    c_nu /= e;
    Vec3d P = U * c_nu - W * s_nu;
    Vec3d Q = U * s_nu + W * c_nu;
    double om = atan2(P.y, Q.y);

    // Compute the period
    double T = 2 * PI * sqrt(cube(a) / GM);

    elements->semimajorAxis     = a;
    elements->eccentricity      = e;
    elements->inclination       = i;
    elements->longAscendingNode = Om;
    elements->argPericenter     = om;
    elements->meanAnomaly       = M;
    elements->period            = T;
}


static void CalculateOsculatingElements(const Orbit& orbit,
                                        double t,
                                        double dt,
                                        OrbitalElements* elements)
{
    double sdt = dt;

    // If the trajectory is finite, make sure we sample
    // it inside the valid time interval.
    if (!orbit.isPeriodic())
    {
        double beginTime = 0.0;
        double endTime = 0.0;
        orbit.getValidRange(beginTime, endTime);
        clog << "t+dt: " << t + dt << ", endTime: " << endTime << endl;
        if (t + dt > endTime)
        {
            clog << "REVERSE\n";
            sdt = -dt;
        }
    }

    Point3d p0 = orbit.positionAtTime(t);
    Point3d p1 = orbit.positionAtTime(t + sdt);
    Vec3d v0 = orbit.velocityAtTime(t);
    Vec3d v1 = orbit.velocityAtTime(t + sdt);

    double accel = ((v1 - v0) / sdt).length();
    Vec3d r = p0 - Point3d(0.0, 0.0, 0.0);
    double GM = accel * (r * r);

    clog << "vel: " << v0.length() / 86400.0 << endl;
    clog << "vel (est): " << (p1 - p0).length() / sdt / 86400 << endl;
    clog << "osc: " << t << ", " << dt << ", " << accel << ", GM=" << GM << endl;

    StateVectorToElements(p0, v0, GM, elements);
}


