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
#if FMT_VERSION < 90000
#include <fmt/format.h>
#endif

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
#include <celephem/rotation.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celutil/flag.h>
#include <celutil/formatnum.h>
#include <celutil/gettext.h>
#include <celutil/includeicu.h>
#include <celutil/logger.h>
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
constexpr double OneLbPerFt3InKgPerM3 = OneLbInKg / math::cube(OneFtInKm * 1000.0);

constexpr util::NumberFormat SigDigitNum = util::NumberFormat::GroupThousands | util::NumberFormat::SignificantFigures;

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

std::string
KelvinToStr(const util::NumberFormatter& formatter, float value, int digits, TemperatureScale temperatureScale)
{
    switch (temperatureScale)
    {
        case TemperatureScale::Celsius:
            return fmt::format("{} °C", formatter.format(KelvinToCelsius(value), digits, SigDigitNum));
        case TemperatureScale::Fahrenheit:
            return fmt::format("{} °F", formatter.format(KelvinToFahrenheit(value), digits, SigDigitNum));
        default: // TemperatureScale::Kelvin
            return fmt::format("{} K", formatter.format(value, digits, SigDigitNum));
    }
}

std::string
DistanceLyToStr(const util::NumberFormatter& formatter, double distance, int digits, MeasurementSystem measurement)
{
    const char* units;
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
        if (std::abs(distance) > astro::kilometersToLightYears(OneMiInKm))
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
    else if (std::abs(distance) > astro::kilometersToLightYears(1.0f))
    {
        units = _("km");
        distance = astro::lightYearsToKilometers(distance);
    }
    else
    {
        units = _("m");
        distance = astro::lightYearsToKilometers(distance) * 1000.0f;
    }

    return fmt::format("{} {}", formatter.format(distance, digits, SigDigitNum), units);
}

std::string
DistanceKmToStr(const util::NumberFormatter& formatter, double distance, int digits, MeasurementSystem measurement)
{
    return DistanceLyToStr(formatter, astro::kilometersToLightYears(distance), digits, measurement);
}

void
displayRotationPeriod(const util::NumberFormatter& formatter, Overlay& overlay, double days)
{
    double unitValue;
    const char *unitStr;

    if (days > 1.0)
    {
        unitValue = days;
        unitStr = _("days");
    }
    else if (days > 1.0 / 24.0)
    {
        unitValue = days * 24.0;
        unitStr = _("hours");
    }
    else if (days > 1.0 / (24.0 * 60.0))
    {
        unitValue = days * 24.0 * 60.0;
        unitStr = _("minutes");
    }
    else
    {
        unitValue = days * 24.0 * 60.0 * 60.0;
        unitStr = _("seconds");
    }

    overlay.print(_("Rotation period: {} {}\n"), formatter.format(unitValue, 3), unitStr);
}

void
displayMass(const util::NumberFormatter& formatter, Overlay& overlay, float mass, MeasurementSystem measurement)
{
    if (mass < 0.001f)
    {
        if (measurement == MeasurementSystem::Imperial)
            overlay.print(_("Mass: {} lb\n"),
                          formatter.format(mass * astro::EarthMass / static_cast<float>(OneLbInKg), 4, SigDigitNum));
        else
            overlay.print(_("Mass: {} kg\n"),
                          formatter.format(mass * astro::EarthMass, 4, SigDigitNum));
    }
    else if (mass > 50.0f)
        overlay.print(_("Mass: {} Mj\n"),
                      formatter.format(mass * astro::EarthMass / astro::JupiterMass, 4, SigDigitNum));
    else
        overlay.print(_("Mass: {} Me\n"),
                      formatter.format(mass, 4, SigDigitNum));
}

void
displaySpeed(const util::NumberFormatter& formatter, Overlay& overlay, float speed, MeasurementSystem measurement)
{
    float unitValue;
    const char *unitStr;

    if (speed >= (float) 1000.0_au)
    {
        unitValue = astro::kilometersToLightYears(speed);
        unitStr = _("ly/s");
    }
    else if (speed >= (float) 100.0_c)
    {
        unitValue = astro::kilometersToAU(speed);
        unitStr = _("AU/s");
    }
    else if (speed >= 10000.0f)
    {
        unitValue = static_cast<float>(speed / astro::speedOfLight);
        unitStr = "c";
    }
    else if (measurement == MeasurementSystem::Imperial)
    {
        if (speed >= (float) OneMiInKm)
        {
            unitValue = static_cast<float>(speed / OneMiInKm);
            unitStr = _("mi/s");
        }
        else
        {
            unitValue = static_cast<float>(speed / OneFtInKm);
            unitStr = _("ft/s");
        }
    }
    else
    {
        if (speed >= 1.0f)
        {
            unitValue = speed;
            unitStr = _("km/s");
        }
        else
        {
            unitValue = speed * 1000.0f;
            unitStr = _("m/s");
        }
    }
    overlay.print(_("Speed: {} {}\n"), formatter.format(unitValue, 3, SigDigitNum), unitStr);
}

// Display a positive angle as degrees, minutes, and seconds. If the angle is less than one
// degree, only minutes and seconds are shown; if the angle is less than one minute, only
// seconds are displayed.
std::string
angleToStr(double angle, const std::locale& loc)
{
    int degrees;
    int minutes;
    double seconds;
    astro::decimalToDegMinSec(angle, degrees, minutes, seconds);

    if (degrees > 0)
    {
        return fmt::format(loc, "{}° {:02d}′ {:.1f}″",
                           degrees, std::abs(minutes), std::abs(seconds));
    }

    if (minutes > 0)
    {
        return fmt::format(loc, "{:02d}′ {:.1f}″", std::abs(minutes), std::abs(seconds));
    }

    return fmt::format(loc, "{:.2f}″", std::abs(seconds));
}

void
displayApparentDiameter(Overlay& overlay, double radius, double distance, const std::locale& loc)
{
    if (distance < radius)
        return;

    double arcSize = math::radToDeg(std::asin(radius / distance) * 2.0);

    // Only display the arc size if it's less than 160 degrees and greater
    // than one second--otherwise, it's probably not interesting data.
    if (arcSize < 160.0 && arcSize > 1.0 / 3600.0)
        overlay.printf(_("Apparent diameter: %s\n"), angleToStr(arcSize, loc));
}

void
displayDeclination(Overlay& overlay, double angle, const std::locale& loc)
{
    int degrees;
    int minutes;
    double seconds;
    astro::decimalToDegMinSec(angle, degrees, minutes, seconds);

    overlay.print(loc, _("Dec: {:+d}° {:02d}′ {:.1f}″\n"),
                  std::abs(degrees), std::abs(minutes), std::abs(seconds));
}

void
displayRightAscension(Overlay& overlay, double angle, const std::locale& loc)
{
    int hours;
    int minutes;
    double seconds;
    astro::decimalToHourMinSec(angle, hours, minutes, seconds);

    overlay.print(loc, _("RA: {}h {:02}m {:.1f}s\n"),
                  hours, std::abs(minutes), std::abs(seconds));
}

void
displayApparentMagnitude(Overlay& overlay,
                         float absMag,
                         double distance,
                         const std::locale& loc)
{
    if (distance > 32.6167)
    {
        float appMag = astro::absToAppMag(absMag, static_cast<float>(distance));
        overlay.print(loc, _("Apparent magnitude: {:.1f}\n"), appMag);
    }
    else
    {
        overlay.print(loc, _("Absolute magnitude: {:.1f}\n"), absMag);
    }
}

void
displayRADec(Overlay& overlay, const Eigen::Vector3d& v, const std::locale& loc)
{
    double phi = std::atan2(v.x(), v.z()) - celestia::numbers::pi / 2;
    if (phi < 0.0)
        phi = phi + 2.0 * celestia::numbers::pi;

    double theta = std::atan2(std::hypot(v.x(), v.z()), v.y());
    if (theta > 0)
        theta = celestia::numbers::pi * 0.5 - theta;
    else
        theta = -celestia::numbers::pi * 0.5 - theta;


    displayRightAscension(overlay, math::radToDeg(phi), loc);
    displayDeclination(overlay, math::radToDeg(theta), loc);
}

// Display nicely formatted planetocentric/planetographic coordinates.
// The latitude and longitude parameters are angles in radians, altitude
// is in kilometers.
void
displayPlanetocentricCoords(const util::NumberFormatter& formatter,
                            Overlay& overlay,
                            const Body& body,
                            double longitude,
                            double latitude,
                            double altitude,
                            MeasurementSystem measurement,
                            const std::locale& loc)
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

        lon = static_cast<float>(std::abs(math::radToDeg(longitude)));
        lat = static_cast<float>(std::abs(math::radToDeg(latitude)));
    }
    else
    {
        // Swap hemispheres if the object is a retrograde rotator
        Eigen::Quaterniond q = body.getEclipticToEquatorial(astro::J2000);
        bool retrograde = (q * Eigen::Vector3d::UnitY()).y() < 0.0;

        if ((latitude < 0.0) != retrograde)
            nsHemi = 'S';
        else if ((latitude > 0.0) != retrograde)
            nsHemi = 'N';

        if (retrograde)
            ewHemi = 'E';
        else
            ewHemi = 'W';

        lon = -math::radToDeg(longitude);
        if (lon < 0.0)
            lon += 360.0;
        lat = std::abs(math::radToDeg(latitude));
    }

    overlay.print(loc, _("{:.6f}{} {:.6f}{} {}"),
                  lat, nsHemi, lon, ewHemi,
                  DistanceKmToStr(formatter, altitude, 5, measurement));
}

void
displayStarInfo(const util::NumberFormatter& formatter,
                Overlay& overlay,
                int detail,
                const Star& star,
                const Universe& universe,
                double distance,
                const HudSettings& hudSettings,
                const std::locale& loc)
{
    overlay.printf(_("Distance: %s\n"),
                   DistanceLyToStr(formatter, distance, 5, hudSettings.measurementSystem));

    if (!star.getVisibility())
    {
        overlay.print(_("Star system barycenter\n"));
    }
    else
    {
        overlay.print(loc, _("Abs (app) mag: {:.2f} ({:.2f})\n"),
                      star.getAbsoluteMagnitude(),
                      star.getApparentMagnitude(float(distance)));

        if (star.getLuminosity() > 1.0e-10f)
            overlay.print(loc, _("Luminosity: {}x Sun\n"), formatter.format(star.getLuminosity(), 3, SigDigitNum));

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
        }
        overlay.printf(_("Class: %s\n"), star_class);

        displayApparentDiameter(overlay, star.getRadius(),
                                astro::lightYearsToKilometers(distance), loc);

        if (detail > 1)
        {
            overlay.printf(_("Surface temp: %s\n"),
                           KelvinToStr(formatter, star.getTemperature(), 3, hudSettings.temperatureScale));

            if (float solarRadii = star.getRadius() / 6.96e5f; solarRadii > 0.01f)
            {
                overlay.print(_("Radius: {} Rsun ({})\n"),
                              formatter.format(star.getRadius() / 696000.0f, 2, SigDigitNum),
                              DistanceKmToStr(formatter, star.getRadius(), 3, hudSettings.measurementSystem));
            }
            else
            {
                overlay.print(_("Radius: {}\n"),
                              DistanceKmToStr(formatter, star.getRadius(), 3, hudSettings.measurementSystem));
            }

            if (star.getRotationModel()->isPeriodic())
            {
                auto period = static_cast<float>(star.getRotationModel()->getPeriod());
                displayRotationPeriod(formatter, overlay, period);
            }
        }
    }

    if (detail > 1)
    {
        const SolarSystem* sys = universe.getSolarSystem(&star);
        if (sys != nullptr && sys->getPlanets()->getSystemSize() != 0)
            overlay.print(_("Planetary companions present\n"));
    }
}

void displayDSOinfo(const util::NumberFormatter& formatter,
                    Overlay& overlay,
                    const DeepSkyObject& dso,
                    double distance,
                    MeasurementSystem measurement,
                    const std::locale& loc)
{
    overlay.print(dso.getDescription());
    overlay.print("\n");

    if (distance >= 0.0)
    {
        overlay.printf(_("Distance: %s\n"),
                     DistanceLyToStr(formatter, distance, 5, measurement));
    }
    else
    {
        overlay.printf(_("Distance from center: %s\n"),
                     DistanceLyToStr(formatter, distance + dso.getRadius(), 5, measurement));
     }
    overlay.printf(_("Radius: %s\n"),
                 DistanceLyToStr(formatter, dso.getRadius(), 5, measurement));

    displayApparentDiameter(overlay, dso.getRadius(), distance, loc);
    if (dso.getAbsoluteMagnitude() > DSO_DEFAULT_ABS_MAGNITUDE)
    {
        displayApparentMagnitude(overlay,
                                 dso.getAbsoluteMagnitude(),
                                 distance,
                                 loc);
    }
}

void
displayPlanetInfo(const util::NumberFormatter& formatter,
                  Overlay& overlay,
                  int detail,
                  const Body& body,
                  double t,
                  const Eigen::Vector3d& viewVec,
                  const HudSettings& hudSettings,
                  const std::locale& loc)
{
    double distanceKm = viewVec.norm();
    double distance = distanceKm - body.getRadius();
    overlay.printf(_("Distance: %s\n"),
                   DistanceKmToStr(formatter, distance, 5, hudSettings.measurementSystem));

    if (body.getClassification() == BodyClassification::Invisible)
    {
        return;
    }
    else if (body.isEllipsoid()) // show mean radius along with triaxial semi-axes
    {
        Eigen::Vector3f semiAxes = body.getSemiAxes();
        if (semiAxes.x() == semiAxes.z())
        {
            if (semiAxes.x() == semiAxes.y())
            {
                overlay.print(_("Radius: {}\n"), DistanceKmToStr(formatter, body.getRadius(), 5, hudSettings.measurementSystem));
            }
            else
            {
                overlay.print(_("Equatorial radius: {}\n"), DistanceKmToStr(formatter, semiAxes.x(), 5, hudSettings.measurementSystem));
                overlay.print(_("Polar radius: {}\n"), DistanceKmToStr(formatter, semiAxes.y(), 5, hudSettings.measurementSystem));
            }
        }
        else
        {
            overlay.print(_("Radii: {} × {} × {}\n"),
                          DistanceKmToStr(formatter, semiAxes.x(), 5, hudSettings.measurementSystem),
                          DistanceKmToStr(formatter, semiAxes.z(), 5, hudSettings.measurementSystem),
                          DistanceKmToStr(formatter, semiAxes.y(), 5, hudSettings.measurementSystem));
        }
    }
    else
    {
        overlay.print(_("Radius: {}\n"), DistanceKmToStr(formatter, body.getRadius(), 5, hudSettings.measurementSystem));
    }

    displayApparentDiameter(overlay, body.getRadius(), distanceKm, loc);

    // Display the phase angle

    // Find the parent star of the body. This can be slightly complicated if
    // the body orbits a barycenter instead of a star.
    const Star* sun = nullptr;
    for (const PlanetarySystem* system = body.getSystem(); system != nullptr;)
    {
        const Body* primaryBody = system->getPrimaryBody();
        if (primaryBody == nullptr)
        {
            sun = system->getStar();
            break;
        }

        system = primaryBody->getSystem();
    }

    if (sun != nullptr)
    {
        bool showPhaseAngle = false;
        if (sun->getVisibility())
        {
            showPhaseAngle = true;
        }
        else if (auto orbitingStars = sun->getOrbitingStars(); orbitingStars.size() == 1)
        {
            // The planet's orbit is defined with respect to a barycenter. If there's
            // a single star orbiting the barycenter, we'll compute the phase angle
            // for the planet with respect to that star. If there are no stars, the
            // planet is an orphan, drifting through space with no star. We also skip
            // displaying the phase angle when there are multiple stars (for now.)
            sun = orbitingStars.front();
            showPhaseAngle = sun->getVisibility();
        }

        if (showPhaseAngle)
        {
            Eigen::Vector3d sunVec = body.getPosition(t).offsetFromKm(sun->getPosition(t));
            sunVec.normalize();
            double cosPhaseAngle = std::clamp(sunVec.dot(viewVec.normalized()), -1.0, 1.0);
            double phaseAngle = acos(cosPhaseAngle);
            overlay.print(loc, _("Phase angle: {:.1f}°\n"), math::radToDeg(phaseAngle));
        }
    }

    if (detail > 1)
    {
        if (body.getRotationModel(t)->isPeriodic())
            displayRotationPeriod(formatter, overlay, body.getRotationModel(t)->getPeriod());

        if (body.getMass() > 0)
            displayMass(formatter, overlay, body.getMass(), hudSettings.measurementSystem);

        if (float density = body.getDensity(); density > 0)
        {
            if (hudSettings.measurementSystem == MeasurementSystem::Imperial)
            {
                overlay.print(_("Density: {} lb/ft³\n"),
                              formatter.format(density / static_cast<float>(OneLbPerFt3InKgPerM3), 4, SigDigitNum));
            }
            else
            {
                overlay.print(_("Density: {} kg/m³\n"), formatter.format(density, 4, SigDigitNum));
            }
        }

        float planetTemp = body.getTemperature(t);
        if (planetTemp > 0)
            overlay.printf(_("Temperature: %s\n"), KelvinToStr(formatter, planetTemp, 3, hudSettings.temperatureScale));
    }
}

void
displayLocationInfo(const util::NumberFormatter& formatter,
                    Overlay& overlay,
                    const Location& location,
                    double distanceKm,
                    MeasurementSystem measurement,
                    const std::locale& loc)
{
    overlay.printf(_("Distance: %s\n"), DistanceKmToStr(formatter, distanceKm, 5, measurement));

    const Body* body = location.getParentBody();
    if (body == nullptr)
        return;

    Eigen::Vector3f locPos = location.getPosition();
    Eigen::Vector3d lonLatAlt = body->cartesianToPlanetocentric(locPos.cast<double>());
    displayPlanetocentricCoords(formatter, overlay, *body,
                                lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z(), measurement, loc);
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

void
HudFonts::setFont(const std::shared_ptr<TextureFont>& f)
{
    m_font = f;
    m_fontHeight = f->getHeight();
    m_emWidth = engine::TextLayout::getTextWidth("M", f.get());
}

void
HudFonts::setTitleFont(const std::shared_ptr<TextureFont>& f)
{
    m_titleFont = f;
    m_titleFontHeight = f->getHeight();
    m_titleEmWidth = engine::TextLayout::getTextWidth("M", f.get());
}

Hud::Hud(const std::locale& loc) :
    loc(loc),
#ifdef USE_ICU
    m_dateFormatter(std::make_unique<celestia::engine::DateFormatter>()),
    m_numberFormatter(std::make_unique<util::NumberFormatter>())
#else
    m_dateFormatter(std::make_unique<celestia::engine::DateFormatter>(loc)),
    m_numberFormatter(std::make_unique<util::NumberFormatter>(loc))
#endif
{
}

Hud::~Hud() = default;

int
Hud::detail() const
{
    return m_hudDetail;
}

void
Hud::detail(int value)
{
    m_hudDetail = value % 3;
}

astro::Date::Format
Hud::dateFormat() const
{
    return m_dateFormat;
}

void
Hud::dateFormat(astro::Date::Format format)
{
    m_dateFormat = format;
    m_dateStrWidth = 0;
}

TextInput&
Hud::textInput()
{
    return m_textInput;
}

Hud::TextEnterMode
Hud::textEnterMode() const
{
    return m_textEnterMode;
}

void
Hud::textEnterMode(TextEnterMode value)
{
    m_textEnterMode = value;
    if (!util::is_set(m_textEnterMode, TextEnterMode::AutoComplete))
        m_textInput.reset();
}

void
Hud::setOverlay(std::unique_ptr<Overlay>&& _overlay)
{
    m_overlay = std::move(_overlay);
}

void
Hud::setWindowSize(int w, int h)
{
    if (m_overlay != nullptr)
        m_overlay->setWindowSize(w, h);
}

void
Hud::setTextAlignment(LayoutDirection dir)
{
    m_overlay->setTextAlignment(dir == LayoutDirection::RightToLeft
                                    ? engine::TextLayout::HorizontalAlignment::Right
                                    : engine::TextLayout::HorizontalAlignment::Left);
}

int
Hud::getTextWidth(std::string_view text) const
{
    return engine::TextLayout::getTextWidth(text, m_hudFonts.titleFont().get());
}

const std::shared_ptr<TextureFont>&
Hud::font() const
{
    return m_hudFonts.font();
}

void
Hud::font(const std::shared_ptr<TextureFont>& f)
{
    m_hudFonts.setFont(f);
    m_dateStrWidth = 0;
}

const std::shared_ptr<TextureFont>&
Hud::titleFont() const
{
    return m_hudFonts.titleFont();
}

void
Hud::titleFont(const std::shared_ptr<TextureFont>& f)
{
    m_hudFonts.setTitleFont(f);
}

std::tuple<int, int>
Hud::titleMetrics() const
{
    return std::make_tuple(m_hudFonts.titleEmWidth(), m_hudFonts.titleFontHeight());
}

#ifdef USE_ICU
MeasurementSystem
defaultMeasurementSystem()
{
    UErrorCode status = U_ZERO_ERROR;
    auto icuSystem = ulocdata_getMeasurementSystem(uloc_getDefault(), &status);
    if (U_SUCCESS(status))
    {
        switch (icuSystem)
        {
        case UMS_SI:
            return MeasurementSystem::Metric;
        case UMS_US:
            return MeasurementSystem::Imperial;
        case UMS_UK:
            return MeasurementSystem::Imperial;
        default:
            util::GetLogger()->error(_("Unknown measurement system {}, fallback to Metric system"), static_cast<int>(icuSystem));
            return MeasurementSystem::Metric;
        }
    }
    else
    {
        util::GetLogger()->error(_("Failed to get default measurement system {}, fallback to Metric system"), static_cast<int>(status));
        return MeasurementSystem::Metric;
    }
}
#endif

void
Hud::renderOverlay(const WindowMetrics& metrics,
                   const Simulation* sim,
                   const ViewManager& views,
                   const MovieCapture* movieCapture,
                   const TimeInfo& timeInfo,
                   bool isScriptRunning,
                   bool editMode)
{
#ifdef USE_ICU
    if (m_hudSettings.measurementSystem == MeasurementSystem::System)
        m_hudSettings.measurementSystem = defaultMeasurementSystem();
#endif

    m_overlay->setFont(m_hudFonts.font());

    m_overlay->begin();

    if (m_hudSettings.showOverlayImage && isScriptRunning && m_image != nullptr)
        m_image->render(static_cast<float>(timeInfo.currentTime), metrics.width, metrics.height);

    views.renderBorders(m_overlay.get(), metrics, timeInfo.currentTime);

    if (m_hudDetail > 0 && util::is_set(m_hudSettings.overlayElements, HudElements::ShowTime))
        renderTimeInfo(metrics, sim, timeInfo);

    if (m_hudDetail > 0 && util::is_set(m_hudSettings.overlayElements, HudElements::ShowVelocity))
    {
        // Speed
        m_overlay->savePos();
        m_overlay->moveBy(metrics.getSafeAreaStart(),
                          metrics.getSafeAreaBottom(m_hudFonts.fontHeight() * 2 + static_cast<int>(static_cast<float>(metrics.screenDpi) / 25.4f * 1.3f)));
        m_overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);

        m_overlay->beginText();
        m_overlay->print("\n");
        if (m_hudSettings.showFPSCounter)
            m_overlay->print(loc, _("FPS: {:.1f}\n"), timeInfo.fps);
        else
            m_overlay->print("\n");

        displaySpeed(*m_numberFormatter, *m_overlay, static_cast<float>(sim->getObserver().getVelocity().norm()), m_hudSettings.measurementSystem);

        m_overlay->endText();
        m_overlay->restorePos();
    }

    if (m_hudDetail > 0 && util::is_set(m_hudSettings.overlayElements, HudElements::ShowFrame))
        renderFrameInfo(metrics, sim);

    if (Selection sel = sim->getSelection();
        !sel.empty() && m_hudDetail > 0 && util::is_set(m_hudSettings.overlayElements, HudElements::ShowSelection))
    {
        Eigen::Vector3d v = sel.getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
        renderSelectionInfo(metrics, sim, sel, v);
    }

    if (util::is_set(m_textEnterMode, TextEnterMode::AutoComplete))
        m_textInput.render(m_overlay.get(), m_hudFonts, metrics);

    if (m_hudSettings.showMessage)
        renderTextMessages(metrics, timeInfo.currentTime);

    if (movieCapture != nullptr)
        renderMovieCapture(metrics, *movieCapture);

    if (editMode)
    {
        m_overlay->savePos();
        m_overlay->beginText();
        int x = (metrics.getSafeAreaWidth() - engine::TextLayout::getTextWidth(_("Edit Mode"), m_hudFonts.font().get())) / 2;
        m_overlay->moveBy(metrics.getSafeAreaStart(x), metrics.getSafeAreaTop(m_hudFonts.fontHeight()));
        m_overlay->setColor(1, 0, 1, 1);
        m_overlay->print(_("Edit Mode"));
        m_overlay->endText();
        m_overlay->restorePos();
    }

    m_overlay->end();
}

void
Hud::renderTimeInfo(const WindowMetrics& metrics, const Simulation* sim, const TimeInfo& timeInfo)
{
    double lt = 0.0;

    if (sim->getSelection().getType() == SelectionType::Body &&
        sim->getTargetSpeed() < 0.99_c &&
        timeInfo.lightTravelFlag)
    {
        Eigen::Vector3d v = sim->getSelection().getPosition(sim->getTime()).offsetFromKm(sim->getObserver().getPosition());
        // light travel time in days
        lt = v.norm() / static_cast<double>(86400.0_c);
    }

    double tdb = sim->getTime() + lt;
    auto dateStr = m_dateFormatter->formatDate(tdb, timeInfo.timeZoneBias != 0, m_dateFormat);
    auto fullDateStr = timeInfo.lightTravelFlag ? dateStr + _("  LT") : dateStr;

    m_dateStrWidth = std::max(m_dateStrWidth, engine::TextLayout::getTextWidth(fullDateStr, m_hudFonts.font().get()) + 2 * m_hudFonts.emWidth());

    // Time and date
    m_overlay->savePos();
    m_overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);
    m_overlay->moveBy(metrics.getSafeAreaEnd(m_dateStrWidth), metrics.getSafeAreaTop(m_hudFonts.fontHeight()));
    m_overlay->beginText();

    m_overlay->print(dateStr);

    if (timeInfo.lightTravelFlag && lt > 0.0)
    {
        m_overlay->setColor(0.42f, 1.0f, 1.0f, 1.0f);
        m_overlay->print(_("  LT"));
        m_overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);
    }
    m_overlay->print("\n");

    if (std::abs(std::abs(sim->getTimeScale()) - 1.0) < 1e-6)
    {
        if (math::sign(sim->getTimeScale()) == 1)
            m_overlay->print(_("Real time"));
        else
            m_overlay->print(_("-Real time"));
    }
    else if (std::abs(sim->getTimeScale()) < TimeInfo::MinimumTimeRate)
    {
        m_overlay->print(_("Time stopped"));
    }
    else if (std::abs(sim->getTimeScale()) > 1.0)
    {
        m_overlay->print(loc, _("{:.6g} x faster"), sim->getTimeScale()); // XXX: %'.12g
    }
    else
    {
        m_overlay->print(loc, _("{:.6g} x slower"), 1.0 / sim->getTimeScale()); // XXX: %'.12g
    }

    if (sim->getPauseState() == true)
    {
        m_overlay->setColor(1.0f, 0.0f, 0.0f, 1.0f);
        m_overlay->print(_(" (Paused)"));
    }

    m_overlay->endText();
    m_overlay->restorePos();
}

void
Hud::renderFrameInfo(const WindowMetrics& metrics, const Simulation* sim)
{
    // Field of view and camera mode in lower right corner
    m_overlay->savePos();
    m_overlay->moveBy(metrics.getSafeAreaEnd(m_hudFonts.emWidth() * 15),
                      metrics.getSafeAreaBottom(m_hudFonts.fontHeight() * 3 +
                          static_cast<int>(static_cast<float>(metrics.screenDpi) / 25.4f * 1.3f)));
    m_overlay->beginText();
    m_overlay->setColor(0.6f, 0.6f, 1.0f, 1);

    if (sim->getObserverMode() == Observer::ObserverMode::Travelling)
    {
        double timeLeft = sim->getArrivalTime() - sim->getRealTime();
        if (timeLeft >= 1)
            m_overlay->print(_("Travelling ({})\n"), m_numberFormatter->format(timeLeft, 0));
        else
            m_overlay->print(_("Travelling\n"));
    }
    else
    {
        m_overlay->print("\n");
    }

    const Universe& u = *sim->getUniverse();

    if (!sim->getTrackedObject().empty())
        m_overlay->printf(_("Track %s\n"), CX_("Track", getSelectionName(sim->getTrackedObject(), u)));
    else
        m_overlay->print("\n");

    Selection refObject = sim->getFrame()->getRefObject();
    switch (sim->getFrame()->getCoordinateSystem())
    {
    case ObserverFrame::CoordinateSystem::Ecliptical:
        m_overlay->printf(_("Follow %s\n"),
                          CX_("Follow", getSelectionName(refObject, u)));
        break;
    case ObserverFrame::CoordinateSystem::BodyFixed:
        m_overlay->printf(_("Sync Orbit %s\n"),
                          CX_("Sync", getSelectionName(refObject, u)));
        break;
    case ObserverFrame::CoordinateSystem::PhaseLock:
        m_overlay->printf(_("Lock %s -> %s\n"),
                          CX_("Lock", getSelectionName(refObject, u)),
                          CX_("LockTo", getSelectionName(sim->getFrame()->getTargetObject(), u)));
        break;

    case ObserverFrame::CoordinateSystem::Chase:
        m_overlay->printf(_("Chase %s\n"),
                          CX_("Chase", getSelectionName(refObject, u)));
        break;

    default:
        m_overlay->print("\n");
        break;
    }

    m_overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);

    // Field of view
    const Observer* activeObserver = sim->getActiveObserver();
    float fov = math::radToDeg(activeObserver->getFOV());
    m_overlay->print(loc, _("FOV: {} ({:.2f}x)\n"), angleToStr(fov, loc), activeObserver->getZoom());
    m_overlay->endText();
    m_overlay->restorePos();
}

void
Hud::renderSelectionInfo(const WindowMetrics& metrics,
                         const Simulation* sim,
                         Selection sel,
                         const Eigen::Vector3d& v)
{
    m_overlay->savePos();
    m_overlay->setColor(0.7f, 0.7f, 1.0f, 1.0f);
    m_overlay->moveBy(metrics.getSafeAreaStart(), metrics.getSafeAreaTop(m_hudFonts.titleFontHeight()));

    m_overlay->beginText();

    switch (sel.getType())
    {
    case SelectionType::Star:
        {
            if (sel != m_lastSelection)
            {
                m_lastSelection = sel;
                m_selectionNames = sim->getUniverse()->getStarCatalog()->getStarNameList(*sel.star());
            }

            m_overlay->setFont(m_hudFonts.titleFont());
            m_overlay->print(m_selectionNames);
            m_overlay->setFont(m_hudFonts.font());
            m_overlay->print("\n");
            displayStarInfo(*m_numberFormatter,
                            *m_overlay,
                             m_hudDetail,
                            *(sel.star()),
                            *(sim->getUniverse()),
                             astro::kilometersToLightYears(v.norm()),
                             m_hudSettings,
                             loc);
        }
        break;

    case SelectionType::DeepSky:
        {
            if (sel != m_lastSelection)
            {
                m_lastSelection = sel;
                m_selectionNames = sim->getUniverse()->getDSOCatalog()->getDSONameList(sel.deepsky());
            }

            m_overlay->setFont(m_hudFonts.titleFont());
            m_overlay->print(m_selectionNames);
            m_overlay->setFont(m_hudFonts.font());
            m_overlay->print("\n");
            displayDSOinfo(*m_numberFormatter,
                           *m_overlay,
                           *sel.deepsky(),
                            astro::kilometersToLightYears(v.norm()) - sel.deepsky()->getRadius(),
                            m_hudSettings.measurementSystem,
                            loc);
        }
        break;

    case SelectionType::Body:
        {
            // Show all names for the body
            if (sel != m_lastSelection)
            {
                m_lastSelection = sel;
                m_selectionNames = getBodySelectionNames(*sel.body());
            }

            m_overlay->setFont(m_hudFonts.titleFont());
            m_overlay->print(m_selectionNames);
            m_overlay->setFont(m_hudFonts.font());
            m_overlay->print("\n");
            displayPlanetInfo(*m_numberFormatter,
                              *m_overlay,
                               m_hudDetail,
                              *(sel.body()),
                               sim->getTime(),
                               v,
                               m_hudSettings,
                               loc);
        }
        break;

    case SelectionType::Location:
        m_overlay->setFont(m_hudFonts.titleFont());
        m_overlay->print(sel.location()->getName(true).c_str());
        m_overlay->setFont(m_hudFonts.font());
        m_overlay->print("\n");
        displayLocationInfo(*m_numberFormatter,
                            *m_overlay,
                            *(sel.location()),
                             v.norm(),
                             m_hudSettings.measurementSystem,
                             loc);
        break;

    default:
        break;
    }

    // Display RA/Dec for the selection, but only when the observer is near
    // the Earth.
    if (const Body* refObject = sim->getFrame()->getRefObject().body();
        refObject != nullptr && refObject->getName() == "Earth")
    {
        UniversalCoord observerPos = sim->getObserver().getPosition();
        double distToEarthCenter = observerPos.offsetFromKm(refObject->getPosition(sim->getTime())).norm();
        double altitude = distToEarthCenter - refObject->getRadius();
        if (altitude < 1000.0 && (sel.getType() == SelectionType::Star || sel.getType() == SelectionType::DeepSky))
        {
            // Code to show the geocentric RA/Dec

            // Only show the coordinates for stars and deep sky objects, where
            // the geocentric values will match the apparent values for observers
            // near the Earth.
            Eigen::Vector3d vEarth = sel.getPosition(sim->getTime()).offsetFromKm(refObject->getPosition(sim->getTime()));
            vEarth = math::XRotation(astro::J2000Obliquity) * vEarth;
            displayRADec(*m_overlay, vEarth, loc);
        }
    }

    m_overlay->endText();
    m_overlay->restorePos();
}

void
Hud::renderTextMessages(const WindowMetrics& metrics, double currentTime)
{
    if (currentTime >= m_messageStart + m_messageDuration)
        return;

    int x = 0;
    int y = 0;

    m_messageTextPosition.resolvePixelPosition(metrics, x, y);

    m_overlay->setFont(m_hudFonts.titleFont());
    m_overlay->savePos();

    float alpha = 1.0f;
    if (currentTime > m_messageStart + m_messageDuration - 0.5)
        alpha = static_cast<float>((m_messageStart + m_messageDuration - currentTime) / 0.5);
    m_overlay->setColor(m_hudSettings.textColor, alpha);
    m_overlay->moveBy(x, y);
    m_overlay->beginText();
    m_overlay->print(m_messageText);
    m_overlay->endText();
    m_overlay->restorePos();
    m_overlay->setFont(m_hudFonts.font());
}

void
Hud::renderMovieCapture(const WindowMetrics& metrics, const MovieCapture& movieCapture)
{
    int movieWidth = movieCapture.getWidth();
    int movieHeight = movieCapture.getHeight();
    m_overlay->savePos();
    Color color(1.0f, 0.0f, 0.0f, 1.0f);
    m_overlay->setColor(color);
    celestia::Rect r(static_cast<float>((metrics.width - movieWidth) / 2 - 1),
                     static_cast<float>((metrics.height - movieHeight) / 2 - 1),
                     static_cast<float>(movieWidth + 1),
                     static_cast<float>(movieHeight + 1));
    r.setColor(color);
    r.setType(celestia::Rect::Type::BorderOnly);
    m_overlay->drawRectangle(r);
    m_overlay->moveBy(static_cast<float>((metrics.width - movieWidth) / 2),
                      static_cast<float>((metrics.height + movieHeight) / 2 + 2));
    m_overlay->beginText();
    m_overlay->print(loc, _("{}x{} at {:.2f} fps  {}"),
                     movieWidth, movieHeight,
                     movieCapture.getFrameRate(),
                     movieCapture.recordingStatus() ? _("Recording") : _("Paused"));

    m_overlay->endText();
    m_overlay->restorePos();

    m_overlay->savePos();
    m_overlay->moveBy(static_cast<float>((metrics.width + movieWidth) / 2 - m_hudFonts.emWidth() * 5),
                      static_cast<float>((metrics.height + movieHeight) / 2 + 2));
    float sec = static_cast<float>(movieCapture.getFrameCount()) / movieCapture.getFrameRate();
    auto min = static_cast<int>(sec / 60.0f);
    sec -= static_cast<float>(min) * 60.0f;
    m_overlay->beginText();
    m_overlay->print(loc, "{:3d}:{:05.2f}", min, sec);
    m_overlay->endText();
    m_overlay->restorePos();

    m_overlay->savePos();
    m_overlay->moveBy(static_cast<float>((metrics.width - movieWidth) / 2),
                      static_cast<float>((metrics.height - movieHeight) / 2 - m_hudFonts.fontHeight() - 2));
    m_overlay->beginText();
    m_overlay->print(_("F11 Start/Pause    F12 Stop"));
    m_overlay->endText();
    m_overlay->restorePos();

    m_overlay->restorePos();
}

void
Hud::showText(const TextPrintPosition& position,
              std::string_view message,
              double duration,
              double currentTime)
{
    if (!m_hudFonts.titleFont())
        return;

    m_messageText.clear();
    m_messageText.append(message);
    m_messageTextPosition = position;
    m_messageStart = currentTime;
    m_messageDuration = duration;
}

void
Hud::setImage(std::unique_ptr<OverlayImage>&& _image, double currentTime)
{
    m_image = std::move(_image);
    m_image->setStartTime(static_cast<float>(currentTime));
}

} // end namespace celestia
