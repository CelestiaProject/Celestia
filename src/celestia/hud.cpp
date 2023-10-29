// hud.cpp
//
// Copyright (C) 2023, the Celestia Development Team
//
// Split out from celestiacore.h/celestiacore.cpp
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "hud.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include <Eigen/Geometry>

#include <fmt/format.h>

#include <celcompat/numbers.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/observer.h>
#include <celengine/overlay.h>
#include <celengine/overlayimage.h>
#include <celengine/rectangle.h>
#include <celengine/simulation.h>
#include <celengine/star.h>
#include <celengine/textlayout.h>
#include <celengine/universe.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celutil/formatnum.h>
#include <celutil/gettext.h>
#include <celutil/utf8.h>
#include "moviecapture.h"
#include "textprintposition.h"
#include "timeinfo.h"
#include "view.h"
#include "viewmanager.h"

using namespace celestia::astro::literals;

namespace celestia
{

namespace
{

// Ye olde wolde conſtantes for ye olde wolde units
constexpr double OneMiInKm = 1.609344;
constexpr double OneFtInKm = 0.0003048;
constexpr double OneLbInKg = 0.45359237;
constexpr double OneLbPerFt3InKgPerM3 = OneLbInKg / celmath::cube(OneFtInKm * 1000.0);

constexpr float
KelvinToCelsius(float kelvin)
{
    return kelvin - 273.15f;
}

constexpr float
KelvinToFahrenheit(float kelvin)
{
    return kelvin * 1.8f - 459.67f;
}

FormattedNumber
SigDigitNum(double v, int digits)
{
    return FormattedNumber(v, digits,
                           FormattedNumber::GroupThousands |
                           FormattedNumber::SignificantDigits);
}

std::string
KelvinToStr(float value, int digits, TemperatureScale temperatureScale)
{
    const char* unitTemplate = "";

    switch (temperatureScale)
    {
        case TemperatureScale::Celsius:
            value = KelvinToCelsius(value);
            unitTemplate = "{} °C";
            break;
        case TemperatureScale::Fahrenheit:
            value = KelvinToFahrenheit(value);
            unitTemplate = "{} °F";
            break;
        default: // TemperatureScale::Kelvin
            unitTemplate = "{} K";
            break;
    }
    return fmt::format(unitTemplate, SigDigitNum(value, digits));
}

std::string
DistanceLyToStr(double distance, int digits, MeasurementSystem measurement)
{
    const char* units = "";

    if (std::abs(distance) >= astro::parsecsToLightYears(1e+6))
    {
        units = _("Mpc");
        distance = astro::lightYearsToParsecs(distance) / 1e+6;
    }
    else if (std::abs(distance) >= 0.5 * astro::parsecsToLightYears(1e+3))
    {
        units = _("kpc");
        distance = astro::lightYearsToParsecs(distance) / 1e+3;
    }
    else if (std::abs(distance) >= astro::AUtoLightYears(1000.0f))
    {
        units = _("ly");
    }
    else if (std::abs(distance) >= astro::kilometersToLightYears(10000000.0))
    {
        units = _("au");
        distance = astro::lightYearsToAU(distance);
    }
    else if (measurement == MeasurementSystem::Imperial)
    {
        if (abs(distance) > astro::kilometersToLightYears(OneMiInKm))
        {
            units = _("mi");
            distance = astro::lightYearsToKilometers(distance) / OneMiInKm;
        }
        else
        {
            units = _("ft");
            distance = astro::lightYearsToKilometers(distance) / OneFtInKm;
        }
    }
    else
    {
        if (std::abs(distance) > astro::kilometersToLightYears(1.0f))
        {
            units = _("km");
            distance = astro::lightYearsToKilometers(distance);
        }
        else
        {
            units = _("m");
            distance = astro::lightYearsToKilometers(distance) * 1000.0f;
        }
    }

    return fmt::format("{} {}", SigDigitNum(distance, digits), units);
}

std::string
DistanceKmToStr(double distance, int digits, MeasurementSystem measurement)
{
    return DistanceLyToStr(astro::kilometersToLightYears(distance), digits, measurement);
}

void
displayRotationPeriod(Overlay& overlay, double days)
{
    FormattedNumber n;
    const char *p;

    if (days > 1.0)
    {
        n = FormattedNumber(days, 3, FormattedNumber::GroupThousands);
        p = _("days");
    }
    else if (days > 1.0 / 24.0)
    {
        n = FormattedNumber(days * 24.0, 3, FormattedNumber::GroupThousands);
        p = _("hours");
    }
    else if (days > 1.0 / (24.0 * 60.0))
    {
        n = FormattedNumber(days * 24.0 * 60.0, 3, FormattedNumber::GroupThousands);
        p = _("minutes");
    }
    else
    {
        n = FormattedNumber(days * 24.0 * 60.0 * 60.0, 3, FormattedNumber::GroupThousands);
        p = _("seconds");
    }

    overlay.print(_("Rotation period: {} {}\n"), n, p);
}

void
displayMass(Overlay& overlay, float mass, MeasurementSystem measurement)
{
    if (mass < 0.001f)
    {
        if (measurement == MeasurementSystem::Imperial)
            overlay.printf(_("Mass: %.6g lb\n"), mass * astro::EarthMass / (float) OneLbInKg);
        else
            overlay.printf(_("Mass: %.6g kg\n"), mass * astro::EarthMass);
    }
    else if (mass > 50)
        overlay.printf(_("Mass: %.2f Mj\n"), mass * astro::EarthMass / astro::JupiterMass);
    else
        overlay.printf(_("Mass: %.2f Me\n"), mass);
}

void
displaySpeed(Overlay& overlay, float speed, MeasurementSystem measurement)
{
    FormattedNumber n;
    const char *u;

    if (speed >= (float) 1000.0_au)
    {
        n = SigDigitNum(astro::kilometersToLightYears(speed), 3);
        u = _("ly/s");
    }
    else if (speed >= (float) 100.0_c)
    {
        n = SigDigitNum(astro::kilometersToAU(speed), 3);
        u = _("AU/s");
    }
    else if (speed >= 10000.0f)
    {
        n = SigDigitNum(speed / astro::speedOfLight, 3);
        u = "c";
    }
    else if (measurement == MeasurementSystem::Imperial)
    {
        if (speed >= (float) OneMiInKm)
        {
            n = SigDigitNum(speed / (float) OneMiInKm, 3);
            u = _("mi/s");
        }
        else
        {
            n = SigDigitNum(speed / (float) OneFtInKm, 3);
            u = _("ft/s");
        }
    }
    else
    {
        if (speed >= 1.0f)
        {
            n = SigDigitNum(speed, 3);
            u = _("km/s");
        }
        else
        {
            n = SigDigitNum(speed * 1000.0f, 3);
            u = _("m/s");
        }
    }
    overlay.print(_("Speed: {} {}\n"), n, u);
}

// Display a positive angle as degrees, minutes, and seconds. If the angle is less than one
// degree, only minutes and seconds are shown; if the angle is less than one minute, only
// seconds are displayed.
std::string
angleToStr(double angle)
{
    int degrees, minutes;
    double seconds;
    astro::decimalToDegMinSec(angle, degrees, minutes, seconds);

    if (degrees > 0)
    {
        return fmt::format("{}" UTF8_DEGREE_SIGN "{:02d}' {:.1f}\"",
                           degrees, abs(minutes), abs(seconds));
    }

    if (minutes > 0)
    {
        return fmt::format("{:02d}' {:.1f}\"", abs(minutes), abs(seconds));
    }

    return fmt::format("{:.2f}\"", abs(seconds));
}

void
displayApparentDiameter(Overlay& overlay, double radius, double distance)
{
    if (distance < radius)
        return;

    double arcSize = celmath::radToDeg(std::asin(radius / distance) * 2.0);

    // Only display the arc size if it's less than 160 degrees and greater
    // than one second--otherwise, it's probably not interesting data.
    if (arcSize < 160.0 && arcSize > 1.0 / 3600.0)
        overlay.printf(_("Apparent diameter: %s\n"), angleToStr(arcSize));
}

void
displayDeclination(Overlay& overlay, double angle)
{
    int degrees, minutes;
    double seconds;
    astro::decimalToDegMinSec(angle, degrees, minutes, seconds);

    overlay.printf(_("Dec: %+d%s %02d' %.1f\"\n"),
                   std::abs(degrees), UTF8_DEGREE_SIGN,
                   std::abs(minutes), std::abs(seconds));
}

void
displayRightAscension(Overlay& overlay, double angle)
{
    int hours, minutes;
    double seconds;
    astro::decimalToHourMinSec(angle, hours, minutes, seconds);

    overlay.printf(_("RA: %dh %02dm %.1fs\n"),
                          hours, abs(minutes), abs(seconds));
}

void
displayApparentMagnitude(Overlay& overlay,
                         float absMag,
                         double distance)
{
    if (distance > 32.6167)
    {
        float appMag = astro::absToAppMag(absMag, (float) distance);
        overlay.printf(_("Apparent magnitude: %.1f\n"), appMag);
    }
    else
    {
        overlay.printf(_("Absolute magnitude: %.1f\n"), absMag);
    }
}

void
displayRADec(Overlay& overlay, const Eigen::Vector3d& v)
{
    double phi = std::atan2(v.x(), v.z()) - celestia::numbers::pi / 2;
    if (phi < 0)
        phi = phi + 2 * celestia::numbers::pi;

    double theta = std::atan2(std::hypot(v.x(), v.z()), v.y());
    if (theta > 0)
        theta = celestia::numbers::pi / 2 - theta;
    else
        theta = -celestia::numbers::pi / 2 - theta;


    displayRightAscension(overlay, celmath::radToDeg(phi));
    displayDeclination(overlay, celmath::radToDeg(theta));
}

// Display nicely formatted planetocentric/planetographic coordinates.
// The latitude and longitude parameters are angles in radians, altitude
// is in kilometers.
void
displayPlanetocentricCoords(Overlay& overlay,
                            const Body& body,
                            double longitude,
                            double latitude,
                            double altitude,
                            bool showAltitude,
                            MeasurementSystem measurement)
{
    char ewHemi = ' ';
    char nsHemi = ' ';
    double lon = 0.0;
    double lat = 0.0;

    // Terrible hack for Earth and Moon longitude conventions.  Fix by
    // adding a field to specify the longitude convention in .ssc files.
    if (body.getName() == "Earth" || body.getName() == "Moon")
    {
        if (latitude < 0.0)
            nsHemi = 'S';
        else if (latitude > 0.0)
            nsHemi = 'N';

        if (longitude < 0.0)
            ewHemi = 'W';
        else if (longitude > 0.0f)
            ewHemi = 'E';

        lon = static_cast<float>(std::abs(celmath::radToDeg(longitude)));
        lat = static_cast<float>(std::abs(celmath::radToDeg(latitude)));
    }
    else
    {
        // Swap hemispheres if the object is a retrograde rotator
        Eigen::Quaterniond q = body.getEclipticToEquatorial(astro::J2000);
        bool retrograde = (q * Eigen::Vector3d::UnitY()).y() < 0.0;

        if ((latitude < 0.0) ^ retrograde)
            nsHemi = 'S';
        else if ((latitude > 0.0) ^ retrograde)
            nsHemi = 'N';

        if (retrograde)
            ewHemi = 'E';
        else
            ewHemi = 'W';

        lon = -celmath::radToDeg(longitude);
        if (lon < 0.0)
            lon += 360.0;
        lat = std::abs(celmath::radToDeg(latitude));
    }

    if (showAltitude)
        overlay.printf("%.6f%c %.6f%c", lat, nsHemi, lon, ewHemi);
    else
        overlay.printf(_("%.6f%c %.6f%c %s"), lat, nsHemi, lon, ewHemi, DistanceKmToStr(altitude, 5, measurement));
}

void
displayStarInfo(Overlay& overlay,
                int detail,
                Star& star,
                const Universe& universe,
                double distance,
                MeasurementSystem measurement,
                TemperatureScale temperatureScale)
{
    overlay.printf(_("Distance: %s\n"), DistanceLyToStr(distance, 5, measurement));

    if (!star.getVisibility())
    {
        overlay.print(_("Star system barycenter\n"));
    }
    else
    {
        overlay.printf(_("Abs (app) mag: %.2f (%.2f)\n"),
                                star.getAbsoluteMagnitude(),
                                star.getApparentMagnitude(float(distance)));

        if (star.getLuminosity() > 1.0e-10f)
            overlay.print(_("Luminosity: {}x Sun\n"), SigDigitNum(star.getLuminosity(), 3));

        const char* star_class;
        switch (star.getSpectralType()[0])
        {
        case 'Q':
            star_class = _("Neutron star");
            break;
        case 'X':
            star_class = _("Black hole");
            break;
        default:
            star_class = star.getSpectralType();
        };
        overlay.printf(_("Class: %s\n"), star_class);

        displayApparentDiameter(overlay, star.getRadius(),
                                astro::lightYearsToKilometers(distance));

        if (detail > 1)
        {
            overlay.printf(_("Surface temp: %s\n"), KelvinToStr(star.getTemperature(), 3, temperatureScale));
            float solarRadii = star.getRadius() / 6.96e5f;

            if (solarRadii > 0.01f)
            {
                overlay.print(_("Radius: {} Rsun  ({})\n"),
                              SigDigitNum(star.getRadius() / 696000.0f, 2),
                              DistanceKmToStr(star.getRadius(), 3, measurement));
            }
            else
            {
                overlay.print(_("Radius: {}\n"),
                              DistanceKmToStr(star.getRadius(), 3, measurement));
            }

            if (star.getRotationModel()->isPeriodic())
            {
                float period = (float) star.getRotationModel()->getPeriod();
                displayRotationPeriod(overlay, period);
            }
        }
    }

    if (detail > 1)
    {
        SolarSystem* sys = universe.getSolarSystem(&star);
        if (sys != nullptr && sys->getPlanets()->getSystemSize() != 0)
            overlay.print(_("Planetary companions present\n"));
    }
}

void displayDSOinfo(Overlay& overlay,
                    const DeepSkyObject& dso,
                    double distance,
                    MeasurementSystem measurement)
{
    overlay.print(dso.getDescription());
    overlay.print("\n");

    if (distance >= 0)
    {
        overlay.printf(_("Distance: %s\n"),
                     DistanceLyToStr(distance, 5, measurement));
    }
    else
    {
        overlay.printf(_("Distance from center: %s\n"),
                     DistanceLyToStr(distance + dso.getRadius(), 5, measurement));
     }
    overlay.printf(_("Radius: %s\n"),
                 DistanceLyToStr(dso.getRadius(), 5, measurement));

    displayApparentDiameter(overlay, dso.getRadius(), distance);
    if (dso.getAbsoluteMagnitude() > DSO_DEFAULT_ABS_MAGNITUDE)
    {
        displayApparentMagnitude(overlay,
                                 dso.getAbsoluteMagnitude(),
                                 distance);
    }
}

static void displayPlanetInfo(Overlay& overlay,
                              int detail,
                              Body& body,
                              double t,
                              double distanceKm,
                              const Eigen::Vector3d& viewVec,
                              MeasurementSystem measurement,
                              TemperatureScale temperatureScale)
{
    double distance = distanceKm - body.getRadius();
    overlay.printf(_("Distance: %s\n"), DistanceKmToStr(distance, 5, measurement));

    if (body.getClassification() == Body::Invisible)
    {
        return;
    }

    overlay.printf(_("Radius: %s\n"), DistanceKmToStr(body.getRadius(), 5, measurement));

    displayApparentDiameter(overlay, body.getRadius(), distanceKm);

    // Display the phase angle

    // Find the parent star of the body. This can be slightly complicated if
    // the body orbits a barycenter instead of a star.
    Selection parent = Selection(&body).parent();
    while (parent.body() != nullptr)
        parent = parent.parent();

    if (parent.star() != nullptr)
    {
        bool showPhaseAngle = false;

        Star* sun = parent.star();
        if (sun->getVisibility())
        {
            showPhaseAngle = true;
        }
        else if (const auto* orbitingStars = sun->getOrbitingStars(); orbitingStars != nullptr && orbitingStars->size() == 1)
        {
            // The planet's orbit is defined with respect to a barycenter. If there's
            // a single star orbiting the barycenter, we'll compute the phase angle
            // for the planet with respect to that star. If there are no stars, the
            // planet is an orphan, drifting through space with no star. We also skip
            // displaying the phase angle when there are multiple stars (for now.)
            sun = orbitingStars->front();
            showPhaseAngle = sun->getVisibility();
        }

        if (showPhaseAngle)
        {
            Eigen::Vector3d sunVec = Selection(&body).getPosition(t).offsetFromKm(Selection(sun).getPosition(t));
            sunVec.normalize();
            double cosPhaseAngle = std::clamp(sunVec.dot(viewVec.normalized()), -1.0, 1.0);
            double phaseAngle = acos(cosPhaseAngle);
            overlay.printf(_("Phase angle: %.1f%s\n"), celmath::radToDeg(phaseAngle), UTF8_DEGREE_SIGN);
        }
    }

    if (detail > 1)
    {
        if (body.getRotationModel(t)->isPeriodic())
            displayRotationPeriod(overlay, body.getRotationModel(t)->getPeriod());

        if (body.getName() != "Earth" && body.getMass() > 0)
            displayMass(overlay, body.getMass(), measurement);

        float density = body.getDensity();
        if (density > 0)
        {
            if (measurement == MeasurementSystem::Imperial)
                overlay.printf(_("Density: %.2f x 1000 lb/ft^3\n"), density / (float) OneLbPerFt3InKgPerM3 / 1000.0f);
            else
                overlay.printf(_("Density: %.2f x 1000 kg/m^3\n"), density / 1000.0f);
        }

        float planetTemp = body.getTemperature(t);
        if (planetTemp > 0)
            overlay.printf(_("Temperature: %s\n"), KelvinToStr(planetTemp, 3, temperatureScale));
    }
}

void
displayLocationInfo(Overlay& overlay,
                    Location& location,
                    double distanceKm,
                    MeasurementSystem measurement)
{
    overlay.printf(_("Distance: %s\n"), DistanceKmToStr(distanceKm, 5, measurement));

    Body* body = location.getParentBody();
    if (body == nullptr)
        return;

    Eigen::Vector3f locPos = location.getPosition();
    Eigen::Vector3d lonLatAlt = body->cartesianToPlanetocentric(locPos.cast<double>());
    displayPlanetocentricCoords(overlay, *body,
                                lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z(), false, measurement);
}

std::string
getSelectionName(const Selection& sel, const Universe& univ)
{
    switch (sel.getType())
    {
    case SelectionType::Body:
        return sel.body()->getName(true);
    case SelectionType::DeepSky:
        return univ.getDSOCatalog()->getDSOName(sel.deepsky(), true);
    case SelectionType::Star:
        return univ.getStarCatalog()->getStarName(*sel.star(), true);
    case SelectionType::Location:
        return sel.location()->getName(true);
    default:
        return {};
    }
}

std::string
getBodySelectionNames(const Body& body)
{
    std::string selectionNames = body.getLocalizedName(); // Primary name, might be localized
    const std::vector<std::string>& names = body.getNames();

    // Start from the second one because primary name is already in the string
    auto secondName = names.begin() + 1;

    for (auto iter = secondName; iter != names.end(); ++iter)
    {
        selectionNames += " / ";

        // Use localized version of parent name in alternative names.
        std::string alias = *iter;

        const PlanetarySystem* parentSystem = body.getSystem();
        if (parentSystem == nullptr)
        {
            selectionNames += alias;
            continue;
        }

        if (const Body* parentBody = parentSystem->getPrimaryBody(); parentBody != nullptr)
        {
            std::string parentName = parentBody->getName();
            std::string locParentName = parentBody->getName(true);
            std::string::size_type startPos = alias.find(parentName);
            if (startPos != std::string::npos)
                alias.replace(startPos, parentName.length(), locParentName);
        }

        selectionNames += alias;
    }

    return selectionNames;
}

} // end unnamed namespace

Hud::~Hud() = default;

int
Hud::getDetail() const
{
    return hudDetail;
}

void
Hud::setDetail(int detail)
{
    hudDetail = detail % 3;
}

astro::Date::Format
Hud::getDateFormat() const
{
    return dateFormat;
}

void
Hud::setDateFormat(astro::Date::Format format)
{
    dateFormat = format;
    dateStrWidth = 0;
}

TextInput&
Hud::textInput()
{
    return m_textInput;
}

unsigned int
Hud::getTextEnterMode() const
{
    return m_textEnterMode;
}

void
Hud::setTextEnterMode(unsigned int value)
{
    m_textEnterMode = static_cast<TextEnterMode>(value);
    if ((value & TextEnterAutoComplete) == 0)
        m_textInput.reset();
}

void
Hud::setOverlay(std::unique_ptr<Overlay>&& _overlay)
{
    overlay = std::move(_overlay);
}

void
Hud::setWindowSize(int w, int h)
{
    if (overlay != nullptr)
        overlay->setWindowSize(w, h);
}

void
Hud::setTextAlignment(LayoutDirection dir)
{
    overlay->setTextAlignment(dir == LayoutDirection::RightToLeft
                                  ? engine::TextLayout::HorizontalAlignment::Right
                                  : engine::TextLayout::HorizontalAlignment::Left);
}

int
Hud::getTextWidth(std::string_view text) const
{
    return engine::TextLayout::getTextWidth(text, titleFont.get());
}

const std::shared_ptr<TextureFont>&
Hud::getFont() const
{
    return font;
}

const std::shared_ptr<TextureFont>&
Hud::getTitleFont() const
{
    return titleFont;
}

std::tuple<int, int>
Hud::getTitleMetrics() const
{
    return std::make_tuple(titleEmWidth, titleFontHeight);
}

void
Hud::renderOverlay(const WindowMetrics& metrics,
                   const Simulation* sim,
                   const ViewManager& views,
                   const MovieCapture* movieCapture,
                   const TimeInfo& timeInfo,
                   bool isScriptRunning,
                   bool editMode)
{
    overlay->setFont(font);

    overlay->begin();

    if (showOverlayImage && isScriptRunning && image != nullptr)
        image->render(static_cast<float>(timeInfo.currentTime), metrics.width, metrics.height);

    if (views.views().size() > 1)
        renderViewBorders(metrics, timeInfo.currentTime, views, views.isResizing());

    setlocale(LC_NUMERIC, "");

    if (hudDetail > 0 && (overlayElements & OverlayElements::ShowTime) != 0)
        renderTimeInfo(metrics, sim, timeInfo);

    if (hudDetail > 0 && (overlayElements & OverlayElements::ShowVelocity) != 0)
    {
        // Speed
        overlay->savePos();
        overlay->moveBy(metrics.getSafeAreaStart(),
                        metrics.getSafeAreaBottom(fontHeight * 2 + static_cast<int>(static_cast<float>(metrics.screenDpi) / 25.4f * 1.3f)));
        overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);

        overlay->beginText();
        overlay->print("\n");
        if (showFPSCounter)
            overlay->printf(_("FPS: %.1f\n"), timeInfo.fps);
        else
            overlay->print("\n");

        displaySpeed(*overlay, sim->getObserver().getVelocity().norm(), measurementSystem);

        overlay->endText();
        overlay->restorePos();
    }

    if (hudDetail > 0 && (overlayElements & OverlayElements::ShowFrame) != 0)
        renderFrameInfo(metrics, sim);

    if (Selection sel = sim->getSelection();
        !sel.empty() && hudDetail > 0 && (overlayElements & OverlayElements::ShowSelection) != 0)
    {
        Eigen::Vector3d v = sel.getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
        renderSelectionInfo(metrics, sim, sel, v);
    }

    if ((m_textEnterMode & TextEnterAutoComplete) != 0)
        renderTextInput(metrics);

    if (showMessage)
        renderTextMessages(metrics, timeInfo.currentTime);

    if (movieCapture != nullptr)
        renderMovieCapture(metrics, *movieCapture);

    if (editMode)
    {
        overlay->savePos();
        overlay->beginText();
        int x = (metrics.getSafeAreaWidth() - engine::TextLayout::getTextWidth(_("Edit Mode"), font.get())) / 2;
        overlay->moveBy(metrics.getSafeAreaStart(x), metrics.getSafeAreaTop(fontHeight));
        overlay->setColor(1, 0, 1, 1);
        overlay->print(_("Edit Mode"));
        overlay->endText();
        overlay->restorePos();
    }

    overlay->end();
    setlocale(LC_NUMERIC, "C");
}

void
Hud::renderViewBorders(const WindowMetrics& metrics,
                       double currentTime,
                       const ViewManager& views,
                       bool resizeSplit)
{
    // Render a thin border arround all views
    if (showViewFrames || resizeSplit)
    {
        for(const auto v : views.views())
        {
            if (v->type == View::ViewWindow)
                v->drawBorder(metrics.width, metrics.height, frameColor);
        }
    }

    // Render a very simple border around the active view
    const View* av = views.activeView();

    if (showActiveViewFrame)
    {
        av->drawBorder(metrics.width, metrics.height, activeFrameColor, 2);
    }

    if (currentTime < views.flashFrameStart() + 0.5)
    {
        float alpha = (float) (1.0 - (currentTime - views.flashFrameStart()) / 0.5);
        av->drawBorder(metrics.width, metrics.height, {activeFrameColor, alpha}, 8);
    }
}

void
Hud::renderTimeInfo(const WindowMetrics& metrics, const Simulation* sim, const TimeInfo& timeInfo)
{
    double lt = 0.0;

    if (sim->getSelection().getType() == SelectionType::Body &&
        (sim->getTargetSpeed() < 0.99_c))
    {
        if (timeInfo.lightTravelFlag)
        {
            Eigen::Vector3d v = sim->getSelection().getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
            // light travel time in days
            lt = v.norm() / (86400.0_c);
        }
    }

    double tdb = sim->getTime() + lt;
    auto dateStr = dateFormatter->formatDate(tdb, timeInfo.timeZoneBias != 0, dateFormat);
    int dateWidth = (engine::TextLayout::getTextWidth(dateStr, font.get()) / (emWidth * 3) + 2) * emWidth * 3;
    if (dateWidth > dateStrWidth) dateStrWidth = dateWidth;

    // Time and date
    overlay->savePos();
    overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);
    overlay->moveBy(metrics.getSafeAreaEnd(dateStrWidth), metrics.getSafeAreaTop(fontHeight));
    overlay->beginText();

    overlay->print(dateStr);

    if (timeInfo.lightTravelFlag && lt > 0.0)
    {
        overlay->setColor(0.42f, 1.0f, 1.0f, 1.0f);
        overlay->print(_("  LT"));
        overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);
    }
    overlay->print("\n");

    {
        if (std::abs(std::abs(sim->getTimeScale()) - 1.0) < 1e-6)
        {
            if (celmath::sign(sim->getTimeScale()) == 1)
                overlay->print(_("Real time"));
            else
                overlay->print(_("-Real time"));
        }
        else if (std::abs(sim->getTimeScale()) < TimeInfo::MinimumTimeRate)
        {
            overlay->print(_("Time stopped"));
        }
        else if (std::abs(sim->getTimeScale()) > 1.0)
        {
            overlay->printf(_("%.6g x faster"), sim->getTimeScale()); // XXX: %'.12g
        }
        else
        {
            overlay->printf(_("%.6g x slower"), 1.0 / sim->getTimeScale()); // XXX: %'.12g
        }

        if (sim->getPauseState() == true)
        {
            overlay->setColor(1.0f, 0.0f, 0.0f, 1.0f);
            overlay->print(_(" (Paused)"));
        }
    }

    overlay->endText();
    overlay->restorePos();
}

void
Hud::renderFrameInfo(const WindowMetrics& metrics, const Simulation* sim)
{
    // Field of view and camera mode in lower right corner
    overlay->savePos();
    overlay->moveBy(metrics.getSafeAreaEnd(emWidth * 15),
                    metrics.getSafeAreaBottom(fontHeight * 3 + static_cast<int>(static_cast<float>(metrics.screenDpi) / 25.4f * 1.3f)));
    overlay->beginText();
    overlay->setColor(0.6f, 0.6f, 1.0f, 1);

    if (sim->getObserverMode() == Observer::Travelling)
    {
        double timeLeft = sim->getArrivalTime() - sim->getRealTime();
        if (timeLeft >= 1)
            overlay->print(_("Travelling ({})\n"),
                            FormattedNumber(timeLeft, 0, FormattedNumber::GroupThousands));
        else
            overlay->print(_("Travelling\n"));
    }
    else
    {
        overlay->print("\n");
    }

    const Universe& u = *sim->getUniverse();

    if (!sim->getTrackedObject().empty())
        overlay->printf(_("Track %s\n"), CX_("Track", getSelectionName(sim->getTrackedObject(), u)));
    else
        overlay->print("\n");

    {
        Selection refObject = sim->getFrame()->getRefObject();
        ObserverFrame::CoordinateSystem coordSys = sim->getFrame()->getCoordinateSystem();

        switch (coordSys)
        {
        case ObserverFrame::Ecliptical:
            overlay->printf(_("Follow %s\n"),
                            CX_("Follow", getSelectionName(refObject, u)));
            break;
        case ObserverFrame::BodyFixed:
            overlay->printf(_("Sync Orbit %s\n"),
                            CX_("Sync", getSelectionName(refObject, u)));
            break;
        case ObserverFrame::PhaseLock:
            overlay->printf(_("Lock %s -> %s\n"),
                            CX_("Lock", getSelectionName(refObject, u)),
                            CX_("LockTo", getSelectionName(sim->getFrame()->getTargetObject(), u)));
            break;

        case ObserverFrame::Chase:
            overlay->printf(_("Chase %s\n"),
                            CX_("Chase", getSelectionName(refObject, u)));
            break;

        default:
            overlay->print("\n");
            break;
        }
    }

    overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);

    // Field of view
    const Observer* activeObserver = sim->getActiveObserver();
    float fov = celmath::radToDeg(activeObserver->getFOV());
    overlay->printf(_("FOV: %s (%.2fx)\n"), angleToStr(fov), activeObserver->getZoom());
    overlay->endText();
    overlay->restorePos();
}

void
Hud::renderSelectionInfo(const WindowMetrics& metrics,
                         const Simulation* sim,
                         Selection sel,
                         const Eigen::Vector3d& v)
{
    overlay->savePos();
    overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);
    overlay->moveBy(metrics.getSafeAreaStart(), metrics.getSafeAreaTop(titleFont->getHeight()));

    overlay->beginText();

    switch (sel.getType())
    {
    case SelectionType::Star:
        {
            if (sel != lastSelection)
            {
                lastSelection = sel;
                selectionNames = sim->getUniverse()->getStarCatalog()->getStarNameList(*sel.star());
            }

            overlay->setFont(titleFont);
            overlay->print(selectionNames);
            overlay->setFont(font);
            overlay->print("\n");
            displayStarInfo(*overlay,
                            hudDetail,
                            *(sel.star()),
                            *(sim->getUniverse()),
                            astro::kilometersToLightYears(v.norm()),
                            measurementSystem,
                            temperatureScale);
        }
        break;

    case SelectionType::DeepSky:
        {
            if (sel != lastSelection)
            {
                lastSelection = sel;
                selectionNames = sim->getUniverse()->getDSOCatalog()->getDSONameList(sel.deepsky());
            }

            overlay->setFont(titleFont);
            overlay->print(selectionNames);
            overlay->setFont(font);
            overlay->print("\n");
            displayDSOinfo(*overlay,
                            *sel.deepsky(),
                            astro::kilometersToLightYears(v.norm()) - sel.deepsky()->getRadius(),
                            measurementSystem);
        }
        break;

    case SelectionType::Body:
        {
            // Show all names for the body
            if (sel != lastSelection)
            {
                lastSelection = sel;
                selectionNames = getBodySelectionNames(*sel.body());
            }

            overlay->setFont(titleFont);
            overlay->print(selectionNames);
            overlay->setFont(font);
            overlay->print("\n");
            displayPlanetInfo(*overlay,
                               hudDetail,
                              *(sel.body()),
                               sim->getTime(),
                               v.norm(),
                               v,
                               measurementSystem,
                               temperatureScale);
        }
        break;

    case SelectionType::Location:
        overlay->setFont(titleFont);
        overlay->print(sel.location()->getName(true).c_str());
        overlay->setFont(font);
        overlay->print("\n");
        displayLocationInfo(*overlay, *(sel.location()), v.norm(), measurementSystem);
        break;

    default:
        break;
    }

    // Display RA/Dec for the selection, but only when the observer is near
    // the Earth.
    Selection refObject = sim->getFrame()->getRefObject();
    if (refObject.body() && refObject.body()->getName() == "Earth")
    {
        Body* earth = refObject.body();

        UniversalCoord observerPos = sim->getObserver().getPosition();
        double distToEarthCenter = observerPos.offsetFromKm(refObject.getPosition(sim->getTime())).norm();
        double altitude = distToEarthCenter - earth->getRadius();
        if (altitude < 1000.0)
        {
            // Code to show the geocentric RA/Dec

            // Only show the coordinates for stars and deep sky objects, where
            // the geocentric values will match the apparent values for observers
            // near the Earth.
            if (sel.star() != nullptr || sel.deepsky() != nullptr)
            {
                Eigen::Vector3d v = sel.getPosition(sim->getTime()).offsetFromKm(Selection(earth).getPosition(sim->getTime()));
                v = celmath::XRotation(astro::J2000Obliquity) * v;
                displayRADec(*overlay, v);
            }
        }
    }

    overlay->endText();
    overlay->restorePos();
}

void
Hud::renderTextInput(const WindowMetrics& metrics)
{
    overlay->setFont(titleFont);
    overlay->savePos();
    int rectHeight = fontHeight * 3.0f + metrics.screenDpi / 25.4f * 9.3f + titleFontHeight;
    celestia::Rect r(0, 0, metrics.width, metrics.insetBottom + rectHeight);
    r.setColor(consoleColor);
    overlay->drawRectangle(r);
    overlay->moveBy(metrics.getSafeAreaStart(), metrics.getSafeAreaBottom(rectHeight - titleFontHeight));
    overlay->setColor(0.6f, 0.6f, 1.0f, 1.0f);
    overlay->beginText();
    overlay->print(_("Target name: {}"), m_textInput.getTypedText());
    overlay->endText();
    overlay->setFont(font);
    if (auto typedTextCompletion = m_textInput.getCompletion(); !typedTextCompletion.empty())
    {
        int nb_cols = 4;
        int nb_lines = 3;
        int start = 0;
        overlay->moveBy(3, -font->getHeight() - 3);
        auto iter = typedTextCompletion.begin();
        auto typedTextCompletionIdx = m_textInput.getCompletionIndex();
        if (typedTextCompletionIdx >= nb_cols * nb_lines)
        {
            start = (typedTextCompletionIdx / nb_lines + 1 - nb_cols) * nb_lines;
            iter += start;
        }
        int columnWidth = metrics.getSafeAreaWidth() / nb_cols;
        for (int i = 0; iter < typedTextCompletion.end() && i < nb_cols; i++)
        {
            overlay->savePos();
            overlay->beginText();
            for (int j = 0; iter < typedTextCompletion.end() && j < nb_lines; iter++, j++)
            {
                if (i * nb_lines + j == typedTextCompletionIdx - start)
                    overlay->setColor(1.0f, 0.6f, 0.6f, 1);
                else
                    overlay->setColor(0.6f, 0.6f, 1.0f, 1);
                overlay->print(*iter);
                overlay->print("\n");
            }
            overlay->endText();
            overlay->restorePos();
            overlay->moveBy(metrics.layoutDirection == LayoutDirection::RightToLeft ? -columnWidth : columnWidth, 0);
        }
    }

    overlay->restorePos();
    overlay->setFont(font);
}

void
Hud::renderTextMessages(const WindowMetrics& metrics, double currentTime)
{
    std::string_view text = getCurrentMessage(currentTime);
    if (text.empty())
        return;

    int x = 0;
    int y = 0;

    messageTextPosition->resolvePixelPosition(metrics, x, y);

    overlay->setFont(titleFont);
    overlay->savePos();

    float alpha = 1.0f;
    if (currentTime > messageStart + messageDuration - 0.5)
        alpha = static_cast<float>((messageStart + messageDuration - currentTime) / 0.5);
    overlay->setColor(textColor.red(), textColor.green(), textColor.blue(), alpha);
    overlay->moveBy(x, y);
    overlay->beginText();
    overlay->print(messageText);
    overlay->endText();
    overlay->restorePos();
    overlay->setFont(font);
}

void
Hud::renderMovieCapture(const WindowMetrics& metrics, const MovieCapture& movieCapture)
{
    int movieWidth = movieCapture.getWidth();
    int movieHeight = movieCapture.getHeight();
    overlay->savePos();
    Color color(1.0f, 0.0f, 0.0f, 1.0f);
    overlay->setColor(color);
    celestia::Rect r((metrics.width - movieWidth) / 2 - 1,
                     (metrics.height - movieHeight) / 2 - 1,
                     movieWidth + 1,
                     movieHeight + 1);
    r.setColor(color);
    r.setType(celestia::Rect::Type::BorderOnly);
    overlay->drawRectangle(r);
    overlay->moveBy((float) ((metrics.width - movieWidth) / 2),
                    (float) ((metrics.height + movieHeight) / 2 + 2));
    overlay->beginText();
    overlay->printf(_("%dx%d at %.2f fps  %s"),
                    movieWidth, movieHeight,
                    movieCapture.getFrameRate(),
                    movieCapture.recordingStatus() ? _("Recording") : _("Paused"));

    overlay->endText();
    overlay->restorePos();

    overlay->savePos();
    overlay->moveBy((float) ((metrics.width + movieWidth) / 2 - emWidth * 5),
                    (float) ((metrics.height + movieHeight) / 2 + 2));
    float sec = movieCapture.getFrameCount() /
        movieCapture.getFrameRate();
    auto min = (int) (sec / 60);
    sec -= min * 60.0f;
    overlay->beginText();
    overlay->print("{:3d}:{:05.2f}", min, sec);
    overlay->endText();
    overlay->restorePos();

    overlay->savePos();
    overlay->moveBy((float) ((metrics.width - movieWidth) / 2),
                    (float) ((metrics.height - movieHeight) / 2 - fontHeight - 2));
    overlay->beginText();
    overlay->print(_("F11 Start/Pause    F12 Stop"));
    overlay->endText();
    overlay->restorePos();

    overlay->restorePos();
}

std::string_view
Hud::getCurrentMessage(double currentTime) const
{
    if (currentTime < messageStart + messageDuration && messageTextPosition)
        return messageText;
    return {};
}

void
Hud::setImage(std::unique_ptr<OverlayImage>&& _image, double currentTime)
{
    image = std::move(_image);
    image->setStartTime(static_cast<float>(currentTime));
}

bool
Hud::setFont(const std::shared_ptr<TextureFont>& f)
{
    if (f == nullptr)
        return false;

    font = f;
    fontHeight = font->getHeight();
    emWidth = engine::TextLayout::getTextWidth("M", font.get());
    assert(emWidth > 0);
    return true;
}

bool
Hud::setTitleFont(const std::shared_ptr<TextureFont>& f)
{
    if (f == nullptr)
        return false;

    titleFont = f;
    titleFontHeight = titleFont->getHeight();
    titleEmWidth = engine::TextLayout::getTextWidth("M", titleFont.get());
    assert(titleEmWidth > 0);
    return true;
}

void
Hud::clearFonts()
{
    if (overlay)
        overlay->setFont(nullptr);

    dateStrWidth = 0;
    titleFont = nullptr;
    font = nullptr;

}

} // end namespace celestia
