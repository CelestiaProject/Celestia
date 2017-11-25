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

using namespace Eigen;

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
    stream << "<h1>" <<QString::fromUtf8(body->getName(true).c_str()) << "</h1><br>\n";

    if (!body->getInfoURL().empty())
    {
        const char* infoURL = body->getInfoURL().c_str();
        stream << _("Web info: ") << anchor(infoURL, infoURL) << "<br>\n";
    }

    stream << "<br>\n";

    bool isArtificial = body->getClassification() == Body::Spacecraft;

    QString units(_("km"));
    double radius = body->getRadius();
    if (radius < 1.0)
    {
        units = _("m");
        radius *= 1000.0;
    }

    if (body->isEllipsoid())
        stream << boldText(_("Equatorial radius: "));
    else
        stream << boldText(_("Size: "));

    stream << radius << " " << units << "<br>";

#if 0
    if (body->getOblateness() != 0.0f && body->isEllipsoid())
    {
        stream << boldText(_("Oblateness: ")) << body->getOblateness() << "<br>\n";
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
            units = _("hours");
        }
        else
        {
            units = _("days");
        }

        stream << boldText(_("Sidereal rotation period: ")) << rotPeriod << " " << units << "<br>\n";
        if (dayLength != 0.0)
        {
            stream << boldText(_("Length of day: ")) << dayLength << " " << units << "<br>\n";
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
            units = _("days");
        }
        else
        {
            units = _("years");
            orbitalPeriod /= 365.25;
        }

        stream << "<br><big><b>" << QString(_("Orbit information")) << "</b></big><br>\n";
        stream << QString(_("Osculating elements for")) << " " << QString::fromUtf8(astro::TDBtoUTC(t).toCStr()) << "<br>\n";
        stream << "<i>[ Orbit reference plane info goes here ]</i><br>\n";
        stream << boldText(_("Period: ")) << orbitalPeriod << " " << units << "<br>\n";

        double sma = elements.semimajorAxis;
        if (sma > 2.5e7)
        {
            units = _("AU");
            sma = astro::kilometersToAU(sma);
        }
        else
        {
            units = _("km");
        }

        stream << boldText(_("Semi-major axis: ")) << sma << " " << units << "<br>\n";
        stream << boldText(_("Eccentricity: ")) << elements.eccentricity << "<br>\n";
        stream << boldText(_("Inclination: ")) << radToDeg(elements.inclination) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(_("Pericenter distance: ")) << sma * (1 - elements.eccentricity) << " " << units << "<br>\n";
        stream << boldText(_("Apocenter distance: ")) << sma * (1 + elements.eccentricity) << " " << units << "<br>\n";

#if 1
        stream << boldText(_("Ascending node: ")) << radToDeg(elements.longAscendingNode) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(_("Argument of periapsis: ")) << radToDeg(elements.argPericenter) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(_("Mean anomaly: ")) << radToDeg(elements.meanAnomaly) << QString::fromUtf8(UTF8_DEGREE_SIGN) << "<br>\n";
        stream << boldText(_("Period (calculated): ")) << elements.period << " " << QString(_("days<br>\n"));
#endif
    }
}


Vector3d celToJ2000Ecliptic(const Vector3d& p)
{
    return Vector3d(p.x(), -p.z(), p.y());
}


Vector3d rectToSpherical(const Vector3d& v)
{
    double r = v.norm();
    double theta = atan2(v.y(), v.x());
    if (theta < 0)
        theta = theta + 2 * PI;
    double phi = asin(v.z() / r);

    return Vector3d(theta, phi, r);
}


void InfoPanel::buildStarPage(const Star* star, const Universe* universe, double tdb, QTextStream& stream)
{
    string name = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*star, true));
    stream << "<h1>" << QString::fromUtf8(name.c_str()) << "</h1><br>\n";
    
    // Compute the star's position relative to the Solar System Barycenter. Note that
    // this will ignore the effect of parallax in the star's position.
    // TODO: Use either the observer's position or the Earth's position as the
    // origin instead.
    Vector3d celPos = star->getPosition(tdb).offsetFromKm(UniversalCoord::Zero());
    Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(celPos));
    Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x()), hours, minutes, seconds);
    stream << "RA: " << hours << "h " << abs(minutes) << "m " << abs(seconds) << "s<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << "Dec: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";

}


void InfoPanel::buildDSOPage(const DeepSkyObject* dso,
                             const Universe* universe,
                             QTextStream& stream)
{
    string name = universe->getDSOCatalog()->getDSOName(dso, true);
    stream << "<h1>" << QString::fromUtf8(name.c_str()) << "</h1><br>\n";
    
    Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(dso->getPosition()));
    Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x()), hours, minutes, seconds);
    stream << "RA: " << hours << "h " << abs(minutes) << "m " << abs(seconds) << "s<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << "Dec: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";

    Vector3d galPos = astro::equatorialToGalactic(eqPos);
    sph = rectToSpherical(galPos);
    
    astro::decimalToDegMinSec(radToDeg(sph.x()), degrees, minutes, seconds);
    stream << "L: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << "B: " << degrees << QString::fromUtf8(UTF8_DEGREE_SIGN) << " " <<
        abs(minutes) << "' " << abs(seconds) << "\"<br>\n";
}




static void StateVectorToElements(const Vector3d& position,
                                  const Vector3d& v,
                                  double GM,
                                  OrbitalElements* elements)
{
    Vector3d R = position;
    Vector3d L = R.cross(v);
    double magR = R.norm();
    double magL = L.norm();
    double magV = v.norm();
    L *= (1.0 / magL);

    Vector3d W = L.cross(R / magR);

    // Compute the semimajor axis
    double a = 1.0 / (2.0 / magR - square(magV) / GM);

    // Compute the eccentricity
    double p = square(magL) / GM;
    double q = R.dot(v);
    double ex = 1.0 - magR / a;
    double ey = q / sqrt(a * GM);
    double e = sqrt(ex * ex + ey * ey);

    // Compute the mean anomaly
    double E = atan2(ey, ex);
    double M = E - e * sin(E);

    // Compute the inclination
    double cosi = L.dot(Vector3d::UnitY());
    double i = 0.0;
    if (cosi < 1.0)
        i = acos(cosi);

    // Compute the longitude of ascending node
    double Om = atan2(L.x(), L.z());

    // Compute the argument of pericenter
    Vector3d U = R / magR;
    double s_nu = v.dot(U) * sqrt(p / GM);
    double c_nu = v.dot(W) * sqrt(p / GM) - 1;
    s_nu /= e;
    c_nu /= e;
    Vector3d P = U * c_nu - W * s_nu;
    Vector3d Q = U * s_nu + W * c_nu;
    double om = atan2(P.y(), Q.y());

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

    Vector3d p0 = orbit.positionAtTime(t);
    Vector3d p1 = orbit.positionAtTime(t + sdt);
    Vector3d v0 = orbit.velocityAtTime(t);
    Vector3d v1 = orbit.velocityAtTime(t + sdt);

    double accel = ((v1 - v0) / sdt).norm();
    Vector3d r = p0;
    double GM = accel * r.squaredNorm();

    clog << "vel: " << v0.norm() / 86400.0 << endl;
    clog << "vel (est): " << (p1 - p0).norm() / sdt / 86400 << endl;
    clog << "osc: " << t << ", " << dt << ", " << accel << ", GM=" << GM << endl;

    StateVectorToElements(p0, v0, GM, elements);
}


