// celx_object.cpp
//
// Copyright (C) 2003-2009, the Celestia Development Team
//
// Lua script extensions for Celestia: object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstring>
#include <iostream>

#include <celengine/atmosphere.h>
#include <celengine/axisarrow.h>
#include <celengine/body.h>
#include <celengine/location.h>
#include <celengine/planetgrid.h>
#include <celengine/multitexture.h>
#include <celengine/timeline.h>
#include <celengine/timelinephase.h>
#include <celengine/visibleregion.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celestia/celestiacore.h>
#include <celscript/common/scriptmaps.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_object.h"
#include "celx_category.h"

using namespace Eigen;
using namespace std;
using celestia::util::GetLogger;


static const char* bodyTypeName(BodyClassification cl)
{
    switch (cl)
    {
    case BodyClassification::Planet:
        return "planet";
    case BodyClassification::DwarfPlanet:
        return "dwarfplanet";
    case BodyClassification::Moon:
        return "moon";
    case BodyClassification::MinorMoon:
        return  "minormoon";
    case BodyClassification::Asteroid:
        return "asteroid";
    case BodyClassification::Comet:
        return "comet";
    case BodyClassification::Spacecraft:
        return "spacecraft";
    case BodyClassification::Invisible:
        return "invisible";
    case BodyClassification::SurfaceFeature:
        return "surfacefeature";
    case BodyClassification::Component:
        return "component";
    case BodyClassification::Diffuse:
        return "diffuse";
    default:
        return "unknown";
    }
}

static const char* dsoTypeName(DeepSkyObjectType dsoType)
{
    switch (dsoType)
    {
    case DeepSkyObjectType::Galaxy:
        return "galaxy";
    case DeepSkyObjectType::Globular:
        return "globular";
    case DeepSkyObjectType::Nebula:
        return "nebula";
    case DeepSkyObjectType::OpenCluster:
        return "opencluster";
    default:
        return "unknown";
    }
}

static celestia::MarkerRepresentation::Symbol parseMarkerSymbol(const string& name)
{
    using namespace celestia;

    if (compareIgnoringCase(name, "diamond") == 0)
        return MarkerRepresentation::Diamond;
    if (compareIgnoringCase(name, "triangle") == 0)
        return MarkerRepresentation::Triangle;
    if (compareIgnoringCase(name, "square") == 0)
        return MarkerRepresentation::Square;
    if (compareIgnoringCase(name, "filledsquare") == 0)
        return MarkerRepresentation::FilledSquare;
    if (compareIgnoringCase(name, "plus") == 0)
        return MarkerRepresentation::Plus;
    if (compareIgnoringCase(name, "x") == 0)
        return MarkerRepresentation::X;
    if (compareIgnoringCase(name, "leftarrow") == 0)
        return MarkerRepresentation::LeftArrow;
    if (compareIgnoringCase(name, "rightarrow") == 0)
        return MarkerRepresentation::RightArrow;
    if (compareIgnoringCase(name, "uparrow") == 0)
        return MarkerRepresentation::UpArrow;
    if (compareIgnoringCase(name, "downarrow") == 0)
        return MarkerRepresentation::DownArrow;
    if (compareIgnoringCase(name, "circle") == 0)
        return MarkerRepresentation::Circle;
    if (compareIgnoringCase(name, "disk") == 0)
        return MarkerRepresentation::Disk;
    else
        return MarkerRepresentation::Diamond;
}


// ==================== Object ====================
// star, planet, or deep-sky object
int object_new(lua_State* l, const Selection& sel)
{
    CelxLua celx(l);

    Selection* ud = static_cast<Selection*>(lua_newuserdata(l, sizeof(Selection)));
    *ud = sel;

    celx.setClass(Celx_Object);

    return 1;
}

Selection* to_object(lua_State* l, int index)
{
    CelxLua celx(l);
    return static_cast<Selection*>(celx.checkUserData(index, Celx_Object));
}

static Selection* this_object(lua_State* l)
{
    CelxLua celx(l);

    Selection* sel = to_object(l, 1);
    if (sel == nullptr)
    {
        celx.doError("Bad position object!");
    }

    return sel;
}


static int object_tostring(lua_State* l)
{
    lua_pushstring(l, "[Object]");

    return 1;
}


// Return true if the object is visible, false if not.
static int object_visible(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:visible");

    Selection* sel = this_object(l);
    lua_pushboolean(l, sel->isVisible());

    return 1;
}


// Set the object visibility flag.
static int object_setvisible(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to object:setvisible()");

    Selection* sel = this_object(l);
    bool visible = celx.safeGetBoolean(2, AllErrors, "Argument to object:setvisible() must be a boolean");
    if (sel->body() != nullptr)
    {
        sel->body()->setVisible(visible);
    }
    else if (sel->deepsky() != nullptr)
    {
        sel->deepsky()->setVisible(visible);
    }

    return 0;
}


static int object_setorbitcolor(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(4, 4, "Red, green, and blue color values exepected for object:setorbitcolor()");

    Selection* sel = this_object(l);
    float r = (float) celx.safeGetNumber(2, WrongType, "Argument 1 to object:setorbitcolor() must be a number", 0.0);
    float g = (float) celx.safeGetNumber(3, WrongType, "Argument 2 to object:setorbitcolor() must be a number", 0.0);
    float b = (float) celx.safeGetNumber(4, WrongType, "Argument 3 to object:setorbitcolor() must be a number", 0.0);
    Color orbitColor(r, g, b);

    if (const Body* body = sel->body(); body != nullptr)
        GetBodyFeaturesManager()->setOrbitColor(body, orbitColor);

    return 0;
}


static int object_orbitcoloroverridden(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to object:orbitcoloroverridden");

    bool isOverridden = false;
    if (const Body* body = this_object(l)->body(); body != nullptr)
        isOverridden = GetBodyFeaturesManager()->getOrbitColorOverridden(body);

    lua_pushboolean(l, isOverridden);

    return 1;
}


static int object_setorbitcoloroverridden(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to object:setorbitcoloroverridden");

    Selection* sel = this_object(l);
    bool overridden = celx.safeGetBoolean(2, AllErrors, "Argument to object:setorbitcoloroverridden() must be a boolean");

    if (Body* body = sel->body(); body != nullptr)
        GetBodyFeaturesManager()->setOrbitColorOverridden(body, overridden);

    return 0;
}


static int object_orbitvisibility(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to object:orbitvisibility");

    Body::VisibilityPolicy visibility = Body::UseClassVisibility;

    Selection* sel = this_object(l);
    if (sel->body() != nullptr)
    {
        visibility = sel->body()->getOrbitVisibility();
    }

    const char* s = "normal";
    if (visibility == Body::AlwaysVisible)
        s = "always";
    else if (visibility == Body::NeverVisible)
        s = "never";

    lua_pushstring(l, s);

    return 1;
}


static int object_setorbitvisibility(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to object:setorbitcoloroverridden");

    if (!lua_isstring(l, 2))
    {
        celx.doError("First argument to object:setorbitvisibility() must be a string");
    }

    Selection* sel = this_object(l);

    string key;
    key = lua_tostring(l, 2);

    auto &OrbitVisibilityMap = celx.appCore(AllErrors)->scriptMaps().OrbitVisibilityMap;
    if (OrbitVisibilityMap.count(key) == 0)
    {
        GetLogger()->warn("Unknown visibility policy: {}\n", key);
    }
    else
    {
        auto visibility = static_cast<Body::VisibilityPolicy>(OrbitVisibilityMap[key]);

        if (sel->body() != nullptr)
        {
            sel->body()->setOrbitVisibility(visibility);
        }
    }

    return 0;
}


static int object_addreferencemark(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Expected one table as argument to object:addreferencemark()");

    if (!lua_istable(l, 2))
    {
        celx.doError("Argument to object:addreferencemark() must be a table");
    }

    Selection* sel = this_object(l);
    Body* body = sel->body();

    lua_pushstring(l, "type");
    lua_gettable(l, 2);
    const char* rmtype = celx.safeGetString(3, NoErrors, "");
    lua_settop(l, 2);

    lua_pushstring(l, "size");
    lua_gettable(l, 2);
    float rmsize = (float) celx.safeGetNumber(3, NoErrors, "", body->getRadius()) + body->getRadius();
    lua_settop(l, 2);

    lua_pushstring(l, "opacity");
    lua_gettable(l, 2);
    // -1 indicates that the opacity wasn't set and the default value
    // should be used.
    float rmopacity = (float) celx.safeGetNumber(3, NoErrors, "", -1.0f);
    lua_settop(l, 2);

    lua_pushstring(l, "color");
    lua_gettable(l, 2);
    const char* rmcolorstring = celx.safeGetString(3, NoErrors, "");
    Color rmcolor(0.0f, 1.0f, 0.0f);
    if (rmcolorstring != nullptr)
        Color::parse(rmcolorstring, rmcolor);
    lua_settop(l, 2);

    lua_pushstring(l, "tag");
    lua_gettable(l, 2);
    const char* rmtag = celx.safeGetString(3, NoErrors, "");
    if (rmtag == nullptr)
        rmtag = rmtype;
    lua_settop(l, 2);

    lua_pushstring(l, "target");
    lua_gettable(l, 2);
    Selection* rmtarget = to_object(l, 3);
    lua_settop(l, 2);

    if (rmtype == nullptr)
        return 0;

    auto bodyFeaturesManager = GetBodyFeaturesManager();
    bodyFeaturesManager->removeReferenceMark(body, rmtype);

    if (compareIgnoringCase(rmtype, "body axes") == 0)
    {
        auto arrow = std::make_unique<BodyAxisArrows>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmopacity >= 0.0f)
            arrow->setOpacity(rmopacity);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "frame axes") == 0)
    {
        auto arrow = std::make_unique<FrameAxisArrows>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmopacity >= 0.0f)
            arrow->setOpacity(rmopacity);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "sun direction") == 0)
    {
        auto arrow = std::make_unique<SunDirectionArrow>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmcolorstring != nullptr)
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "velocity vector") == 0)
    {
        auto arrow = std::make_unique<VelocityVectorArrow>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmcolorstring != nullptr)
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "spin vector") == 0)
    {
        auto arrow = std::make_unique<SpinVectorArrow>(*body);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmcolorstring != nullptr)
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "body to body direction") == 0 && rmtarget != nullptr)
    {
        auto arrow = std::make_unique<BodyToBodyDirectionArrow>(*body, *rmtarget);
        arrow->setTag(rmtag);
        arrow->setSize(rmsize);
        if (rmcolorstring != nullptr)
            arrow->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(arrow));
    }
    else if (compareIgnoringCase(rmtype, "visible region") == 0 && rmtarget != nullptr)
    {
        auto region = std::make_unique<VisibleRegion>(*body, *rmtarget);
        region->setTag(rmtag);
        if (rmopacity >= 0.0f)
            region->setOpacity(rmopacity);
        if (rmcolorstring != nullptr)
            region->setColor(rmcolor);
        bodyFeaturesManager->addReferenceMark(body, std::move(region));
    }
    else if (compareIgnoringCase(rmtype, "planetographic grid") == 0)
    {
        bodyFeaturesManager->addReferenceMark(body,std::make_unique<PlanetographicGrid>(*body));
    }

    return 0;
}


static int object_removereferencemark(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1000, "Invalid number of arguments in object:removereferencemark");

    Selection* sel = this_object(l);
    Body* body = sel->body();
    if (body == nullptr)
        return 0;

    int argc = lua_gettop(l);
    auto bodyFeaturesManager = GetBodyFeaturesManager();
    for (int i = 2; i <= argc; i++)
    {
        const char* refMark = celx.safeGetString(i, AllErrors, "Arguments to object:removereferencemark() must be strings");
        bodyFeaturesManager->removeReferenceMark(body, refMark);
    }

    return 0;
}


static int object_radius(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:radius");

    Selection* sel = this_object(l);
    lua_pushnumber(l, sel->radius());

    return 1;
}

static int object_setradius(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to object:setradius()");

    Selection* sel = this_object(l);
    if (sel->body() == nullptr)
        return 0;

    Body* body = sel->body();
    float iradius = body->getRadius();
    double radius = celx.safeGetNumber(2, AllErrors, "Argument to object:setradius() must be a number");
    if (radius <= 0)
        return 0;

    float scaleFactor = static_cast<float>(radius) / iradius;
    body->setSemiAxes(body->getSemiAxes() * scaleFactor);
    GetBodyFeaturesManager()->scaleRings(body, scaleFactor);

    return 0;
}

static int object_type(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:type");

    Selection* sel = this_object(l);
    const char* tname;
    switch (sel->getType())
    {
    case SelectionType::Body:
        tname = bodyTypeName(sel->body()->getClassification());
        break;
    case SelectionType::Star:
        tname = "star";
        break;
    case SelectionType::DeepSky:
        tname = dsoTypeName(sel->deepsky()->getObjType());
        break;
    case SelectionType::Location:
        tname = "location";
        break;
    case SelectionType::None:
        tname = "null";
        break;
    default:
        assert(0);
        tname = "unknown";
        break;
    }

    lua_pushstring(l, tname);

    return 1;
}

static int object_name(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:name");

    Selection* sel = this_object(l);
    switch (sel->getType())
    {
        case SelectionType::Body:
            lua_pushstring(l, sel->body()->getName().c_str());
            break;
        case SelectionType::DeepSky:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getDSOCatalog()->getDSOName(sel->deepsky()).c_str());
            break;
        case SelectionType::Star:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getStarCatalog()->getStarName(*(sel->star())).c_str());
            break;
        case SelectionType::Location:
            lua_pushstring(l, sel->location()->getName().c_str());
            break;
        default:
            lua_pushstring(l, "?");
            break;
    }

    return 1;
}

static int object_localname(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:localname");

    Selection* sel = this_object(l);
    switch (sel->getType())
    {
        case SelectionType::Body:
            lua_pushstring(l, sel->body()->getName(true).c_str());
            break;
        case SelectionType::DeepSky:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getDSOCatalog()->getDSOName(sel->deepsky(), true).c_str());
            break;
        case SelectionType::Star:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getStarCatalog()->getStarName(*(sel->star()), true).c_str());
            break;
        case SelectionType::Location:
            lua_pushstring(l, sel->location()->getName(true).c_str());
            break;
        default:
            lua_pushstring(l, "?");
            break;
    }

    return 1;
}

static int object_spectraltype(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:spectraltype");

    Selection* sel = this_object(l);
    if (sel->star() != nullptr)
    {
        char buf[16];
        strncpy(buf, sel->star()->getSpectralType(), sizeof buf);
        buf[sizeof(buf) - 1] = '\0'; // make sure it's zero terminate
        lua_pushstring(l, buf);
    }
    else
    {
        lua_pushnil(l);
    }

    return 1;
}

static int object_getinfo(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:getinfo");

    lua_newtable(l);

    Selection* sel = this_object(l);
    if (sel->star() != nullptr)
    {
        Star* star = sel->star();
        celx.setTable("type", "star");
        celx.setTable("name", celx.appCore(AllErrors)->getSimulation()->getUniverse()
                 ->getStarCatalog()->getStarName(*(sel->star())).c_str());
        celx.setTable("catalogNumber", star->getIndex());
        celx.setTable("stellarClass", star->getSpectralType());
        celx.setTable("absoluteMagnitude", (lua_Number)star->getAbsoluteMagnitude());
        celx.setTable("luminosity", (lua_Number)star->getLuminosity());
        celx.setTable("radius", (lua_Number)star->getRadius());
        celx.setTable("temperature", (lua_Number)star->getTemperature());
        celx.setTable("rotationPeriod", (lua_Number)star->getRotationModel()->getPeriod());
        celx.setTable("bolometricMagnitude", (lua_Number)star->getBolometricMagnitude());

        const celestia::ephem::Orbit* orbit = star->getOrbit();
        if (orbit != nullptr)
            celx.setTable("orbitPeriod", orbit->getPeriod());

        if (star->getOrbitBarycenter() != nullptr)
        {
            Selection parent((Star*)(star->getOrbitBarycenter()));
            lua_pushstring(l, "parent");
            object_new(l, parent);
            lua_settable(l, -3);
        }
    }
    else if (sel->body() != nullptr)
    {
        Body* body = sel->body();
        const char* tname = bodyTypeName(body->getClassification());

        celx.setTable("type", tname);
        celx.setTable("name", body->getName().c_str());
        celx.setTable("mass", (lua_Number)body->getMass());
        celx.setTable("albedo", (lua_Number)body->getGeomAlbedo());
        celx.setTable("geomAlbedo", (lua_Number)body->getGeomAlbedo());
        celx.setTable("bondAlbedo", (lua_Number)body->getBondAlbedo());
        celx.setTable("reflectivity", (lua_Number)body->getReflectivity());
        celx.setTable("infoURL", body->getInfoURL().c_str());
        celx.setTable("radius", (lua_Number)body->getRadius());

        // TODO: add method to return semiaxes
        Vector3f semiAxes = body->getSemiAxes();
        // Note: oblateness is an obsolete field, replaced by semiaxes;
        // it's only here for backward compatibility.
        float polarRadius = semiAxes.y();
        float eqRadius = max(semiAxes.x(), semiAxes.z());
        celx.setTable("oblateness", (eqRadius - polarRadius) / eqRadius);

        double lifespanStart, lifespanEnd;
        body->getLifespan(lifespanStart, lifespanEnd);
        celx.setTable("lifespanStart", (lua_Number)lifespanStart);
        celx.setTable("lifespanEnd", (lua_Number)lifespanEnd);
        // TODO: atmosphere, surfaces ?

        PlanetarySystem* system = body->getSystem();
        if (system->getPrimaryBody() != nullptr)
        {
            Selection parent(system->getPrimaryBody());
            lua_pushstring(l, "parent");
            object_new(l, parent);
            lua_settable(l, -3);
        }
        else
        {
            Selection parent(system->getStar());
            lua_pushstring(l, "parent");
            object_new(l, parent);
            lua_settable(l, -3);
        }

        const BodyFeaturesManager* bodyFeaturesManager = GetBodyFeaturesManager();

        lua_pushstring(l, "hasRings");
        lua_pushboolean(l, bodyFeaturesManager->getRings(body) != nullptr);
        lua_settable(l, -3);

        // TIMELINE-TODO: The code to retrieve orbital and rotation periods only works
        // if the object has a single timeline phase. This should hardly ever
        // be a problem, but it still may be best to set the periods to zero
        // for objects with multiple phases.
        const celestia::ephem::RotationModel* rm = body->getRotationModel(0.0);
        celx.setTable("rotationPeriod", (double) rm->getPeriod());

        const celestia::ephem::Orbit* orbit = body->getOrbit(0.0);
        celx.setTable("orbitPeriod", orbit->getPeriod());
        const Atmosphere* atmosphere = bodyFeaturesManager->getAtmosphere(body);
        if (atmosphere != nullptr)
        {
            celx.setTable("atmosphereHeight", (double)atmosphere->height);
            celx.setTable("atmosphereCloudHeight", (double)atmosphere->cloudHeight);
            celx.setTable("atmosphereCloudSpeed", (double)atmosphere->cloudSpeed);
        }
    }
    else if (sel->deepsky() != nullptr)
    {
        DeepSkyObject* deepsky = sel->deepsky();
        const char* objTypeName = dsoTypeName(deepsky->getObjType());
        celx.setTable("type", objTypeName);

        celx.setTable("name", celx.appCore(AllErrors)->getSimulation()->getUniverse()
                 ->getDSOCatalog()->getDSOName(deepsky).c_str());
        celx.setTable("catalogNumber", deepsky->getIndex());

        if (!strcmp(objTypeName, "galaxy"))
            celx.setTable("hubbleType", deepsky->getType());

        celx.setTable("absoluteMagnitude", (lua_Number)deepsky->getAbsoluteMagnitude());
        celx.setTable("radius", (lua_Number)deepsky->getRadius());
    }
    else if (sel->location() != nullptr)
    {
        celx.setTable("type", "location");
        Location* location = sel->location();
        celx.setTable("name", location->getName().c_str());
        celx.setTable("size", (lua_Number)location->getSize());
        celx.setTable("importance", (lua_Number)location->getImportance());
        celx.setTable("infoURL", location->getInfoURL().c_str());

        auto featureType = location->getFeatureType();
        auto &LocationFlagMap = celx.appCore(AllErrors)->scriptMaps().LocationFlagMap;
        auto iter = std::find_if(LocationFlagMap.begin(),
                                 LocationFlagMap.end(),
                                 [&featureType](auto& it){ return it.second == featureType; });
        if (iter != LocationFlagMap.end())
             celx.setTable("featureType", std::string(iter->first).c_str());
        else
             celx.setTable("featureType", "Unknown");

        Body* parent = location->getParentBody();
        if (parent != nullptr)
        {
            Selection selection(parent);
            lua_pushstring(l, "parent");
            object_new(l, selection);
            lua_settable(l, -3);
        }
    }
    else
    {
        celx.setTable("type", "null");
    }
    return 1;
}

static int object_absmag(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:absmag");

    Selection* sel = this_object(l);
    if (sel->star() != nullptr)
        lua_pushnumber(l, sel->star()->getAbsoluteMagnitude());
    else
        lua_pushnil(l);

    return 1;
}

static int object_mark(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 7, "Need 0 to 6 arguments for object:mark");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    Color markColor(0.0f, 1.0f, 0.0f);
    const char* colorString = celx.safeGetString(2, WrongType, "First argument to object:mark must be a string");
    if (colorString != nullptr)
        Color::parse(colorString, markColor);

    auto markSymbol = celestia::MarkerRepresentation::Diamond;
    const char* markerString = celx.safeGetString(3, WrongType, "Second argument to object:mark must be a string");
    if (markerString != nullptr)
        markSymbol = parseMarkerSymbol(markerString);

    float markSize = (float)celx.safeGetNumber(4, WrongType, "Third arg to object:mark must be a number", 10.0);
    markSize = std::clamp(markSize, 1.0f, 10000.0f);

    float markAlpha = (float)celx.safeGetNumber(5, WrongType, "Fourth arg to object:mark must be a number", 0.9);
    markAlpha = std::clamp(markAlpha, 0.0f, 1.0f);

    Color markColorAlpha(markColor, markAlpha);

    const char* markLabel = celx.safeGetString(6, WrongType, "Fifth argument to object:mark must be a string");
    if (markLabel == nullptr)
        markLabel = "";

    bool occludable = celx.safeGetBoolean(7, WrongType, "Sixth argument to object:mark must be a boolean", true);

    Simulation* sim = appCore->getSimulation();

    celestia::MarkerRepresentation markerRep(markSymbol);
    markerRep.setSize(markSize);
    markerRep.setColor(markColorAlpha);
    markerRep.setLabel(markLabel);
    sim->getUniverse()->markObject(*sel, markerRep, 1, occludable);

    return 0;
}

static int object_unmark(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:unmark");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    Simulation* sim = appCore->getSimulation();
    sim->getUniverse()->unmarkObject(*sel, 1);

    return 0;
}

// Return the object's current position.  A time argument is optional;
// if not provided, the current master simulation time is used.
static int object_getposition(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "Expected no or one argument to object:getposition");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    double t = celx.safeGetNumber(2, WrongType, "Time expected as argument to object:getposition",
                                  appCore->getSimulation()->getTime());
    celx.newPosition(sel->getPosition(t));

    return 1;
}

static int object_getchildren(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected for object:getchildren()");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    Simulation* sim = appCore->getSimulation();

    lua_newtable(l);
    if (sel->star() != nullptr)
    {
        const SolarSystem* solarSys = sim->getUniverse()->getSolarSystem(sel->star());
        if (solarSys != nullptr)
        {
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize(); i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                Selection satSel(body);
                object_new(l, satSel);
                lua_rawseti(l, -2, i + 1);
            }
        }
    }
    else if (sel->body() != nullptr)
    {
        const PlanetarySystem* satellites = sel->body()->getSatellites();
        if (satellites != nullptr && satellites->getSystemSize() != 0)
        {
            for (int i = 0; i < satellites->getSystemSize(); i++)
            {
                Body* body = satellites->getBody(i);
                Selection satSel(body);
                object_new(l, satSel);
                lua_rawseti(l, -2, i + 1);
            }
        }
    }

    return 1;
}

static int object_preloadtexture(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected to object:preloadtexture");
    CelestiaCore* appCore = celx.appCore(AllErrors);

    Renderer* renderer = appCore->getRenderer();
    Selection* sel = this_object(l);

    if (sel->body() != nullptr && renderer != nullptr)
    {
        LuaState* luastate = celx.getLuaStateObject();
        // make sure we don't timeout because of texture-loading:
        double timeToTimeout = luastate->timeout - luastate->getTime();

        renderer->loadTextures(sel->body());

        // no matter how long it really took, make it look like 0.1s:
        luastate->timeout = luastate->getTime() + timeToTimeout - 0.1;
    }

    return 0;
}


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
static int object_catalognumber(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to object:catalognumber");
    CelestiaCore* appCore = celx.appCore(AllErrors);

    Selection* sel = this_object(l);
    const char* catalogName = celx.safeGetString(2, WrongType, "Argument to object:catalognumber must be a string");

    // The argument is a string indicating the catalog.
    bool validCatalog = false;
    bool useHIPPARCOS = false;
    StarCatalog catalog = StarCatalog::HenryDraper;
    if (catalogName != nullptr)
    {
        if (compareIgnoringCase(catalogName, "HD") == 0)
        {
            catalog = StarCatalog::HenryDraper;
            validCatalog = true;
        }
        else if (compareIgnoringCase(catalogName, "SAO") == 0)
        {
            catalog = StarCatalog::SAO;
            validCatalog = true;
        }
        else if (compareIgnoringCase(catalogName, "HIP") == 0)
        {
            useHIPPARCOS = true;
            validCatalog = true;
        }
    }

    uint32_t catalogNumber = AstroCatalog::InvalidIndex;
    if (sel->star() != nullptr && validCatalog)
    {
        uint32_t internalNumber = sel->star()->getIndex();

        if (useHIPPARCOS)
        {
            // Celestia's internal catalog numbers /are/ HIPPARCOS numbers
            if (internalNumber < StarDatabase::MAX_HIPPARCOS_NUMBER)
                catalogNumber = internalNumber;
        }
        else
        {
            const StarDatabase* stardb = appCore->getSimulation()->getUniverse()->getStarCatalog();
            catalogNumber = stardb->getNameDatabase()->crossIndex(catalog, internalNumber);
        }
    }

    if (catalogNumber != AstroCatalog::InvalidIndex)
        lua_pushnumber(l, catalogNumber);
    else
        lua_pushnil(l);

    return 1;
}


// Locations iterator function; two upvalues expected. Used by
// object:locations method.
static int object_locations_iter(lua_State* l)
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

    const Body* body = sel->body();
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
static int object_locations(lua_State* l)
{
    CelxLua celx(l);
    // Push a closure with two upvalues: the object and a counter
    lua_pushvalue(l, 1);    // object
    lua_pushnumber(l, 0);   // counter
    lua_pushcclosure(l, object_locations_iter, 2);

    return 1;
}


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
static int object_bodyfixedframe(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments allowed for object:bodyfixedframe");

    Selection* sel = this_object(l);
    celx.newFrame(ObserverFrame(ObserverFrame::CoordinateSystem::BodyFixed, *sel));

    return 1;
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
static int object_equatorialframe(lua_State* l)
{
    // TODO: allow one argument specifying a freeze time
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments allowed for to object:equatorialframe");

    Selection* sel = this_object(l);
    celx.newFrame(ObserverFrame(ObserverFrame::CoordinateSystem::Equatorial, *sel));

    return 1;
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
static int object_orbitframe(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or no arguments allowed for to object:orbitframe");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    double t = celx.safeGetNumber(2, WrongType, "Time expected as argument to object:orbitframe",
                                  appCore->getSimulation()->getTime());

    if (sel->body() == nullptr)
    {
        // The default universal frame
        celx.newFrame(ObserverFrame());
    }
    else
    {
        const auto& f = sel->body()->getOrbitFrame(t);
        celx.newFrame(ObserverFrame(f));
    }

    return 1;
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
static int object_bodyframe(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or no arguments allowed for to object:bodyframe");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    double t = celx.safeGetNumber(2, WrongType, "Time expected as argument to object:orbitframe",
                                  appCore->getSimulation()->getTime());

    if (sel->body() == nullptr)
    {
        // The default universal frame
        celx.newFrame(ObserverFrame());
    }
    else
    {
        const auto& f = sel->body()->getBodyFrame(t);
        celx.newFrame(ObserverFrame(f));
    }

    return 1;
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
static int object_getphase(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 2, "One or no arguments allowed for to object:getphase");

    Selection* sel = this_object(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);

    double t = celx.safeGetNumber(2, WrongType, "Time expected as argument to object:getphase",
                                  appCore->getSimulation()->getTime());

    if (sel->body() == nullptr)
    {
        lua_pushnil(l);
    }
    else
    {
        const Timeline* timeline = sel->body()->getTimeline();
        if (timeline->includes(t))
        {
            celx.newPhase(timeline->findPhase(t));
        }
        else
        {
            lua_pushnil(l);
        }
    }

    return 1;
}


// Phases iterator function; two upvalues expected. Used by
// object:phases method.
static int object_phases_iter(lua_State* l)
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
    if (sel->body() != nullptr)
    {
        timeline = sel->body()->getTimeline();
    }

    if (timeline != nullptr && i < timeline->phaseCount())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));

        const auto& phase = timeline->getPhase(i);
        celx.newPhase(phase);

        return 1;
    }

    // Return nil when we've enumerated all the phases (or if
    // if the object wasn't a solar system body.)
    return 0;
}


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
static int object_phases(lua_State* l)
{
    CelxLua celx(l);
    // Push a closure with two upvalues: the object and a counter
    lua_pushvalue(l, 1);    // object
    lua_pushnumber(l, 0);   // counter
    lua_pushcclosure(l, object_phases_iter, 2);

    return 1;
}


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
static int object_setringstexture(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "One or two arguments are expected for object:setringstexture()");

    const Body* body = this_object(l)->body();
    if (body == nullptr)
        return 0;

    RingSystem* rings = GetBodyFeaturesManager()->getRings(body);
    if (rings == nullptr)
        return 0;

    const char* textureName = celx.safeGetString(2);
    if (*textureName == '\0')
    {
        celx.doError("Empty texture name passed to object:setringstexture()");
        return 0;
    }
    const char* path = celx.safeGetString(3, WrongType);
    if (path == nullptr)
        path = "";

    rings->texture = MultiResTexture(textureName, path);

    return 0;
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
static int object_setcloudtexture(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "One or two arguments are expected for object:setcloudtexture()");

    const Body* body = this_object(l)->body();
    if (body == nullptr)
        return 0;

    auto atmosphere = GetBodyFeaturesManager()->getAtmosphere(body);
    if (atmosphere == nullptr)
        return 0;

    const char* textureName = celx.safeGetString(2);
    if (*textureName == '\0')
    {
        celx.doError("Empty texture name passed to object:setcloudtexture()");
        return 0;
    }
    const char* path = celx.safeGetString(3, WrongType);
    if (path == nullptr)
        path = "";

    atmosphere->cloudTexture = MultiResTexture(textureName, path);

    return 0;
}


static int object_getmass(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments are expected for object:getmass()");

    Selection* sel = this_object(l);

    if (sel->body() == nullptr)
        return 0;

    return celx.push(sel->body()->getMass());
}


static int object_getdensity(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments are expected for object:getdensity()");

    Selection* sel = this_object(l);

    if (sel->body() == nullptr)
        return 0;

    return celx.push(sel->body()->getDensity());
}


static int object_gettemperature(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments are expected for object:getdensity()");

    Selection* sel = this_object(l);

    float temp = 0;
    if (sel->body() != nullptr)
    {
        CelestiaCore* appCore = celx.appCore(AllErrors);
        double time = appCore->getSimulation()->getTime();
        temp = sel->body()->getTemperature(time);
    }
    else if (sel->star() != nullptr)
    {
        temp = sel->star()->getTemperature();
    }

    if (temp > 0)
        return celx.push(temp);

    return 0;
}

void CreateObjectMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Object);

    celx.registerMethod("__tostring", object_tostring);
    celx.registerMethod("visible", object_visible);
    celx.registerMethod("setvisible", object_setvisible);
    celx.registerMethod("orbitcoloroverridden", object_orbitcoloroverridden);
    celx.registerMethod("setorbitcoloroverridden", object_setorbitcoloroverridden);
    celx.registerMethod("setorbitcolor", object_setorbitcolor);
    celx.registerMethod("orbitvisibility", object_orbitvisibility);
    celx.registerMethod("setorbitvisibility", object_setorbitvisibility);
    celx.registerMethod("addreferencemark", object_addreferencemark);
    celx.registerMethod("removereferencemark", object_removereferencemark);
    celx.registerMethod("radius", object_radius);
    celx.registerMethod("setradius", object_setradius);
    celx.registerMethod("type", object_type);
    celx.registerMethod("spectraltype", object_spectraltype);
    celx.registerMethod("getinfo", object_getinfo);
    celx.registerMethod("catalognumber", object_catalognumber);
    celx.registerMethod("absmag", object_absmag);
    celx.registerMethod("name", object_name);
    celx.registerMethod("localname", object_localname);
    celx.registerMethod("mark", object_mark);
    celx.registerMethod("unmark", object_unmark);
    celx.registerMethod("getposition", object_getposition);
    celx.registerMethod("getchildren", object_getchildren);
    celx.registerMethod("locations", object_locations);
    celx.registerMethod("bodyfixedframe", object_bodyfixedframe);
    celx.registerMethod("equatorialframe", object_equatorialframe);
    celx.registerMethod("orbitframe", object_orbitframe);
    celx.registerMethod("bodyframe", object_bodyframe);
    celx.registerMethod("getphase", object_getphase);
    celx.registerMethod("phases", object_phases);
    celx.registerMethod("preloadtexture", object_preloadtexture);
    celx.registerMethod("setringstexture", object_setringstexture);
    celx.registerMethod("setcloudtexture", object_setcloudtexture);
    celx.registerMethod("gettemperature", object_gettemperature);
    celx.registerMethod("getmass", object_getmass);
    celx.registerMethod("getdensity", object_getdensity);

    lua_pop(l, 1); // pop metatable off the stack
}


// ==================== object extensions ====================

static const char* gettablekey(lua_State *l, const char *error)
{
    if (!lua_isstring(l, -2))
    {
        Celx_DoError(l, error);
        return nullptr;
    }
    return lua_tostring(l, -2);
}

static bool gettablevaluefloat(lua_State *l, const char *key, float *value)
{
    if (!lua_isnumber(l, -1))
    {
        Celx_DoError(l, fmt::format("Value of {} must be number", key).c_str());
        return false;
    }

    *value = static_cast<float>(lua_tonumber(l, -1));
    return true;
}

#if LUA_VERSION_NUM < 502
inline std::size_t lua_rawlen(lua_State *l, int idx)
{
    return lua_objlen(l, -1);
}
#endif

static bool gettablevaluevector3(lua_State *l, const char *key, Eigen::Vector3f *value)
{
    if (!lua_istable(l, -1) || lua_rawlen(l, -1) != 3)
    {
        Celx_DoError(l, fmt::format("Value of {} must be array of 3 numbers", key).c_str());
        return false;
    }

    for (int i = 1; i <= 3; i++)
    {
        lua_rawgeti(l, -1, i);
        (*value)[i - 1] = static_cast<float>(lua_tonumber(l, -1));
        lua_pop(l, 1);
    }
    return true;

}

static int object_setatmosphere(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One parameter expected to function object:setatmosphere");

    if (!lua_istable(l, 2))
    {
        Celx_DoError(l, "Argument to celestia:setatmosphere() must be a table");
        return 0;
    }

    Body* body = this_object(l)->body();
    if (body == nullptr)
        return 0;

    Atmosphere* atmosphere = GetBodyFeaturesManager()->getAtmosphere(body);
    if (atmosphere == nullptr)
        return 0;

    lua_pushnil(l);
    while (lua_next(l, -2) != 0)
    {
        const char *key = gettablekey(l, "Keys in table-argument to celestia:setatmosphere() must be strings");
        if (key == nullptr)
            return 0;

        float value;
        Eigen::Vector3f array;
        if (strcmp(key, "height") == 0)
        {
            if (!gettablevaluefloat(l, "height", &value))
                return 0;
            atmosphere->height = value;
        }
        else if (strcmp(key, "mie") == 0)
        {
            if (!gettablevaluefloat(l, "mie", &value))
                return 0;
            atmosphere->mieCoeff = value;
        }
        else if (strcmp(key, "miescaleheight") == 0)
        {
            if (!gettablevaluefloat(l, "miescaleheight", &value))
                return 0;
            atmosphere->mieScaleHeight = value;
        }
        else if (strcmp(key, "mieasymmetry") == 0)
        {
            if (!gettablevaluefloat(l, "mieasymmetry", &value))
                return 0;
            atmosphere->miePhaseAsymmetry = value;
        }
#if 0
        else if (strcmp(key, "rayleighscaleheight") == 0)
        {
            if (!gettablevaluefloat(l, "rayleighscaleheight", &value))
                return 0;
            atmosphere->rayleighScaleHeight = value;
        }
#endif
        else if (strcmp(key, "rayleigh") == 0) {
            if (!gettablevaluevector3(l, "rayleigh", &array))
                return 0;
            atmosphere->rayleighCoeff = array;
        }
        else if (strcmp(key, "absorption") == 0) {
            if (!gettablevaluevector3(l, "absorption", &array))
                return 0;
            atmosphere->absorptionCoeff = array;
        }
        else if (strcmp(key, "lowercolor") == 0)
        {
            if (!gettablevaluevector3(l, "lowercolor", &array))
                return 0;
            atmosphere->lowerColor = array;
        }
        else if (strcmp(key, "uppercolor") == 0)
        {
            if (!gettablevaluevector3(l, "uppercolor", &array))
                return 0;
            atmosphere->upperColor = array;
        }
        else if (strcmp(key, "skycolor") == 0)
        {
            if (!gettablevaluevector3(l, "skycolor", &array))
                return 0;
            atmosphere->skyColor = array;
        }
        else if (strcmp(key, "sunsetcolor") == 0)
        {
            if (!gettablevaluevector3(l, "sunsetcolor", &array))
                return 0;
            atmosphere->sunsetColor = array;
        }
        else
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
        lua_pop(l, 1);
    }

    body->recomputeCullingRadius();

    return 0;
}

#define checkEmpty(c,s) \
    if (s->empty()) \
    { \
        c.doError("Selection object is empty!"); \
        return 0; \
    }

static int object_getcategories(lua_State *l)
{
    CelxLua celx(l);

    Selection *s = celx.getThis<Selection>();
    checkEmpty(celx, s);
    const auto* set = UserCategory::getCategories(*s);
    return celx.pushIterable<UserCategoryId>(set);
}

static int object_addtocategory(lua_State *l)
{
    CelxLua celx(l);

    Selection *s = celx.getThis<Selection>();
    checkEmpty(celx, s);
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

static int object_removefromcategory(lua_State *l)
{
    CelxLua celx(l);

    Selection *s = celx.getThis<Selection>();
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

void ExtendObjectMetaTable(lua_State* l)
{
    CelxLua celx(l);
    celx.pushClassName(Celx_Object);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        std::cout << "Metatable for " << CelxLua::classNameForId(Celx_Object) << " not found!\n";
    celx.registerMethod("setatmosphere", object_setatmosphere);
    celx.registerMethod("getcategories", object_getcategories);
    celx.registerMethod("addtocategory", object_addtocategory);
    celx.registerMethod("removefromcategory", object_removefromcategory);
    celx.pop(1);
}
