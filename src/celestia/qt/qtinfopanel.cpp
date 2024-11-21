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

#include "qtinfopanel.h"
#include "qtdateutil.h"

#include <cmath>
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <QIODevice>
#include <QItemSelection>
#include <QString>
#include <QTextBrowser>
#include <QTextStream>

#include <celastro/astro.h>
#include <celastro/date.h>
#include <celcompat/numbers.h>
#include <celengine/body.h>
#include <celengine/selection.h>
#include <celengine/universe.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celestia/celestiacore.h>
#include <celmath/mathlib.h>
#include <celutil/gettext.h>
#include <celutil/greek.h>
#include <celutil/logger.h>
#include <celutil/utf8.h>

using celestia::util::GetLogger;

namespace celestia::qt
{

namespace
{

astro::KeplerElements
CalculateOsculatingElements(const celestia::ephem::Orbit& orbit,
                            double t,
                            double dt)
{
    double sdt = dt;

    // If the trajectory is finite, make sure we sample
    // it inside the valid time interval.
    if (!orbit.isPeriodic())
    {
        double beginTime = 0.0;
        double endTime = 0.0;
        orbit.getValidRange(beginTime, endTime);
        GetLogger()->debug("t+dt: {}, endTime: {}\n", t + dt, endTime);
        if (t + dt > endTime)
        {
            GetLogger()->debug("REVERSE\n");
            sdt = -dt;
        }
    }

    Eigen::Vector3d p0 = orbit.positionAtTime(t);
    Eigen::Vector3d p1 = orbit.positionAtTime(t + sdt);
    Eigen::Vector3d v0 = orbit.velocityAtTime(t);
    Eigen::Vector3d v1 = orbit.velocityAtTime(t + sdt);

    double accel = ((v1 - v0) / sdt).norm();
    Eigen::Vector3d r = p0;
    double GM = accel * r.squaredNorm();

    GetLogger()->debug("vel: {}\n", v0.norm() / 86400.0);
    GetLogger()->debug("vel (est): {}\n", (p1 - p0).norm() / sdt / 86400);
    GetLogger()->debug("osc: {}, {}, {}, GM={}\n", t, dt, accel, GM);

    return astro::StateVectorToElements(p0, v0, GM);
}


QString
anchor(const QString& href, const QString& text)
{
    return QString("<a href=\"%1\">%2</a>").arg(href, text);
}



Eigen::Vector3d
celToJ2000Ecliptic(const Eigen::Vector3d& p)
{
    return Eigen::Vector3d(p.x(), -p.z(), p.y());
}


Eigen::Vector3d
rectToSpherical(const Eigen::Vector3d& v)
{
    double r = v.norm();
    double theta = std::atan2(v.y(), v.x());
    if (theta < 0)
        theta = theta + 2.0 * celestia::numbers::pi;
    double phi = std::asin(v.z() / r);

    return Eigen::Vector3d(theta, phi, r);
}

} // end unnamed namespace


InfoPanel::InfoPanel(CelestiaCore* _appCore, const QString& title, QWidget* parent) :
    QDockWidget(title, parent),
    appCore(_appCore)
{
    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    setWidget(textBrowser);
}


void
InfoPanel::buildInfoPage(Selection sel,
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


void
InfoPanel::pageHeader(QTextStream& stream)
{
    stream << "<html><head><title>" << QString(_("Info")) << "</title></head><body>";
}


void
InfoPanel::pageFooter(QTextStream& stream)
{
    stream << "</body></html>";
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

    bool isArtificial = body->getClassification() == BodyClassification::Spacecraft;

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
    const celestia::ephem::Orbit* orbit = body->getOrbit(t);
    if (orbit->isPeriodic())
        orbitalPeriod = orbit->getPeriod();

    // Show rotation information for natural, periodic rotators
    if (body->getRotationModel(t)->isPeriodic() && !isArtificial)
    {
        const celestia::ephem::RotationModel* rotationModel = body->getRotationModel(t);
        double rotPeriod = rotationModel->getPeriod();

        double dayLength = 0.0;
        bool prograde = false;
        if (orbitalPeriod > 0.0)
        {
            Eigen::Vector3d axis = Eigen::AngleAxisd(rotationModel->equatorOrientationAtTime(t)
                                                     * body->getBodyFrame(t)->getOrientation(t)).axis();
            Eigen::Vector3d orbitNormal = body->getOrbitFrame(t)->getOrientation(t)
                                        * orbit->positionAtTime(t).cross(orbit->velocityAtTime(t));
            prograde = axis.dot(orbitNormal) >= 0;
            double siderealDaysPerYear = orbitalPeriod / rotPeriod;
            double solarDaysPerYear = prograde ? siderealDaysPerYear - 1.0 : siderealDaysPerYear + 1.0;
            if (std::abs(solarDaysPerYear) > 0.0001)
            {
                dayLength = std::abs(orbitalPeriod / solarDaysPerYear);
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
        if (orbitalPeriod > 0.0)
        {
            QString direction = prograde ? _("Prograde") : _("Retrograde");
            stream << QString(_("<b>Rotation direction:</b> %1")).arg(direction) << "<br>\n";
        }
        if (dayLength != 0.0)
        {
            stream << QString(_("<b>Length of day:</b> %L1 %2")).arg(dayLength).arg(units) << "<br>\n";
        }
    }

    // Orbit information
    auto elements = CalculateOsculatingElements(*orbit, t, orbitalPeriod > 0.0 ? orbitalPeriod * 1e-6 : 3.6525e-4);
    const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();
    if (bodyFeaturesManager->getRings(body) != nullptr)
        stream << QString(_("<b>Has rings</b>")) << "<br>\n";
    if (bodyFeaturesManager->getAtmosphere(body) != nullptr)
        stream << QString(_("<b>Has atmosphere</b>")) << "<br>\n";

    // Start and end dates
    double startTime = 0.0;
    double endTime = 0.0;
    body->getLifespan(startTime, endTime);

    if (startTime > -1.0e9)
        stream << "<br>" << QString(_("<b>Start:</b> %1")).arg(TDBToQString(startTime)) << "<br>\n";

    if (endTime < 1.0e9)
        stream << "<br>" << QString(_("<b>End:</b> %1")).arg(TDBToQString(endTime)) << "<br>\n";

    stream << "<br><big><b>" << QString(_("Orbit information")) << "</b></big><br>\n";
    stream << QString(_("Osculating elements for %1")).arg(TDBToQString(t)) << "<br>\n";
    stream << "<br>\n";

    if (orbitalPeriod > 0.0)
    {
        if (orbitalPeriod < 2.0)
        {
            orbitalPeriod *= 24.0;
            units = _("hours");
        }
        else if (orbitalPeriod < 365.25 * 2.0)
        {
            units = _("days");
        }
        else
        {
            units = _("years");
            orbitalPeriod /= 365.25;
        }

        stream << QString(_("<b>Period:</b> %L1 %2")).arg(orbitalPeriod).arg(units) << "<br>\n";
    }

    double sma = elements.semimajorAxis;
    if (std::abs(sma) > 2.5e7)
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
    stream << QString(_("<b>Inclination:</b> %L1°")).arg(math::radToDeg(elements.inclination)) << "<br>\n";
    stream << QString(_("<b>Pericenter distance:</b> %L1 %2")).arg(sma * (1 - elements.eccentricity)).arg(units) << "<br>\n";
    if (elements.eccentricity < 1.0)
        stream << QString(_("<b>Apocenter distance:</b> %L1 %2")).arg(sma * (1 + elements.eccentricity)).arg(units) << "<br>\n";

    stream << QString(_("<b>Ascending node:</b> %L1°")).arg(math::radToDeg(elements.longAscendingNode)) << "<br>\n";
    stream << QString(_("<b>Argument of periapsis:</b> %L1°")).arg(math::radToDeg(elements.argPericenter)) << "<br>\n";
    stream << QString(_("<b>Mean anomaly:</b> %L1°")).arg(math::radToDeg(elements.meanAnomaly)) << "<br>\n";
    if (elements.eccentricity < 1.0)
    {
        stream << QString(_("<b>Period (calculated):</b> %L1 %2")).arg(elements.period).arg(_("days")) << "<br>\n";
    }
    else
    {
        stream << QString(_("<b>Mean motion (calculated):</b> %L1°/day")).arg(360.0 / elements.period) << "<br>\n";
    }
}


void
InfoPanel::buildStarPage(const Star* star, const Universe* universe, double tdb, QTextStream& stream)
{
    std::string name = ReplaceGreekLetterAbbr(universe->getStarCatalog()->getStarName(*star, true));
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(name));

    // Compute the star's position relative to the Solar System Barycenter. Note that
    // this will ignore the effect of parallax in the star's position.
    // TODO: Use either the observer's position or the Earth's position as the
    // origin instead.
    Eigen::Vector3d celPos = star->getPosition(tdb).offsetFromKm(UniversalCoord::Zero());
    Eigen::Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(celPos));
    Eigen::Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(math::radToDeg(sph.x()), hours, minutes, seconds);
    stream << QString(_("<b>RA:</b> %L1h %L2m %L3s")).arg(hours).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(math::radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>Dec:</b> %L1° %L2′ %L3″")).arg(degrees).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
}


void InfoPanel::buildDSOPage(const DeepSkyObject* dso,
                             const Universe* universe,
                             QTextStream& stream)
{
    std::string name = universe->getDSOCatalog()->getDSOName(dso, true);
    stream << QString("<h1>%1</h1>").arg(QString::fromStdString(name));

    Eigen::Vector3d eqPos = astro::eclipticToEquatorial(celToJ2000Ecliptic(dso->getPosition()));
    Eigen::Vector3d sph = rectToSpherical(eqPos);

    int hours = 0;
    int minutes = 0;
    double seconds = 0;
    astro::decimalToHourMinSec(math::radToDeg(sph.x()), hours, minutes, seconds);
    stream << QString(_("<b>RA:</b> %L1h %L2m %L3s")).arg(hours).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    int degrees = 0;
    astro::decimalToDegMinSec(math::radToDeg(sph.y()), degrees, minutes, seconds);
    stream << QString(_("<b>Dec:</b> %L1° %L2′ %L3″")).arg(degrees).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";

    Eigen::Vector3d galPos = astro::equatorialToGalactic(eqPos);
    sph = rectToSpherical(galPos);

    astro::decimalToDegMinSec(math::radToDeg(sph.x()), degrees, minutes, seconds);
    // TRANSLATORS: Galactic longitude
    stream << QString(_("<b>L:</b> %L1° %L2′ %L3″")).arg(degrees).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
    astro::decimalToDegMinSec(math::radToDeg(sph.y()), degrees, minutes, seconds);
    // TRANSLATORS: Galactic latitude
    stream << QString(_("<b>B:</b> %L1° %L2′ %L3″")).arg(degrees).arg(abs(minutes)).arg(abs(seconds)) << "<br>\n";
}


void
InfoPanel::updateHelper(ModelHelper* model, const QItemSelection& newSel, const QItemSelection& oldSel)
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

} // end namespace celestia::qt
