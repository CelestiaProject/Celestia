#include <fmt/printf.h>
#include <celengine/body.h>
#include <celengine/render.h>
#include <celengine/selection.h>
#include <celutil/gettext.h>
#include "helper.h"


constexpr const int SpacecraftPrimaryBody = Body::Planet | Body::DwarfPlanet | Body::Moon |
                                            Body::MinorMoon | Body::Asteroid | Body::Comet;


bool Helper::hasPrimaryStar(const Body* body)
{
    return body->getSystem() != nullptr && body->getSystem()->getStar() != nullptr;
}


bool Helper::hasPrimaryBody(const Body* body, int classification)
{
    return body->getSystem() != nullptr && body->getSystem()->getPrimaryBody() != nullptr &&
           (body->getSystem()->getPrimaryBody()->getClassification() & classification);
}


bool Helper::hasBarycenter(const Body* body)
{
    PlanetarySystem *system = body->getSystem();
    if (system == nullptr || system->getPrimaryBody() == nullptr ||
        !(system->getPrimaryBody()->getClassification() & Body::Invisible))
    {
        return false;
    }
    for (int bodyIdx = 0; bodyIdx < system->getSystemSize(); bodyIdx++)
    {
        if (system->getBody(bodyIdx)->getClassification() & (Body::Planet | Body::DwarfPlanet))
            return true;
    }
    return false;
}


bool Helper::hasPrimary(const Body* body)
{
    switch (body->getClassification())
    {
    case Body::Planet:
    case Body::DwarfPlanet:
    case Body::Asteroid:
    case Body::Comet:
        return hasPrimaryStar(body);
    case Body::Moon:
    case Body::MinorMoon:
        return hasPrimaryBody(body, Body::Planet | Body::DwarfPlanet) ||
            hasBarycenter(body);
    case Body::Spacecraft:
        return hasPrimaryBody(body, SpacecraftPrimaryBody) || hasPrimaryStar(body);
    default:
        break;
    }
    return false;
}


Selection Helper::getPrimary(const Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == nullptr)
        return Selection();

    Body* primaryBody = system->getPrimaryBody();
    if (primaryBody != nullptr)
    {
        int primaryClass = primaryBody->getClassification();
        if (primaryClass & SpacecraftPrimaryBody)
            return Selection(primaryBody);

        if ((primaryClass & Body::Invisible) &&
            (body->getClassification() & (Body::Moon | Body::MinorMoon)))
        {
            for (int bodyIdx = 0; bodyIdx < system->getSystemSize(); bodyIdx++)
            {
                Body *barycenterBody = system->getBody(bodyIdx);
                if (barycenterBody->getClassification() & (Body::Planet | Body::DwarfPlanet))
                    return Selection(barycenterBody);
            }
        }
    }

    Star* primaryStar = system->getStar();
    if (primaryStar != nullptr)
        return Selection(primaryStar);

    return Selection();
}

std::string Helper::getRenderInfo(const Renderer *r)
{
    map<string, string> info;
    r->getInfo(info);

    string s;
    s.reserve(4096);
    if (info.count("API") > 0 && info.count("APIVersion") > 0)
    s += fmt::sprintf(_("%s Version: %s\n"), info["API"], info["APIVersion"]);

    if (info.count("Vendor") > 0)
        s += fmt::sprintf(_("Vendor: %s\n"), info["Vendor"]);

    if (info.count("Renderer") > 0)
        s += fmt::sprintf(_("Renderer: %s\n"), info["Renderer"]);

    if (info.count("Language") > 0)
        s += fmt::sprintf(_("%s Version: %s\n"), info["Language"], info["LanguageVersion"]);

    if (info.count("MaxTextureUnits") > 0)
        s += fmt::sprintf(_("Max simultaneous textures: %s\n"), info["MaxTextureUnits"]);

    if (info.count("MaxTextureSize") > 0)
        s += fmt::sprintf(_("Max texture size: %s\n"), info["MaxTextureSize"]);

    if (info.count("PointSizeMax") > 0 && info.count("PointSizeMin") > 0)
        s += fmt::sprintf(_("Point size range: %s - %s\n"), info["PointSizeMin"], info["PointSizeMax"]);

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
        string::size_type old = 0, pos = ext.find(' ');
        while (pos != string::npos)
        {
            s += fmt::sprintf("    %s\n", ext.substr(old, pos - old));
            old = pos + 1;
            pos = ext.find(' ', old);
        }
        s += fmt::sprintf("    %s\n", ext.substr(old));
    }

    return s;
}
