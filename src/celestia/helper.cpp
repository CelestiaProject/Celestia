#include <celengine/body.h>
#include <celengine/selection.h>
#include "helper.h"


constexpr const int SpacecraftPrimaryBody = Body::Planet | Body::DwarfPlanet | Body::Moon |
                                            Body::MinorMoon | Body::Asteroid | Body::Comet;


bool Helper::hasPrimaryStar(Body* body)
{
    return body->getSystem() != nullptr && body->getSystem()->getStar() != nullptr;
}


bool Helper::hasPrimaryBody(Body* body, int classification)
{
    return body->getSystem() != nullptr && body->getSystem()->getPrimaryBody() != nullptr &&
           (body->getSystem()->getPrimaryBody()->getClassification() & classification);
}


bool Helper::hasBarycenter(Body* body)
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
        {
            return true;
        }
    }
    return false;
}


bool Helper::hasPrimary(Body* body)
{
    switch (body->getClassification()) {
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


Selection Helper::getPrimary(Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == nullptr)
    {
        return Selection((Body*)nullptr);
    }

    Body* primaryBody = system->getPrimaryBody();
    if (primaryBody != nullptr) {
        int primaryClass = primaryBody->getClassification();
        if (primaryClass & SpacecraftPrimaryBody)
        {
            return Selection(primaryBody);
        }
        if ((primaryClass & Body::Invisible) &&
            (body->getClassification() & (Body::Moon | Body::MinorMoon)))
        {
            for (int bodyIdx = 0; bodyIdx < system->getSystemSize(); bodyIdx++)
            {
                Body *barycenterBody = system->getBody(bodyIdx);
                if (barycenterBody->getClassification() & (Body::Planet | Body::DwarfPlanet))
                {
                    return Selection(barycenterBody);
                }
            }
        }
    }

    Star* primaryStar = system->getStar();
    if (primaryStar != nullptr)
    {
        return Selection(primaryStar);
    }

    return Selection((Body*)nullptr);
}