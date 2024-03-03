#include <fmt/printf.h>
#include <celengine/render.h>
#include <celengine/selection.h>
#include <celutil/flag.h>
#include <celutil/gettext.h>
#include "helper.h"

namespace util = celestia::util;

constexpr auto SpacecraftPrimaryBody = BodyClassification::Planet |
                                       BodyClassification::DwarfPlanet |
                                       BodyClassification::Moon |
                                       BodyClassification::MinorMoon |
                                       BodyClassification::Asteroid |
                                       BodyClassification::Comet;

bool
Helper::hasPrimaryStar(const Body* body)
{
    return body->getSystem() != nullptr && body->getSystem()->getStar() != nullptr;
}

bool
Helper::hasPrimaryBody(const Body* body, BodyClassification classification)
{
    if (const PlanetarySystem* system = body->getSystem(); system != nullptr)
    {
        if (auto primaryBody = system->getPrimaryBody(); primaryBody != nullptr)
            return util::is_set(primaryBody->getClassification(), classification);
    }

    return false;
}

bool
Helper::hasBarycenter(const Body* body)
{
    const PlanetarySystem *system = body->getSystem();
    if (system == nullptr)
        return false;

    if (const Body* primaryBody = system->getPrimaryBody();
        primaryBody == nullptr || !util::is_set(primaryBody->getClassification(), BodyClassification::Invisible))
    {
        return false;
    }

    for (int bodyIdx = 0; bodyIdx < system->getSystemSize(); bodyIdx++)
    {
        if (util::is_set(system->getBody(bodyIdx)->getClassification(), BodyClassification::Planet | BodyClassification::DwarfPlanet))
            return true;
    }

    return false;
}

bool
Helper::hasPrimary(const Body* body)
{
    switch (body->getClassification())
    {
    case BodyClassification::Planet:
    case BodyClassification::DwarfPlanet:
    case BodyClassification::Asteroid:
    case BodyClassification::Comet:
        return hasPrimaryStar(body);
    case BodyClassification::Moon:
    case BodyClassification::MinorMoon:
        return hasPrimaryBody(body, BodyClassification::Planet | BodyClassification::DwarfPlanet) ||
            hasBarycenter(body);
    case BodyClassification::Spacecraft:
        return hasPrimaryBody(body, SpacecraftPrimaryBody) || hasPrimaryStar(body);
    default:
        break;
    }
    return false;
}

Selection
Helper::getPrimary(const Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == nullptr)
        return Selection();

    Body* primaryBody = system->getPrimaryBody();
    if (primaryBody != nullptr)
    {
        BodyClassification primaryClass = primaryBody->getClassification();
        if (util::is_set(primaryClass, SpacecraftPrimaryBody))
            return Selection(primaryBody);

        if (util::is_set(primaryClass, BodyClassification::Invisible) &&
            util::is_set(body->getClassification(), BodyClassification::Moon | BodyClassification::MinorMoon))
        {
            for (int bodyIdx = 0; bodyIdx < system->getSystemSize(); bodyIdx++)
            {
                Body *barycenterBody = system->getBody(bodyIdx);
                if (util::is_set(barycenterBody->getClassification(), BodyClassification::Planet | BodyClassification::DwarfPlanet))
                    return Selection(barycenterBody);
            }
        }
    }

    Star* primaryStar = system->getStar();
    if (primaryStar != nullptr)
        return Selection(primaryStar);

    return Selection();
}

std::string
Helper::getRenderInfo(const Renderer *r)
{
    std::map<std::string, std::string> info;
    r->getInfo(info);

    std::string s;
    s.reserve(4096);
    if (info.count("API") > 0 && info.count("APIVersion") > 0)
    s += fmt::sprintf(_("%s Version: %s\n"), info["API"], info["APIVersion"]);

    if (info.count("Vendor") > 0)
        s += fmt::sprintf(_("Vendor: %s\n"), info["Vendor"]);

    if (info.count("Renderer") > 0)
        s += fmt::sprintf(_("Renderer: %s\n"), info["Renderer"]);

    if (info.count("Language") > 0)
        s += fmt::sprintf(_("%s Version: %s\n"), info["Language"], info["LanguageVersion"]);

    if (info.count("ColorComponent") > 0)
        s += fmt::sprintf(_("Color component: %s\n"), info["ColorComponent"]);

    if (info.count("DepthComponent") > 0)
        s += fmt::sprintf(_("Depth component: %s\n"), info["DepthComponent"]);

    if (info.count("MaxTextureUnits") > 0)
        s += fmt::sprintf(_("Max simultaneous textures: %s\n"), info["MaxTextureUnits"]);

    if (info.count("MaxTextureSize") > 0)
        s += fmt::sprintf(_("Max texture size: %s\n"), info["MaxTextureSize"]);

    if (info.count("PointSizeMax") > 0 && info.count("PointSizeMin") > 0)
        s += fmt::sprintf(_("Point size range: %s - %s\n"), info["PointSizeMin"], info["PointSizeMax"]);

    if (info.count("LineWidthMax") > 0 && info.count("LineWidthMin") > 0)
        s += fmt::sprintf(_("Line width range: %s - %s\n"), info["LineWidthMin"], info["LineWidthMax"]);

    if (info.count("PointSizeGran") > 0)
        s += fmt::sprintf(_("Point size granularity: %s\n"), info["PointSizeGran"]);

    if (info.count("MaxCubeMapSize") > 0)
        s += fmt::sprintf(_("Max cube map size: %s\n"), info["MaxCubeMapSize"]);

    if (info.count("MaxVaryingFloats") > 0)
        s += fmt::sprintf(_("Number of interpolators: %s\n"), info["MaxVaryingFloats"]);

    if (info.count("MaxAnisotropy") > 0)
        s += fmt::sprintf(_("Max anisotropy filtering: %s\n"), info["MaxAnisotropy"]);

    s += "\n";

    if (info.count("Extensions") > 0)
    {
        s += "Supported Extensions:\n";
        auto ext = info["Extensions"];
        std::string::size_type old = 0, pos = ext.find(' ');
        while (pos != std::string::npos)
        {
            s += fmt::format("    {}\n", ext.substr(old, pos - old));
            old = pos + 1;
            pos = ext.find(' ', old);
        }
        s += fmt::format("    {}\n", ext.substr(old));
    }

    return s;
}
