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

#include <cstring>
#include "celx.h"
#include "celx_internal.h"
#include "celx_object.h"
#include <celengine/body.h>
#include <celengine/timelinephase.h>
#include <celengine/axisarrow.h>
#include <celengine/visibleregion.h>
#include <celengine/planetgrid.h>
#include "celestiacore.h"


using namespace std;


static MarkerRepresentation::Symbol parseMarkerSymbol(const string& name)
{
    if (compareIgnoringCase(name, "diamond") == 0)
        return MarkerRepresentation::Diamond;
    else if (compareIgnoringCase(name, "triangle") == 0)
        return MarkerRepresentation::Triangle;
    else if (compareIgnoringCase(name, "square") == 0)
        return MarkerRepresentation::Square;
    else if (compareIgnoringCase(name, "filledsquare") == 0)
        return MarkerRepresentation::FilledSquare;
    else if (compareIgnoringCase(name, "plus") == 0)
        return MarkerRepresentation::Plus;
    else if (compareIgnoringCase(name, "x") == 0)
        return MarkerRepresentation::X;
    else if (compareIgnoringCase(name, "leftarrow") == 0)
        return MarkerRepresentation::LeftArrow;
    else if (compareIgnoringCase(name, "rightarrow") == 0)
        return MarkerRepresentation::RightArrow;
    else if (compareIgnoringCase(name, "uparrow") == 0)
        return MarkerRepresentation::UpArrow;
    else if (compareIgnoringCase(name, "downarrow") == 0)
        return MarkerRepresentation::DownArrow;
    else if (compareIgnoringCase(name, "circle") == 0)
        return MarkerRepresentation::Circle;
    else if (compareIgnoringCase(name, "disk") == 0)
        return MarkerRepresentation::Disk;
    else
        return MarkerRepresentation::Diamond;
}


// ==================== Object ====================
// star, planet, or deep-sky object
int object_new(lua_State* l, const Selection& sel)
{
    CelxLua celx(l);
    
    Selection* ud = reinterpret_cast<Selection*>(lua_newuserdata(l, sizeof(Selection)));
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
    if (sel == NULL)
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
    if (sel->body() != NULL)
    {
        sel->body()->setVisible(visible);
    }
    else if (sel->deepsky() != NULL)
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
    
    if (sel->body() != NULL)
    {
        sel->body()->setOrbitColor(orbitColor);
    }
    
    return 0;
}


static int object_orbitcoloroverridden(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to object:orbitcoloroverridden");
    
    bool isOverridden = false;
    Selection* sel = this_object(l);
    if (sel->body() != NULL)
    {
        isOverridden = sel->body()->isOrbitColorOverridden();
    }
    
    lua_pushboolean(l, isOverridden);
    
    return 1;
}


static int object_setorbitcoloroverridden(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to object:setorbitcoloroverridden");
    
    Selection* sel = this_object(l);
    bool override = celx.safeGetBoolean(2, AllErrors, "Argument to object:setorbitcoloroverridden() must be a boolean");
    
    if (sel->body() != NULL)
    {
        sel->body()->setOrbitColorOverridden(override);
    }
    
    return 0;
}


static int object_orbitvisibility(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to object:orbitvisibility");
    
    Body::VisibilityPolicy visibility = Body::UseClassVisibility;
    
    Selection* sel = this_object(l);
    if (sel->body() != NULL)
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
    
    if (CelxLua::OrbitVisibilityMap.count(key) == 0)
    {
        cerr << "Unknown visibility policy: " << key << endl;
    }
    else
    {
        Body::VisibilityPolicy visibility = static_cast<Body::VisibilityPolicy>(CelxLua::OrbitVisibilityMap[key]);
        
        if (sel->body() != NULL)
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
    if (rmcolorstring != NULL)
        Color::parse(rmcolorstring, rmcolor);
    lua_settop(l, 2);
    
    lua_pushstring(l, "tag");
    lua_gettable(l, 2);
    const char* rmtag = celx.safeGetString(3, NoErrors, "");
    if (rmtag == NULL)
        rmtag = rmtype;
    lua_settop(l, 2);
    
    lua_pushstring(l, "target");
    lua_gettable(l, 2);
    Selection* rmtarget = to_object(l, 3);
    lua_settop(l, 2);
    
    if (rmtype != NULL)
    {
        body->removeReferenceMark(rmtype);
        
        if (compareIgnoringCase(rmtype, "body axes") == 0)
        {
            BodyAxisArrows* arrow = new BodyAxisArrows(*body);
            arrow->setTag(rmtag);
            arrow->setSize(rmsize);
            if (rmopacity >= 0.0f)
                arrow->setOpacity(rmopacity);
            body->addReferenceMark(arrow);
        }
        else if (compareIgnoringCase(rmtype, "frame axes") == 0)
        {
            FrameAxisArrows* arrow = new FrameAxisArrows(*body);
            arrow->setTag(rmtag);
            arrow->setSize(rmsize);
            if (rmopacity >= 0.0f)
                arrow->setOpacity(rmopacity);
            body->addReferenceMark(arrow);
        }
        else if (compareIgnoringCase(rmtype, "sun direction") == 0)
        {
            SunDirectionArrow* arrow = new SunDirectionArrow(*body);
            arrow->setTag(rmtag);
            arrow->setSize(rmsize);
            if (rmcolorstring != NULL)
                arrow->setColor(rmcolor);
            body->addReferenceMark(arrow);
        }
        else if (compareIgnoringCase(rmtype, "velocity vector") == 0)
        {
            VelocityVectorArrow* arrow = new VelocityVectorArrow(*body);
            arrow->setTag(rmtag);
            arrow->setSize(rmsize);
            if (rmcolorstring != NULL)
                arrow->setColor(rmcolor);
            body->addReferenceMark(arrow);
        }
        else if (compareIgnoringCase(rmtype, "spin vector") == 0)
        {
            SpinVectorArrow* arrow = new SpinVectorArrow(*body);
            arrow->setTag(rmtag);
            arrow->setSize(rmsize);
            if (rmcolorstring != NULL)
                arrow->setColor(rmcolor);
            body->addReferenceMark(arrow);
        }
        else if (compareIgnoringCase(rmtype, "body to body direction") == 0 && rmtarget != NULL)
        {
            BodyToBodyDirectionArrow* arrow = new BodyToBodyDirectionArrow(*body, *rmtarget);
            arrow->setTag(rmtag);
            arrow->setSize(rmsize);
            if (rmcolorstring != NULL)
                arrow->setColor(rmcolor);
            body->addReferenceMark(arrow);
        }
        else if (compareIgnoringCase(rmtype, "visible region") == 0 && rmtarget != NULL)
        {
            VisibleRegion* region = new VisibleRegion(*body, *rmtarget);
            region->setTag(rmtag);
            if (rmopacity >= 0.0f)
                region->setOpacity(rmopacity);
            if (rmcolorstring != NULL)
                region->setColor(rmcolor);
            body->addReferenceMark(region);
        }
        else if (compareIgnoringCase(rmtype, "planetographic grid") == 0)
        {
            PlanetographicGrid* grid = new PlanetographicGrid(*body);
            body->addReferenceMark(grid);
        }
    }
    
    return 0;
}


static int object_removereferencemark(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1000, "Invalid number of arguments in object:removereferencemark");
    CelestiaCore* appCore = celx.appCore(AllErrors);
    
    Selection* sel = this_object(l);
    Body* body = sel->body();
    
    int argc = lua_gettop(l);
    for (int i = 2; i <= argc; i++)
    {
        string refMark = celx.safeGetString(i, AllErrors, "Arguments to object:removereferencemark() must be strings");
        
        if (body->findReferenceMark(refMark))
            appCore->toggleReferenceMark(refMark, *sel);
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
    if (sel->body() != NULL)
    {
        Body* body = sel->body();
        float iradius = body->getRadius();
        double radius = celx.safeGetNumber(2, AllErrors, "Argument to object:setradius() must be a number");
        if ((radius > 0))
        {
            body->setSemiAxes(body->getSemiAxes() * ((float) radius / iradius));
        }
        
        if (body->getRings() != NULL)
        {
            RingSystem rings(0.0f, 0.0f);
            rings = *body->getRings();
            float inner = rings.innerRadius;
            float outer = rings.outerRadius;
            rings.innerRadius = inner * (float) radius / iradius;
            rings.outerRadius = outer * (float) radius / iradius;
            body->setRings(rings);
        }
    }
    
    return 0;
}

static int object_type(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to function object:type");
    
    Selection* sel = this_object(l);
    const char* tname = "unknown";
    switch (sel->getType())
    {
        case Selection::Type_Body:
        {
            int cl = sel->body()->getClassification();
            switch (cl)
            {
                case Body::Planet : tname = "planet"; break;
                case Body::DwarfPlanet : tname = "dwarfplanet"; break;
                case Body::Moon : tname = "moon"; break;
                case Body::MinorMoon : tname = "minormoon"; break;
                case Body::Asteroid : tname = "asteroid"; break;
                case Body::Comet : tname = "comet"; break;
                case Body::Spacecraft : tname = "spacecraft"; break;
                case Body::Invisible : tname = "invisible"; break;
                case Body::SurfaceFeature : tname = "surfacefeature"; break;
                case Body::Component : tname = "component"; break;
                case Body::Diffuse : tname = "diffuse"; break;
            }
        }
            break;
            
        case Selection::Type_Star:
            tname = "star";
            break;
            
        case Selection::Type_DeepSky:
            tname = sel->deepsky()->getObjTypeName();
            break;
            
        case Selection::Type_Location:
            tname = "location";
            break;
            
        case Selection::Type_Nil:
            tname = "null";
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
        case Selection::Type_Body:
            lua_pushstring(l, sel->body()->getName().c_str());
            break;
        case Selection::Type_DeepSky:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getDSOCatalog()->getDSOName(sel->deepsky()).c_str());
            break;
        case Selection::Type_Star:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getStarCatalog()->getStarName(*(sel->star())).c_str());
            break;
        case Selection::Type_Location:
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
        case Selection::Type_Body:
            lua_pushstring(l, sel->body()->getName(true).c_str());
            break;
        case Selection::Type_DeepSky:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getDSOCatalog()->getDSOName(sel->deepsky(), true).c_str());
            break;
        case Selection::Type_Star:
            lua_pushstring(l, celx.appCore(AllErrors)->getSimulation()->getUniverse()
                           ->getStarCatalog()->getStarName(*(sel->star()), true).c_str());
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
    if (sel->star() != NULL)
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
    if (sel->star() != NULL)
    {
        Star* star = sel->star();
        celx.setTable("type", "star");
        celx.setTable("name", celx.appCore(AllErrors)->getSimulation()->getUniverse()
                 ->getStarCatalog()->getStarName(*(sel->star())).c_str());
        celx.setTable("catalogNumber", star->getCatalogNumber());
        celx.setTable("stellarClass", star->getSpectralType());
        celx.setTable("absoluteMagnitude", (lua_Number)star->getAbsoluteMagnitude());
        celx.setTable("luminosity", (lua_Number)star->getLuminosity());
        celx.setTable("radius", (lua_Number)star->getRadius());
        celx.setTable("temperature", (lua_Number)star->getTemperature());
        celx.setTable("rotationPeriod", (lua_Number)star->getRotationModel()->getPeriod());
        celx.setTable("bolometricMagnitude", (lua_Number)star->getBolometricMagnitude());

        const Orbit* orbit = star->getOrbit();
        if (orbit != NULL)
            celx.setTable("orbitPeriod", orbit->getPeriod());

        if (star->getOrbitBarycenter() != NULL)
        {
            Selection parent((Star*)(star->getOrbitBarycenter()));
            lua_pushstring(l, "parent");
            object_new(l, parent);
            lua_settable(l, -3);
        }
    }
    else if (sel->body() != NULL)
    {
        Body* body = sel->body();
        const char* tname = "unknown";
        switch (body->getClassification())
        {
            case Body::Planet : tname = "planet"; break;
            case Body::DwarfPlanet : tname = "dwarfplanet"; break;
            case Body::Moon : tname = "moon"; break;
            case Body::MinorMoon : tname = "minormoon"; break;
            case Body::Asteroid : tname = "asteroid"; break;
            case Body::Comet : tname = "comet"; break;
            case Body::Spacecraft : tname = "spacecraft"; break;
            case Body::Invisible : tname = "invisible"; break;
            case Body::SurfaceFeature : tname = "surfacefeature"; break;
            case Body::Component : tname = "component"; break;
            case Body::Diffuse : tname = "diffuse"; break;
        }
        
        celx.setTable("type", tname);
        celx.setTable("name", body->getName().c_str());
        celx.setTable("mass", (lua_Number)body->getMass());
        celx.setTable("albedo", (lua_Number)body->getAlbedo());
        celx.setTable("infoURL", body->getInfoURL().c_str());
        celx.setTable("radius", (lua_Number)body->getRadius());
        
        // TODO: add method to return semiaxes
        Vec3f semiAxes = body->getSemiAxes();
        // Note: oblateness is an obsolete field, replaced by semiaxes;
        // it's only here for backward compatibility.
        float polarRadius = semiAxes.y;
        float eqRadius = max(semiAxes.x, semiAxes.z);
        celx.setTable("oblateness", (eqRadius - polarRadius) / eqRadius);
        
        double lifespanStart, lifespanEnd;
        body->getLifespan(lifespanStart, lifespanEnd);
        celx.setTable("lifespanStart", (lua_Number)lifespanStart);
        celx.setTable("lifespanEnd", (lua_Number)lifespanEnd);
        // TODO: atmosphere, surfaces ?
        
        PlanetarySystem* system = body->getSystem();
        if (system->getPrimaryBody() != NULL)
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
        
        lua_pushstring(l, "hasRings");
        lua_pushboolean(l, body->getRings() != NULL);
        lua_settable(l, -3);
        
        // TIMELINE-TODO: The code to retrieve orbital and rotation periods only works
        // if the object has a single timeline phase. This should hardly ever
        // be a problem, but it still may be best to set the periods to zero
        // for objects with multiple phases.
        const RotationModel* rm = body->getRotationModel(0.0);
        celx.setTable("rotationPeriod", (double) rm->getPeriod());
        
        const Orbit* orbit = body->getOrbit(0.0);
        celx.setTable("orbitPeriod", orbit->getPeriod());
        Atmosphere* atmosphere = body->getAtmosphere();
        if (atmosphere != NULL)
        {
            celx.setTable("atmosphereHeight", (double)atmosphere->height);
            celx.setTable("atmosphereCloudHeight", (double)atmosphere->cloudHeight);
            celx.setTable("atmosphereCloudSpeed", (double)atmosphere->cloudSpeed);
        }
    }
    else if (sel->deepsky() != NULL)
    {
        DeepSkyObject* deepsky = sel->deepsky();
        const char* objTypeName = deepsky->getObjTypeName();
        celx.setTable("type", objTypeName);
        
        celx.setTable("name", celx.appCore(AllErrors)->getSimulation()->getUniverse()
                 ->getDSOCatalog()->getDSOName(deepsky).c_str());
        celx.setTable("catalogNumber", deepsky->getCatalogNumber());
        
        if (!strcmp(objTypeName, "galaxy"))
            celx.setTable("hubbleType", deepsky->getType());
        
        celx.setTable("absoluteMagnitude", (lua_Number)deepsky->getAbsoluteMagnitude());
        celx.setTable("radius", (lua_Number)deepsky->getRadius());
    }
    else if (sel->location() != NULL)
    {
        celx.setTable("type", "location");
        Location* location = sel->location();
        celx.setTable("name", location->getName().c_str());
        celx.setTable("size", (lua_Number)location->getSize());
        celx.setTable("importance", (lua_Number)location->getImportance());
        celx.setTable("infoURL", location->getInfoURL().c_str());
        
        uint32 featureType = location->getFeatureType();
        string featureName("Unknown");
        for (CelxLua::FlagMap::const_iterator it = CelxLua::LocationFlagMap.begin();
             it != CelxLua::LocationFlagMap.end(); it++)
        {
            if (it->second == featureType)
            {
                featureName = it->first;
                break;
            }
        }
        celx.setTable("featureType", featureName.c_str());
        
        Body* parent = location->getParentBody();
        if (parent != NULL)
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
    if (sel->star() != NULL)
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
    if (colorString != NULL)
        Color::parse(colorString, markColor);
    
    MarkerRepresentation::Symbol markSymbol = MarkerRepresentation::Diamond;
    const char* markerString = celx.safeGetString(3, WrongType, "Second argument to object:mark must be a string");
    if (markerString != NULL)
        markSymbol = parseMarkerSymbol(markerString);
    
    float markSize = (float)celx.safeGetNumber(4, WrongType, "Third arg to object:mark must be a number", 10.0);
    if (markSize < 1.0f)
        markSize = 1.0f;
    else if (markSize > 10000.0f)
        markSize = 10000.0f;
    
    float markAlpha = (float)celx.safeGetNumber(5, WrongType, "Fourth arg to object:mark must be a number", 0.9);
    if (markAlpha < 0.0f)
        markAlpha = 0.0f;
    else if (markAlpha > 1.0f)
        markAlpha = 1.0f;
    
    Color markColorAlpha(0.0f, 1.0f, 0.0f, 0.9f);
    markColorAlpha = Color(markColor, markAlpha);
    
    const char* markLabel = celx.safeGetString(6, WrongType, "Fifth argument to object:mark must be a string");
    if (markLabel == NULL)
        markLabel = "";
    
    bool occludable = celx.safeGetBoolean(7, WrongType, "Sixth argument to object:mark must be a boolean", true);	
    
    Simulation* sim = appCore->getSimulation();
    
    MarkerRepresentation markerRep(markSymbol);
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
    if (sel->star() != NULL)
    {
        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel->star()->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            for (int i = 0; i < solarSys->getPlanets()->getSystemSize(); i++)
            {
                Body* body = solarSys->getPlanets()->getBody(i);
                Selection satSel(body);
                object_new(l, satSel);
                lua_rawseti(l, -2, i + 1);
            }
        }
    }
    else if (sel->body() != NULL)
    {
        const PlanetarySystem* satellites = sel->body()->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
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

    if (sel->body() != NULL && renderer != NULL)
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
	StarDatabase::Catalog catalog = StarDatabase::HenryDraper;
	if (catalogName != NULL)
	{
		if (compareIgnoringCase(catalogName, "HD") == 0)
		{
			catalog = StarDatabase::HenryDraper;
			validCatalog = true;
		}
		else if (compareIgnoringCase(catalogName, "SAO") == 0)
		{
			catalog = StarDatabase::SAO;
			validCatalog = true;
		}
		else if (compareIgnoringCase(catalogName, "HIP") == 0)
		{
			useHIPPARCOS = true;
			validCatalog = true;
		}
	}
    
	uint32 catalogNumber = Star::InvalidCatalogNumber;
	if (sel->star() != NULL && validCatalog)
	{
		uint32 internalNumber = sel->star()->getCatalogNumber();
        
		if (useHIPPARCOS)
		{
			// Celestia's internal catalog numbers /are/ HIPPARCOS numbers
			if (internalNumber < StarDatabase::MAX_HIPPARCOS_NUMBER)
				catalogNumber = internalNumber;
		}
		else
		{
			const StarDatabase* stardb = appCore->getSimulation()->getUniverse()->getStarCatalog();
			catalogNumber = stardb->crossIndex(catalog, internalNumber);
		}
	}
    
	if (catalogNumber != Star::InvalidCatalogNumber)
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
    if (sel == NULL)
    {
        celx.doError("Bad object!");
        return 0;
    }
    
    // Get the current counter value
    uint32 i = (uint32) lua_tonumber(l, lua_upvalueindex(2));
    
    vector<Location*>* locations = NULL;
    if (sel->body() != NULL)
    {
        locations = sel->body()->getLocations();
    }
    
    if (locations != NULL && i < locations->size())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));
        
        Location* loc = locations->at(i);
        if (loc == NULL)
            lua_pushnil(l);
        else
            object_new(l, Selection(loc));
        
        return 1;
    }
    else
    {
        // Return nil when we've enumerated all the locations (or if
        // there were no locations associated with the object.)
        return 0;
    }
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
    celx.newFrame(ObserverFrame(ObserverFrame::BodyFixed, *sel));
    
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
    celx.newFrame(ObserverFrame(ObserverFrame::Equatorial, *sel));
    
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
    
    if (sel->body() == NULL)
    {
        // The default universal frame
        celx.newFrame(ObserverFrame());
    }
    else 
    {
        const ReferenceFrame* f = sel->body()->getOrbitFrame(t);
        celx.newFrame(ObserverFrame(*f));
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
* -- Example: get the curren body frame for the International Space Station.
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
    
    if (sel->body() == NULL)
    {
        // The default universal frame
        celx.newFrame(ObserverFrame());
    }
    else 
    {
        const ReferenceFrame* f = sel->body()->getBodyFrame(t);
        celx.newFrame(ObserverFrame(*f));
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
    
    if (sel->body() == NULL)
    {
        lua_pushnil(l);
    }
    else 
    {
        const Timeline* timeline = sel->body()->getTimeline();
        if (timeline->includes(t))
        {
            celx.newPhase(*timeline->findPhase(t));
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
    if (sel == NULL)
    {
        celx.doError("Bad object!");
        return 0;
    }
    
    // Get the current counter value
    uint32 i = (uint32) lua_tonumber(l, lua_upvalueindex(2));
    
    const Timeline* timeline = NULL;
    if (sel->body() != NULL)
    {
        timeline = sel->body()->getTimeline();
    }
    
    if (timeline != NULL && i < timeline->phaseCount())
    {
        // Increment the counter
        lua_pushnumber(l, i + 1);
        lua_replace(l, lua_upvalueindex(2));
        
        const TimelinePhase* phase = timeline->getPhase(i);
        celx.newPhase(*phase);
        
        return 1;
    }
    else
    {
        // Return nil when we've enumerated all the phases (or if
        // if the object wasn't a solar system body.)
        return 0;
    }
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
    
    lua_pop(l, 1); // pop metatable off the stack
}


// ==================== object extensions ====================

// TODO: This should be replaced by an actual Atmosphere object
static int object_setatmosphere(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(23, 23, "22 arguments (!) expected to function object:setatmosphere");
    
    Selection* sel = this_object(l);
    //CelestiaCore* appCore = getAppCore(l, AllErrors);
    
    if (sel->body() != NULL)
    {
        Body* body = sel->body();
        Atmosphere* atmosphere = body->getAtmosphere();
        if (atmosphere != NULL)
        {
            float r = (float) celx.safeGetNumber(2, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            float g = (float) celx.safeGetNumber(3, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            float b = (float) celx.safeGetNumber(4, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            //			Color testColor(0.0f, 1.0f, 0.0f);
            Color testColor(r, g, b);
            atmosphere->lowerColor = testColor;
            r = (float) celx.safeGetNumber(5, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            g = (float) celx.safeGetNumber(6, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            b = (float) celx.safeGetNumber(7, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->upperColor = Color(r, g, b);
            r = (float) celx.safeGetNumber(8, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            g = (float) celx.safeGetNumber(9, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            b = (float) celx.safeGetNumber(10, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->skyColor = Color(r, g, b);
            r = (float) celx.safeGetNumber(11, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            g = (float) celx.safeGetNumber(12, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            b = (float) celx.safeGetNumber(13, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->sunsetColor = Color(r, g, b);
            r = (float) celx.safeGetNumber(14, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            g = (float) celx.safeGetNumber(15, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            b = (float) celx.safeGetNumber(16, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            //HWR			atmosphere->rayleighCoeff = Vector3(r, g, b);
            r = (float) celx.safeGetNumber(17, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            g = (float) celx.safeGetNumber(18, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            b = (float) celx.safeGetNumber(19, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            //HWR			atmosphere->absorptionCoeff = Vector3(r, g, b);
            b = (float) celx.safeGetNumber(20, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->mieCoeff = b;
            b = (float) celx.safeGetNumber(21, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->mieScaleHeight = b;
            b = (float) celx.safeGetNumber(22, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->miePhaseAsymmetry = b;
            b = (float) celx.safeGetNumber(23, AllErrors, "Arguments to observer:setatmosphere() must be numbers");
            atmosphere->rayleighScaleHeight = b;
            
            body->setAtmosphere(*atmosphere);
            cout << "set atmosphere\n";
        }
    }
    
    return 0;
}

void ExtendObjectMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.pushClassName(Celx_Object);
    lua_rawget(l, LUA_REGISTRYINDEX);
    if (lua_type(l, -1) != LUA_TTABLE)
        cout << "Metatable for " << CelxLua::ClassNames[Celx_Object] << " not found!\n";
    celx.registerMethod("setatmosphere", object_setatmosphere);
	lua_pop(l, 1);
}
