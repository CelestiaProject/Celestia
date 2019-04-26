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


#include <celestia/celestiacore.h>
#include <celengine/astro.h>
#include <celutil/utf8.h>
#include <celengine/universe.h>
#include <QTextBrowser>
#include <QItemSelection>
#include "qtinfopanel.h"

using namespace Eigen;
using namespace celmath;

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


InfoPanel::InfoPanel(CelestiaCore* _appCore, const QString& title, QWidget* parent) :
    QDockWidget(title, parent),
    appCore(_appCore)
{
    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    setWidget(textBrowser);
}


void InfoPanel::buildInfoPage(Selection sel,
                              Universe* universe,
                              double tdb)
{
    QString pageText;
    QTextStream stream(&pageText, QIODevice::WriteOnly);

    pageHeader(stream);

    if (sel.body() != nullptr)
    {
        buildSolarSystemBodyPage(sel.body(), tdb, stream);
    }
    else if (sel.star() != nullptr)
    {
        buildStarPage(sel.star(), universe, tdb, stream);
    }
    else if (sel.deepsky() != nullptr)
    {
        buildDSOPage(sel.deepsky(), universe, stream);
    }
    else
    {
        stream << QString(_("Error: no object selected!\n"));
    }

    pageFooter(stream);

    textBrowser->setHtml(pageText);
}


void InfoPanel::pageHeader(QTextStream& stream)
{
    stream << "<html><head><title>" << QString(_("Info")) << "</title></head><body>";
}


void InfoPanel::pageFooter(QTextStream& stream)
{
    stream << "</body></html>";
}


static QString anchor(const QString& href, const QString& text)
{
    return QString("<a href=\"%1\">%2</a>").arg(href, text);
}


void InfoPanel::buildSolarSystemBodyPage(const Body* body,
                                         double t,
                                         QTextStream& stream)
{
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(body->getName(true)));

    if (!body->getInfoURL().empty())
    {
        QString infoURL = QString::fromStdString(body->getInfoURL());
        stream << QString(_("Web info: %1")).arg(anchor(infoURL, infoURL)) << "<br>\n";
    }

    stream << "<br>";

    bool isArtificial = body->getClassification() == Body::Spacecraft;

    QString units(_("km"));
    double radius = body->getRadius();
    if (radius < 1.0)
    {
        units = _("m");
        radius *= 1000.0;
    }

    if (body->isEllipsoid())
        stream << QString(_("<b>Equatorial radius:</b> %L1 %2")).arg(radius).arg(units) << "<br>\n";
    else
        stream << QString(_("<b>Size:</b> %L1 %2")).arg(radius).arg(units) << "<br>\n";

#if 0
    if (body->getOblateness() != 0.0f && body->isEllipsoid())
    {
        stream << QString(_("<b>Oblateness: ")) << body->getOblateness() << "<br>\n";
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

        stream << QString(_("<b>Sidereal rotation period:</b> %L1 %2")).arg(rotPeriod).arg(units) << "<br>\n";
        if (dayLength != 0.0)
        {
            stream << QString(_("<b>Length of day:</b> %L1 %2")).arg(dayLength).arg(units) << "<br>\n";
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
        stream << QString(_("Osculating elements for %1")).arg(QString::fromUtf8(astro::TDBtoUTC(t).toCStr())) << "<br>\n";
        stream << "<br>\n";
//        stream << "<i>[ Orbit reference plane info goes here ]</i><br>\n";
        stream << QString(_("<b>Period:</b> %L1 %2")).arg(orbitalPeriod).arg(units) << "<br>\n";

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

        stream << QString(_("<b>Semi-major axis:</b> %L1 %2")).arg(sma).arg(units) << "<br>\n";
        stream << QString(_("<b>Eccentricity:</b> %L1")).arg(elements.eccentricity) << "<br>\n";
        stream << QString(_("<b>Inclination:</b> %L1%2")).arg(radToDeg(elements.inclination)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Pericenter distance:</b> %L1 %2")).arg(sma * (1 - elements.eccentricity)).arg(units) << "<br>\n";
        stream << QString(_("<b>Apocenter distance:</b> %L1 %2")).arg(sma * (1 + elements.eccentricity)).arg(units) << "<br>\n";

        stream << QString(_("<b>Ascending node:</b> %L1%2")).arg(radToDeg(elements.longAscendingNode)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Argument of periapsis:</b> %L1%2")).arg(radToDeg(elements.argPericenter)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Mean anomaly:</b> %L1%2")).arg(radToDeg(elements.meanAnomaly)).arg(QString::fromUtf8(UTF8_DEGREE_SIGN)) << "<br>\n";
        stream << QString(_("<b>Period (calculated):</b> %L1 %2")).arg(elements.period).arg(_("days")) << "<br>\n";;
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
    string name = ReplaceGreekLetterAbbr(universe->getDatabase().getObjectName(star, true));
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(name));

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
    stream << QString(_("<b>RA:</b> %L1h %L2m %L3s")).arg(hours).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>Dec:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                        .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
}


void InfoPanel::buildDSOPage(const DeepSkyObject* dso,
                             const Universe* universe,
                             QTextStream& stream)
{
    string name = universe->getDatabase().getObjectName(dso, true);
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(name));

    Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(dso->getPosition()));
    Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(radToDeg(sph.x()), hours, minutes, seconds);
    stream << QString(_("<b>RA:</b> %L1h %L2m %L3s")).arg(hours).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>Dec:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                        .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    Vector3d galPos = astro::equatorialToGalactic(eqPos);
    sph = rectToSpherical(galPos);

    astro::decimalToDegMinSec(radToDeg(sph.x()), degrees, minutes, seconds);
    stream << QString(_("<b>L:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                      .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
    astro::decimalToDegMinSec(radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>B:</b> %L1%2 %L3' %L4\"")).arg(degrees).arg(QString::fromUtf8(UTF8_DEGREE_SIGN))
                                                      .arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
}


void InfoPanel::updateHelper(ModelHelper* model, const QItemSelection& newSel, const QItemSelection& oldSel)
{
    if (!isVisible() || newSel == oldSel || newSel.indexes().length() == 0)
        return;

    Selection sel = model->itemForInfoPanel(newSel.indexes().at(0));
    if (!sel.empty())
    {
        buildInfoPage(sel,
                      appCore->getSimulation()->getUniverse(),
                      appCore->getSimulation()->getTime());
    }
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


