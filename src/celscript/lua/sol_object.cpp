#include <iostream>
#include <string_view>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "celx_internal.h" // TODO
#include "sol_phase.h"

#include <celengine/atmosphere.h>
#include <celengine/axisarrow.h>
#include <celengine/body.h>
#include <celengine/deepskyobj.h>
#include <celengine/frametree.h>
#include <celengine/location.h>
#include <celengine/marker.h>
#include <celengine/planetgrid.h>
#include <celengine/timeline.h>
#include <celengine/visibleregion.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celestia/celestiacore.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>

using namespace std::string_view_literals;
using namespace celestia::celx;
using celestia::util::GetLogger;

namespace
{

// TODO: move to own file, rewrite without CelxLua
CelestiaCore *
getAppCore(sol::this_state s)
{
    return CelxLua(s).appCore(AllErrors);
}

std::string_view
bodyTypeName(BodyClassification cl)
{
    switch (cl)
    {
    case BodyClassification::Planet:
        return "planet"sv;
    case BodyClassification::DwarfPlanet:
        return "dwarfplanet"sv;
    case BodyClassification::Moon:
        return "moon"sv;
    case BodyClassification::MinorMoon:
        return "minormoon"sv;
    case BodyClassification::Asteroid:
        return "asteroid"sv;
    case BodyClassification::Comet:
        return "comet"sv;
    case BodyClassification::Spacecraft:
        return "spacecraft"sv;
    case BodyClassification::Invisible:
        return "invisible"sv;
    case BodyClassification::SurfaceFeature:
        return "surfacefeature"sv;
    case BodyClassification::Component:
        return "component"sv;
    case BodyClassification::Diffuse:
        return "diffuse"sv;
    default:
        return "unknown"sv;
    }
}

std::string_view
dsoTypeName(DeepSkyObjectType dsoType)
{
    switch (dsoType)
    {
    case DeepSkyObjectType::Galaxy:
        return "galaxy"sv;
    case DeepSkyObjectType::Globular:
        return "globular"sv;
    case DeepSkyObjectType::Nebula:
        return "nebula"sv;
    case DeepSkyObjectType::OpenCluster:
        return "opencluster"sv;
    default:
        return "unknown"sv;
    }
}

celestia::engine::MarkerSymbol
parseMarkerSymbol(std::string_view name)
{
    using enum celestia::engine::MarkerSymbol;

    if (compareIgnoringCase(name, "diamond"sv) == 0)
        return Diamond;
    if (compareIgnoringCase(name, "triangle"sv) == 0)
        return Triangle;
    if (compareIgnoringCase(name, "square"sv) == 0)
        return Square;
    if (compareIgnoringCase(name, "filledsquare"sv) == 0)
        return FilledSquare;
    if (compareIgnoringCase(name, "plus"sv) == 0)
        return Plus;
    if (compareIgnoringCase(name, "x"sv) == 0)
        return X;
    if (compareIgnoringCase(name, "leftarrow"sv) == 0)
        return LeftArrow;
    if (compareIgnoringCase(name, "rightarrow"sv) == 0)
        return RightArrow;
    if (compareIgnoringCase(name, "uparrow"sv) == 0)
        return UpArrow;
    if (compareIgnoringCase(name, "downarrow"sv) == 0)
        return DownArrow;
    if (compareIgnoringCase(name, "circle"sv) == 0)
        return Circle;
    if (compareIgnoringCase(name, "disk"sv) == 0)
        return Disk;
    else
        return Diamond;
}

#if 0
// Return true if the object is visible, false if not.
bool visible(const Selection &sel)
{
    return sel.isVisible();
}
#endif

// Set the object visibility flag.
void
setvisible(Selection &sel, bool visible)
{
    if (sel.body() != nullptr)
        sel.body()->setVisible(visible);
    else if (sel.deepsky() != nullptr)
        sel.deepsky()->setVisible(visible);
}

void
setorbitcolor(Selection &sel, float r, float g, float b)
{
    if (const auto *body = sel.body(); body != nullptr)
        GetBodyFeaturesManager()->setOrbitColor(body, { r, g, b });
}

bool
orbitcoloroverridden(const Selection &sel)
{
    if (const auto *body = sel.body(); body != nullptr)
        return GetBodyFeaturesManager()->getOrbitColorOverridden(body);
    return false;
}

void
setorbitcoloroverridden(const Selection &sel, bool overridden)
{
    if (auto *body = sel.body(); body != nullptr)
        GetBodyFeaturesManager()->setOrbitColorOverridden(body, overridden);
}

std::string_view
orbitvisibility(const Selection &sel)
{
    Body::VisibilityPolicy visibility = Body::UseClassVisibility;

    if (sel.body() != nullptr)
        visibility = sel.body()->getOrbitVisibility();

    switch (visibility)
    {
    case Body::AlwaysVisible:
        return "always"sv;
    case Body::NeverVisible:
        return "never"sv;
    default:
        return "normal"sv;
    }
}

void
setorbitvisibility(const Selection &sel, std::string_view key, sol::this_state s)
{
    if (auto &ovm = getAppCore(s)->scriptMaps().OrbitVisibilityMap; ovm.contains(key))
    {
        auto visibility = static_cast<Body::VisibilityPolicy>(ovm[key]);

        if (sel.body() != nullptr)
            sel.body()->setOrbitVisibility(visibility);
    }
    else
    {
        GetLogger()->warn("Unknown visibility policy: {}\n", key);
    }
}

void
addreferencemark(const Selection &sel, sol::table table)
{
    auto *body = sel.body();

    std::string_view rmtype = table.get_or("type"sv, std::string_view());
    if (rmtype.empty())
        return;
    float rmsize = table.get_or("size"sv, body->getRadius()) + body->getRadius();
    // -1 indicates that the opacity wasn't set and the default value
    // should be used.
    float            rmopacity     = table.get_or("opacity"sv, -1.0f);
    std::string_view rmcolorstring = table.get_or("color"sv, std::string_view());
    Color            rmcolor(0.0f, 1.0f, 0.0f);
    if (!rmcolorstring.empty())
        Color::parse(rmcolorstring, rmcolor);
    std::string_view rmtag = table.get_or("tag"sv, rmtype);

    Selection empty;
    Selection rmtarget = table.get_or("target"sv, empty);

    auto bodyFeaturesManager = GetBodyFeaturesManager();
    bodyFeaturesManager->removeReferenceMark(body, rmtype);

    if (compareIgnoringCase(rmtype, "body axes"sv) == 0)
    {
        auto arrow = std::make_unique<BodyAxisArrows>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmopacity >= 0.0f)
            arrow->setOpacity(rmopacity);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "frame axes"sv) == 0)
    {
        auto arrow = std::make_unique<FrameAxisArrows>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmopacity >= 0.0f)
            arrow->setOpacity(rmopacity);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "sun direction"sv) == 0)
    {
        auto arrow = std::make_unique<SunDirectionArrow>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (!rmcolorstring.empty())
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "velocity vector"sv) == 0)
    {
        auto arrow = std::make_unique<VelocityVectorArrow>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (!rmcolorstring.empty())
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "spin vector"sv) == 0)
    {
        auto arrow = std::make_unique<SpinVectorArrow>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (!rmcolorstring.empty())
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "body to body direction"sv) == 0 && !rmtarget.empty())
    {
        auto arrow = std::make_unique<BodyToBodyDirectionArrow>(*body, rmtarget);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (!rmcolorstring.empty())
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "visible region"sv) == 0 && !rmtarget.empty())
    {
        auto region = std::make_unique<VisibleRegion>(*body, rmtarget);
        region->setTag(rmtag);
        if (rmopacity >= 0.0f)
            region->setOpacity(rmopacity);
        if (!rmcolorstring.empty())
            region->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(region));
    }
    else if (compareIgnoringCase(rmtype, "planetographic grid"sv) == 0)
    {
        bodyFeaturesManager->addReferenceMark(body, std::make_unique<PlanetographicGrid>(*body));
    }
}

void
removereferencemark(const Selection &sel, sol::variadic_args va)
{
    auto *body = sel.body();
    if (body == nullptr)
        return;

    auto bodyFeaturesManager = GetBodyFeaturesManager();
    for (const auto &v : va)
    {
        std::string_view refMark = v.get<std::string_view>();
        bodyFeaturesManager->removeReferenceMark(body, refMark);
    }
}

#if 0
double radius(const Selection &sel)
{
    return sel.radius();
}
#endif

void
setradius(const Selection &sel, float radius)
{
    if (sel.body() == nullptr || radius <= 0.0f)
        return;

    auto *body        = sel.body();
    float scaleFactor = radius / body->getRadius();
    body->setSemiAxes(body->getSemiAxes() * scaleFactor);
    GetBodyFeaturesManager()->scaleRings(body, scaleFactor);
}

std::string_view
type(const Selection &sel)
{
    switch (sel.getType())
    {
    case SelectionType::Body:
        return bodyTypeName(sel.body()->getClassification());
    case SelectionType::Star:
        return "star"sv;
    case SelectionType::DeepSky:
        return dsoTypeName(sel.deepsky()->getObjType());
    case SelectionType::Location:
        return "location"sv;
    case SelectionType::None:
        return "null"sv;
    default:
        assert(0);
        return "unknown"sv;
    }
}

std::string
name(const Selection &sel, sol::this_state s)
{
    switch (sel.getType())
    {
    case SelectionType::Body:
        return sel.body()->getName();
    case SelectionType::DeepSky:
        return getAppCore(s)->getSimulation()->getUniverse()->getDSOCatalog()->getDSOName(
            sel.deepsky());
    case SelectionType::Star:
        return getAppCore(s)->getSimulation()->getUniverse()->getStarCatalog()->getStarName(
            *(sel.star()));
    case SelectionType::Location:
        return sel.location()->getName();
    default:
        return "?";
    }
}

std::string
localname(const Selection &sel, sol::this_state s)
{
    switch (sel.getType())
    {
    case SelectionType::Body:
        return sel.body()->getName(true);
    case SelectionType::DeepSky:
        return getAppCore(s)->getSimulation()->getUniverse()->getDSOCatalog()->getDSOName(
            sel.deepsky(),
            true);
    case SelectionType::Star:
        return getAppCore(s)->getSimulation()->getUniverse()->getStarCatalog()->getStarName(
            *(sel.star()),
            true);
    case SelectionType::Location:
        return sel.location()->getName(true);
    default:
        return "?";
    }
}

sol::optional<std::string>
spectraltype(const Selection &sel)
{
    if (sel.star() != nullptr)
        return { sel.star()->getSpectralType() };
    return sol::nullopt;
}

sol::table
getinfo(const Selection &sel, sol::this_state s)
{
    auto table = sol::state_view(s).create_table();

    if (sel.star() != nullptr)
    {
        auto *star    = sel.star();
        table["type"] = "star";
        table["name"]
            = getAppCore(s)->getSimulation()->getUniverse()->getStarCatalog()->getStarName(
                *(sel.star()));
        table["catalogNumber"]       = star->getIndex();
        table["stellarClass"]        = star->getSpectralType();
        table["absoluteMagnitude"]   = star->getAbsoluteMagnitude();
        table["luminosity"]          = star->getLuminosity();
        table["radius"]              = star->getRadius();
        table["temperature"]         = star->getTemperature();
        table["rotationPeriod"]      = star->getRotationModel()->getPeriod();
        table["bolometricMagnitude"] = star->getBolometricMagnitude();

        const auto *orbit = star->getOrbit();
        if (orbit != nullptr)
            table["orbitPeriod"] = orbit->getPeriod();

        if (star->getOrbitBarycenter() != nullptr)
            table["parent"] = Selection(star->getOrbitBarycenter());
    }
    else if (sel.body() != nullptr)
    {
        auto *body = sel.body();

        table["type"]       = bodyTypeName(body->getClassification());
        table["name"]       = body->getName();
        table["mass"]       = body->getMass();
        table["albedo"]     = body->getGeomAlbedo();
        table["geomAlbedo"] = body->getGeomAlbedo();
        table["bondAlbedo"] = body->getBondAlbedo();
        table["sphAlbedo"]  = body->getSphAlbedo();
        table["infoURL"]    = getAppCore(s)->getSimulation()->getUniverse()->getInfoURL(body);
        table["radius"]     = body->getRadius();

        // TODO: add method to return semiaxes
        Eigen::Vector3f semiAxes = body->getSemiAxes();
        // Note: oblateness is an obsolete field, replaced by semiaxes;
        // it's only here for backward compatibility.
        float polarRadius   = semiAxes.y();
        float eqRadius      = std::max(semiAxes.x(), semiAxes.z());
        table["oblateness"] = (eqRadius - polarRadius) / eqRadius;

        double lifespanStart, lifespanEnd;
        body->getLifespan(lifespanStart, lifespanEnd);
        table["lifespanStart"] = lifespanStart;
        table["lifespanEnd"]   = lifespanEnd;
        // TODO: atmosphere, surfaces ?

        auto *system    = body->getSystem();
        table["parent"] = system->getPrimaryBody() != nullptr ?
                              Selection(system->getPrimaryBody()) :
                              Selection(system->getStar());

        const auto *bfm   = GetBodyFeaturesManager();
        table["hasRings"] = bfm->getRings(body) != nullptr;

        // TIMELINE-TODO: The code to retrieve orbital and rotation periods only works
        // if the object has a single timeline phase. This should hardly ever
        // be a problem, but it still may be best to set the periods to zero
        // for objects with multiple phases.
        const auto *rm          = body->getRotationModel(0.0);
        table["rotationPeriod"] = rm->getPeriod();

        const auto *orbit      = body->getOrbit(0.0);
        table["orbitPeriod"]   = orbit->getPeriod();
        const auto *atmosphere = bfm->getAtmosphere(body);
        if (atmosphere != nullptr)
        {
            table["atmosphereHeight"]      = atmosphere->height;
            table["atmosphereCloudHeight"] = atmosphere->cloudHeight;
            table["atmosphereCloudSpeed"]  = atmosphere->cloudSpeed;
        }
    }
    else if (sel.deepsky() != nullptr)
    {
        const auto *deepsky     = sel.deepsky();
        auto        objTypeName = dsoTypeName(deepsky->getObjType());
        table["type"]           = objTypeName;
        table["name"]
            = getAppCore(s)->getSimulation()->getUniverse()->getDSOCatalog()->getDSOName(deepsky);
        table["catalogNumber"] = deepsky->getIndex();
        if (objTypeName == "galaxy"sv)
            table["hubbleType"] = deepsky->getType();

        table["absoluteMagnitude"] = deepsky->getAbsoluteMagnitude();
        table["radius"]            = deepsky->getRadius();
    }
    else if (sel.location() != nullptr)
    {
        table["type"]       = "location";
        auto *location      = sel.location();
        table["name"]       = location->getName();
        table["size"]       = location->getSize();
        table["importance"] = location->getImportance();
        table["infoURL"]    = getAppCore(s)->getSimulation()->getUniverse()->getInfoURL(location);

        auto  featureType = location->getFeatureType();
        auto &lfm         = getAppCore(s)->scriptMaps().LocationFlagMap;
        auto  iter        = std::find_if(
            lfm.begin(),
            lfm.end(),
            [&featureType](auto &it) { return it.second == featureType; });
        table["featureType"] = iter != lfm.end() ? iter->first : "Unknown"sv;

        if (auto *parent = location->getParentBody(); parent != nullptr)
            table["parent"] = Selection(parent);
    }
    else
    {
        table["type"] = "null";
    }
    return table;
}

sol::optional<float>
absmag(const Selection &sel)
{
    if (sel.star() != nullptr)
        return { sel.star()->getAbsoluteMagnitude() };
    return sol::nullopt;
}

void
mark(
    const Selection                &sel,
    sol::optional<std::string_view> colorString,
    sol::optional<std::string_view> markerString,
    sol::optional<float>            markSize,
    sol::optional<float>            markAlpha,
    sol::optional<std::string_view> markLabel,
    sol::optional<bool>             occludable,
    sol::this_state                 s)
{
    Color markColor(0.0f, 1.0f, 0.0f);
    if (!colorString->empty())
        Color::parse(*colorString, markColor);

    auto markSymbol = celestia::engine::MarkerSymbol::Diamond;
    if (!markerString->empty())
        markSymbol = parseMarkerSymbol(*markerString);

    Color color(markColor, std::clamp(markAlpha.value_or(0.9f), 0.0f, 1.0f));

    const auto                            *sim = getAppCore(s)->getSimulation();
    celestia::engine::MarkerRepresentation markerRep(markSymbol);
    markerRep.setSize(std::clamp(markSize.value_or(10.0f), 1.0f, 10000.0f));
    markerRep.setColor(color);
    markerRep.setLabel(markLabel.value_or(std::string_view()));
    sim->getUniverse()->markObject(sel, markerRep, 1, occludable.value_or(true));
}

void
unmark(const Selection &sel, sol::this_state s)
{
    getAppCore(s)->getSimulation()->getUniverse()->unmarkObject(sel, 1);
}

// Return the object's current position.  A time argument is optional;
// if not provided, the current master simulation time is used.
UniversalCoord
getposition(const Selection &sel, std::optional<double> t, sol::this_state s)
{
    return sel.getPosition(t.value_or(getAppCore(s)->getSimulation()->getTime()));
}

sol::table
getchildren(const Selection &sel, sol::this_state s)
{
    auto table = sol::state_view(s).create_table();
    if (sel.star() != nullptr)
    {
        const auto *solarSys
            = getAppCore(s)->getSimulation()->getUniverse()->getSolarSystem(sel.star());
        if (solarSys != nullptr)
        {
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize(); i++)
                table[i] = Selection(solarSys->getPlanets()->getBody(i));
        }
    }
    else if (sel.body() != nullptr)
    {
        const auto *satellites = sel.body()->getSatellites();
        if (satellites != nullptr)
        {
            for (int i = 0; i < satellites->getSystemSize(); i++)
                table[i] = Selection(satellites->getBody(i));
        }
    }

    return table;
}

#if 0
static int object_preloadtexture(const Selection &sel)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected to object:preloadtexture");
    CelestiaCore* appCore = getAppCore(s);

    Renderer* renderer = appCore->getRenderer();


    if (sel.body() != nullptr && renderer != nullptr)
    {
        LuaState* luastate = celx.getLuaStateObject();
        // make sure we don't timeout because of texture-loading:
        double timeToTimeout = luastate->timeout - luastate->getTime();

        renderer->loadTextures(sel.body());

        // no matter how long it really took, make it look like 0.1s:
        luastate->timeout = luastate->getTime() + timeToTimeout - 0.1;
    }

    return 0;
}
#endif

/*! object:catalognumber(string: catalog_prefix)
 *
 *  Look up the catalog number for a star in one of the supported catalogs,
 *  currently HIPPARCOS, HD, or SAO. The single argument is a string that
 *  specifies the catalog number, either "HD", "SAO", or "HIP".
 *  If the object is a star, the catalog string is valid, and the star
 *  is present in the catalog, the catalog number is returned on the stack.
 *  Otherwise, nil is returned.
 *
 * \verbatim
 * -- Example: Get the SAO and HD catalog numbers for Rigel
 * --
 * rigel = celestia:find("Rigel")
 * sao = rigel:catalognumber("SAO")
 * hd = rigel:catalognumber("HD")
 *
 * \endverbatim
 */
sol::optional<std::uint32_t>
catalognumber(const Selection &sel, std::string_view catalogName, sol::this_state s)
{
    // The argument is a string indicating the catalog.
    bool        validCatalog = false;
    bool        useHIPPARCOS = false;
    StarCatalog catalog      = StarCatalog::HenryDraper;
    if (!catalogName.empty())
    {
        if (compareIgnoringCase(catalogName, "HD"sv) == 0)
        {
            catalog      = StarCatalog::HenryDraper;
            validCatalog = true;
        }
        else if (compareIgnoringCase(catalogName, "SAO"sv) == 0)
        {
            catalog      = StarCatalog::SAO;
            validCatalog = true;
        }
        else if (compareIgnoringCase(catalogName, "HIP"sv) == 0)
        {
            useHIPPARCOS = true;
            validCatalog = true;
        }
    }

    uint32_t catalogNumber = AstroCatalog::InvalidIndex;
    if (sel.star() != nullptr && validCatalog)
    {
        std::uint32_t internalNumber = sel.star()->getIndex();

        if (useHIPPARCOS)
        {
            // Celestia's internal catalog numbers /are/ HIPPARCOS numbers
            if (internalNumber < StarDatabase::MAX_HIPPARCOS_NUMBER)
                catalogNumber = internalNumber;
        }
        else
        {
            const StarDatabase *stardb
                = getAppCore(s)->getSimulation()->getUniverse()->getStarCatalog();
            catalogNumber = stardb->getNameDatabase()->crossIndex(catalog, internalNumber);
        }
    }

    if (catalogNumber != AstroCatalog::InvalidIndex)
        return catalogNumber;

    return sol::nullopt;
}

// Locations iterator function; two upvalues expected. Used by
// object:locations method.
#if 0
static int object_locations_iter(const Selection &sel)
{
    CelxLua celx(l);
    Selection* sel = to_object(l, lua_upvalueindex(1));
    if (sel == nullptr)
    {
        celx.doError("Bad object!");
        return 0;
    }

    // Get the current counter value
    uint32_t i = (uint32_t) lua_tonumber(l, lua_upvalueindex(2));

    const auto *body = sel.body();
    if (body == nullptr)
        return 0;

    auto locations = GetBodyFeaturesManager()->getLocations(body);
    if (!locations.has_value() || i >= locations->size())
    {
        // Return nil when we've enumerated all the locations (or if
        // there were no locations associated with the object.)
        return 0;
    }

    // Increment the counter
    lua_pushnumber(l, i + 1);
    lua_replace(l, lua_upvalueindex(2));

    Location* loc = (*locations)[i];
    if (loc == nullptr)
        lua_pushnil(l);
    else
        object_new(l, Selection(loc));

    return 1;
}
#endif

/*! object:locations()
 *
 * Return an iterator over all the locations associated with an object.
 * Only solar system bodies have locations; for all other object types,
 * this method will return an empty iterator.
 *
 * \verbatim
 * -- Example: print locations of current selection
 * --
 * for loc in celestia:getselection():locations() do
 *     celestia:log(loc:name())
 * end
 *
 * \endverbatim
 */
#if 0
static int object_locations(const Selection &sel)
{
    CelxLua celx(l);
    // Push a closure with two upvalues: the object and a counter
    lua_pushvalue(l, 1);    // object
    lua_pushnumber(l, 0);   // counter
    lua_pushcclosure(l, object_locations_iter, 2);

    return 1;
}
#endif

/*! object:bodyfixedframe()
 *
 * Return the body-fixed frame for this object.
 *
 * \verbatim
 * -- Example: get the body-fixed frame of the Earth
 * --
 * earth = celestia:find("Sol/Earth")
 * ebf = earth:bodyfixedframe()
 *
 * \endverbatim
 */
ObserverFrame
bodyfixedframe(const Selection &sel)
{
    return { ObserverFrame::CoordinateSystem::BodyFixed, sel };
}

/*! object:equatorialframe()
 *
 * Return the mean equatorial frame for this object.
 *
 * \verbatim
 * -- Example: getthe equatorial frame of the Earth
 * --
 * earth = celestia:find("Sol/Earth")
 * eme = earth:equatorialframe()
 *
 * \endverbatim
 */
ObserverFrame
equatorialframe(const Selection &sel)
{
    // TODO: allow one argument specifying a freeze time
    return { ObserverFrame::CoordinateSystem::Equatorial, sel };
}

/*! object:orbitframe(time: t)
 *
 * Return the frame in which the orbit for an object is defined at a particular
 * time. If time isn't specified, the current simulation time is assumed. The
 * positions of stars and deep sky objects are always defined in the universal
 * frame.
 *
 * \verbatim
 * -- Example: get the orbital frame for the Earth at the current time.
 * --
 * earth = celestia:find("Sol/Earth")
 * eof = earth:orbitframe()
 *
 * \endverbatim
 */
ObserverFrame
orbitframe(const Selection &sel, sol::optional<double> jd, sol::this_state s)
{
    if (sel.body() == nullptr)
    {
        // The default universal frame
        return {};
    }

    double      t     = jd.value_or(getAppCore(s)->getSimulation()->getTime());
    const auto &phase = sel.body()->getTimeline()->findPhase(t);
    return { phase.getFrameTree()->getOwner(), phase.orbitFrame() };
}

/*! object:bodyframe(time: t)
 *
 * Return the frame in which the orientation for an object is defined at a
 * particular time. If time isn't specified, the current simulation time is
 * assumed. The positions of stars and deep sky objects are always defined
 * in the universal frame.
 *
 * \verbatim
 * -- Example: get the current body frame for the International Space Station.
 * --
 * iss = celestia:find("Sol/Earth/ISS")
 * f = iss:bodyframe()
 *
 * \endverbatim
 */
ObserverFrame
bodyframe(const Selection &sel, sol::optional<double> jd, sol::this_state s)
{
    if (sel.body() == nullptr)
    {
        // The default universal frame
        return {};
    }

    double t = jd.value_or(getAppCore(s)->getSimulation()->getTime());
    return { sel.body(), sel.body()->getBodyFrame(t) };
}

/*! object:getphase(time: t)
 *
 * Get the active timeline phase at the specified time. If no time is
 * specified, the current simulation time is used. This method returns
 * nil if the object is not a solar system body, or if the time lies
 * outside the range covered by the timeline.
 *
 * \verbatim
 * -- Example: get the timeline phase for Cassini at midnight January 1, 2000 UTC.
 * --
 * cassini = celestia:find("Sol/Cassini")
 * tdb = celestia:utctotdb(2000, 1, 1)
 * phase = cassini:getphase(tdb)
 *
 * \endverbatim
 */
sol::optional<CelxPhase>
getphase(const Selection &sel, sol::optional<double> jd, sol::this_state s)
{
    if (sel.body() == nullptr)
        return sol::nullopt;

    const auto *timeline = sel.body()->getTimeline();
    double      t        = jd.value_or(getAppCore(s)->getSimulation()->getTime());
    if (timeline->includes(t))
        return { CelxPhase(&timeline->findPhase(t)) };

    return sol::nullopt;
}

// Phases iterator function; two upvalues expected. Used by
// object:phases method.
#if 0
static int object_phases_iter(const Selection &sel)
{
    CelxLua celx(l);
    Selection* sel = to_object(l, lua_upvalueindex(1));
    if (sel == nullptr)
    {
        celx.doError("Bad object!");
        return 0;
    }

    // Get the current counter value
    uint32_t i = (uint32_t) lua_tonumber(l, lua_upvalueindex(2));

    const Timeline* timeline = nullptr;
    if (sel.body() != nullptr)
    {
        timeline = sel.body()->getTimeline();
    }

    if (timeline != nullptr && i < timeline->phaseCount())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));

        celx.newPhase(&timeline->getPhase(i));

        return 1;
    }

    // Return nil when we've enumerated all the phases (or if
    // if the object wasn't a solar system body.)
    return 0;
}
#endif

/*! object:phases()
 *
 * Return an iterator over all the phases in an object's timeline.
 * Only solar system bodies have timeline; for all other object types,
 * this method will return an empty iterator. The phases in a timeline
 * are always sorted from earliest to latest, and always coverage a
 * continuous span of time.
 *
 * \verbatim
 * -- Example: copy all of an objects phases into the array timeline
 * --
 * timeline = { }
 * count = 0
 * for phase in celestia:getselection():phases() do
 *     count = count + 1
 *     timeline[count] = phase
 * end
 *
 * \endverbatim
 */
#if 0
static int object_phases(const Selection &sel)
{
    CelxLua celx(l);
    // Push a closure with two upvalues: the object and a counter
    lua_pushvalue(l, 1);    // object
    lua_pushnumber(l, 0);   // counter
    lua_pushcclosure(l, object_phases_iter, 2);

    return 1;
}
#endif

/*! object:setringstexture(string: texture_name, string: path)
 *
 * Sets the texture for the object's rings. The texture at path will
 * be used to render the rings of the object. If no path is provided,
 * the texture is loaded from the default location.
 *
 * \verbatim
 * -- Example: set rings texture for Saturn
 * --
 * earth = celestia:find("Sol/Saturn")
 * earth:setcloudtexture("saturn_rings.png", "my_dir")
 *
 * \endverbatim
 */
void
setringstexture(
    const Selection                &sel,
    std::string_view                textureName,
    sol::optional<std::string_view> path,
    sol::this_state                 s)
{
    const auto *body = sel.body();
    if (body == nullptr)
        return;

    auto *rings = GetBodyFeaturesManager()->getRings(body);
    if (rings == nullptr)
        return;

    if (textureName.empty())
    {
        // TODO: abort
        std::cerr << "Empty texture name passed to object:setringstexture()";
        return;
    }

    rings->texture = getAppCore(s)->getTexturePaths()->getHandle(
        textureName,
        path.value_or(std::string_view()));
}

/*! object:setcloudtexture(string: texture_name, string: path)
 *
 * Sets the cloud texture for the object's atmosphere. The texture at path
 * will be used to render the clouds on the object's surface. If no path is
 * provided, the texture is loaded from the default location.
 *
 * \verbatim
 * -- Example: set cloud texture on Earth
 * --
 * earth = celestia:find("Sol/Earth")
 * earth:setcloudtexture("earth_clouds.png", "my_dir")
 *
 * \endverbatim
 */
void
setcloudtexture(
    const Selection                &sel,
    std::string_view                textureName,
    sol::optional<std::string_view> path,
    sol::this_state                 s)
{
    const auto *body = sel.body();
    if (body == nullptr)
        return;

    auto *atmosphere = GetBodyFeaturesManager()->getAtmosphere(body);
    if (atmosphere == nullptr)
        return;

    if (textureName.empty())
    {
        // TODO: abort
        std::cerr << "Empty texture name passed to object:setcloudtexture()";
        return;
    }
    atmosphere->cloudTexture = getAppCore(s)->getTexturePaths()->getHandle(
        textureName,
        path.value_or(std::string_view()),
        celestia::engine::TextureFlags::WrapTexture);
}

sol::optional<float>
getmass(const Selection &sel)
{
    if (sel.body() == nullptr)
        return sol::nullopt;

    return sel.body()->getMass();
}

sol::optional<float>
getdensity(const Selection &sel)
{
    if (sel.body() == nullptr)
        return sol::nullopt;

    return sel.body()->getDensity();
}

sol::optional<float>
gettemperature(const Selection &sel, sol::this_state s)
{
    float temp = 0.0f;
    if (sel.body() != nullptr)
    {
        double time = getAppCore(s)->getSimulation()->getTime();
        temp        = sel.body()->getTemperature(time);
    }
    else if (sel.star() != nullptr)
    {
        temp = sel.star()->getTemperature();
    }

    if (temp > 0.0f)
        return temp;

    return sol::nullopt;
}

// ==================== object extensions ====================

bool
gettablevaluefloat(sol::object luaValue, std::string_view key, float *value)
{
    if (luaValue.get_type() != sol::type::number)
    {
        // TODO: abort
        std::cerr << fmt::format("Value of {} must be number", key);
        return false;
    }

    *value = static_cast<float>(luaValue.as<float>());
    return true;
}

bool
gettablevaluevector3(sol::object luaValue, std::string_view key, Eigen::Vector3f *value)
{
    if (luaValue.get_type() != sol::type::table || luaValue.as<sol::table>().size() != 3)
    {
        // TODO: abort
        std::cerr << fmt::format("Value of {} must be array of 3 numbers", key);
        return false;
    }

    sol::table table = luaValue.as<sol::table>();
    for (int i = 1; i <= 3; ++i) (*value)[i - 1] = table.get<float>(i);

    return true;
}

void
setatmosphere(const Selection &sel, sol::table table)
{
    auto *body = sel.body();
    if (body == nullptr)
        return;

    auto *atmosphere = GetBodyFeaturesManager()->getAtmosphere(body);
    if (atmosphere == nullptr)
        return;

    for (const auto &[luaKey, luaValue] : table)
    {
        if (luaKey.get_type() != sol::type::string)
        {
            // TODO: abort
            std::cerr << "Keys in table-argument to celestia:setatmosphere() must be strings";
            return;
        }
        std::string_view key = luaKey.as<std::string_view>();

        Eigen::Vector3f array;
        if (key == "height"sv)
        {
            if (!gettablevaluefloat(luaValue, "height"sv, &atmosphere->height))
                return;
        }
        else if (key == "mie"sv)
        {
            if (!gettablevaluefloat(luaValue, "mie"sv, &atmosphere->mieCoeff))
                return;
        }
        else if (key == "miescaleheight"sv)
        {
            if (!gettablevaluefloat(luaValue, "miescaleheight"sv, &atmosphere->mieScaleHeight))
                return;
        }
        else if (key == "mieasymmetry"sv)
        {
            if (!gettablevaluefloat(luaValue, "mieasymmetry"sv, &atmosphere->miePhaseAsymmetry))
                return;
        }
        else if (key == "rayleighscaleheight"sv)
        {
            if (!gettablevaluefloat(
                    luaValue,
                    "rayleighscaleheight"sv,
                    &atmosphere->rayleighScaleHeight))
                return;
        }
        else if (key == "rayleigh")
        {
            if (!gettablevaluevector3(luaValue, "rayleigh", &array))
                return;
            atmosphere->rayleighCoeff = array;
        }
        else if (key == "absorption")
        {
            if (!gettablevaluevector3(luaValue, "absorption", &array))
                return;
            atmosphere->absorptionCoeff = array;
        }
        else if (key == "lowercolor")
        {
            if (!gettablevaluevector3(luaValue, "lowercolor", &array))
                return;
            atmosphere->lowerColor = array;
        }
        else if (key == "uppercolor"sv)
        {
            if (!gettablevaluevector3(luaValue, "uppercolor"sv, &array))
                return;
            atmosphere->upperColor = array;
        }
        else if (key == "skycolor"sv)
        {
            if (!gettablevaluevector3(luaValue, "skycolor"sv, &array))
                return;
            atmosphere->skyColor = array;
        }
        else if (key == "sunsetcolor"sv)
        {
            if (!gettablevaluevector3(luaValue, "sunsetcolor"sv, &array))
                return;
            atmosphere->sunsetColor = array;
        }
        else
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
    }

    body->recomputeCullingRadius();
}

#if 0
#define checkEmpty(c, s)                         \
    if (s->empty())                              \
    {                                            \
        c.doError("Selection object is empty!"); \
        return 0;                                \
    }

static int object_getcategories(const Selection &sel,)
{
    checkEmpty(celx, sel);
    const auto* set = UserCategory::getCategories(s);
    return celx.pushIterable<UserCategoryId>(set);
}


static int object_addtocategory(const Selection &sel)
{
    checkEmpty(celx, sel);
    bool ret;
    if (celx.isUserData(2))
    {
        auto c = *celx.getUserData<UserCategoryId>(2);
        ret = UserCategory::addObject(*s, c);
    }
    else
    {
        const char *n = celx.safeGetString(2, AllErrors, "Argument to object:addtocategory() must be string or userdata");
        if (n == nullptr)
            return celx.push(false);
        auto c = UserCategory::find(n);
        ret = UserCategory::addObject(*s, c);
    }
    return celx.push(ret);
}

static int object_removefromcategory(const Selection &sel)
{
    checkEmpty(celx, s);
    bool ret;
    if (celx.isUserData(2))
    {
        UserCategoryId c = *celx.getUserData<UserCategoryId>(2);
        ret = UserCategory::removeObject(*s, c);
    }
    else
    {
        const char *n = celx.safeGetString(2, AllErrors, "Argument to object:addtocategory() must be string or userdata");
        if (n == nullptr)
            return celx.push(false);
        auto c = UserCategory::find(n);
        ret = UserCategory::removeObject(*s, c);
    }
    return celx.push(ret);
}
#endif

} // namespace

void
CreateObjectMetaTable(sol::state_view lua)
{
    lua.new_usertype<Selection>(
        "Object",
        sol::no_constructor,
        "__tostring",
        []() { return "[Object]"; },
        "visible",
        &Selection::isVisible,
        "setvisible",
        setvisible,
        "orbitcoloroverridden",
        orbitcoloroverridden,
        "setorbitcoloroverridden",
        setorbitcoloroverridden,
        "setorbitcolor",
        setorbitcolor,
        "orbitvisibility",
        orbitvisibility,
        "setorbitvisibility",
        setorbitvisibility,
        "addreferencemark",
        addreferencemark,
        "removereferencemark",
        removereferencemark,
        "radius",
        &Selection::radius,
        "setradius",
        setradius,
        "type",
        type,
        "spectraltype",
        spectraltype,
        "getinfo",
        getinfo,
        "catalognumber",
        catalognumber,
        "absmag",
        absmag,
        "name",
        name,
        "localname",
        localname,
        "mark",
        mark,
        "unmark",
        unmark,
        "getposition",
        getposition,
        "getchildren",
        getchildren,
        //"locations", locations,
        "bodyfixedframe",
        bodyfixedframe,
        "equatorialframe",
        equatorialframe,
        "orbitframe",
        orbitframe,
        "bodyframe",
        bodyframe,
        "getphase",
        getphase,
        //"phases", phases,
        //"preloadtexture", preloadtexture,
        "setringstexture",
        setringstexture,
        "setcloudtexture",
        setcloudtexture,
        "gettemperature",
        gettemperature,
        "getmass",
        getmass,
        "getdensity",
        getdensity,
        "setatmosphere",
        setatmosphere
        //"getcategories", getcategories,
        //"addtocategory", addtocategory,
        //"removefromcategory", removefromcategory
    );
}
