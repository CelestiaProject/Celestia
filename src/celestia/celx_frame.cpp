// celx_frame.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: frame object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celx.h"
#include "celx_internal.h"
#include "celx_frame.h"
#include "celestiacore.h"
#include <celengine/observer.h>


int frame_new(lua_State* l, const ObserverFrame& f)
{
    CelxLua celx(l);
    
	// Use placement new to put the new frame in the userdata block.
	void* block = lua_newuserdata(l, sizeof(ObserverFrame));
	new (block) ObserverFrame(f);
    
    celx.setClass(Celx_Frame);
    
    return 1;
}


ObserverFrame* to_frame(lua_State* l, int index)
{
    CelxLua celx(l);
    
    return static_cast<ObserverFrame*>(celx.checkUserData(index, Celx_Frame));
}


static ObserverFrame* this_frame(lua_State* l)
{
    CelxLua celx(l);
    
    ObserverFrame* f = to_frame(l, 1);
    if (f == NULL)
    {
        celx.doError("Bad frame object!");
    }
    
    return f;
}


// Convert from frame coordinates to universal.
static int frame_from(lua_State* l)
{
    CelxLua celx(l);
    
    celx.checkArgs(2, 3, "Two or three arguments required for frame:from");
    
    ObserverFrame* frame = this_frame(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);
    
    UniversalCoord* uc = NULL;
    Quatd* q = NULL;
    double jd = 0.0;
    
    if (celx.isType(2, Celx_Position))
    {
        uc = celx.toPosition(2);
    }
    else if (celx.isType(2, Celx_Rotation))
    {
        q = celx.toRotation(2);
    }
    if (uc == NULL && q == NULL)
    {
        celx.doError("Position or rotation expected as second argument to frame:from()");
    }
    
    jd = celx.safeGetNumber(3, WrongType, "Second arg to frame:from must be a number", appCore->getSimulation()->getTime());
    
    if (uc != NULL)
    {
        UniversalCoord uc1 = frame->convertToUniversal(*uc, jd);
        celx.newPosition(uc1);
    }
    else
    {
        Quatd q1 = frame->convertToUniversal(*q, jd);
        celx.newRotation(q1);
    }
    
    return 1;
}

// Convert from universal to frame coordinates.
static int frame_to(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 3, "Two or three arguments required for frame:to");
    
    ObserverFrame* frame = this_frame(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);
    
    UniversalCoord* uc = NULL;
    Quatd* q = NULL;
    double jd = 0.0;
    
    if (celx.isType(2, Celx_Position))
    {
        uc = celx.toPosition(2);
    }
    else if (celx.isType(2, Celx_Rotation))
    {
        q = celx.toRotation(2);
    }
    
    if (uc == NULL && q == NULL)
    {
        celx.doError("Position or rotation expected as second argument to frame:to()");
    }
    
    jd = celx.safeGetNumber(3, WrongType, "Second arg to frame:to must be a number", appCore->getSimulation()->getTime());
    
    if (uc != NULL)
    {
        UniversalCoord uc1 = frame->convertFromUniversal(*uc, jd);
        celx.newPosition(uc1);
    }
    else
    {
        Quatd q1 = frame->convertFromUniversal(*q, jd);
        celx.newRotation(q1);
    }
    
    return 1;
}

static int frame_getrefobject(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for frame:getrefobject()");
    
    ObserverFrame* frame = this_frame(l);
    if (frame->getRefObject().getType() == Selection::Type_Nil)
    {
        celx.push(CelxValue());
    }
    else
    {
        celx.newObject(frame->getRefObject());
    }
    
    return 1;
}

static int frame_gettargetobject(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for frame:gettarget()");
    
    ObserverFrame* frame = this_frame(l);
    if (frame->getTargetObject().getType() == Selection::Type_Nil)
    {
        lua_pushnil(l);
    }
    else
    {
        celx.newObject(frame->getTargetObject());
    }
    return 1;
}

static int frame_getcoordinatesystem(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for frame:getcoordinatesystem()");
    
    ObserverFrame* frame = this_frame(l);
    string coordsys;
    switch (frame->getCoordinateSystem())
    {
        case ObserverFrame::Universal:
            coordsys = "universal"; break;
        case ObserverFrame::Ecliptical:
            coordsys = "ecliptic"; break;
        case ObserverFrame::Equatorial:
            coordsys = "equatorial"; break;
        case ObserverFrame::BodyFixed:
            coordsys = "bodyfixed"; break;
        case ObserverFrame::ObserverLocal:
            coordsys = "observer"; break;
        case ObserverFrame::PhaseLock:
            coordsys = "lock"; break;
        case ObserverFrame::Chase:
            coordsys = "chase"; break;
        default:
            coordsys = "invalid";
    }
    
    celx.push(coordsys.c_str());
    
    return 1;
}

static int frame_tostring(lua_State* l)
{
    CelxLua celx(l);
    
    // TODO: print out the actual information about the frame
    celx.push("[Frame]");
    
    return 1;
}


/*! Garbage collection metamethod frame objects.
*/
static int frame_gc(lua_State* l)
{
	ObserverFrame* frame = this_frame(l);
    
	// Explicitly call the destructor since the object was created with placement new
	frame->~ObserverFrame();
    
	return 0;
}


void CreateFrameMetaTable(lua_State* l)
{
    CelxLua celx(l);
    
    celx.createClassMetatable(Celx_Frame);
    
    celx.registerMethod("__tostring", frame_tostring);
	celx.registerMethod("__gc", frame_gc);
    celx.registerMethod("to", frame_to);
    celx.registerMethod("from", frame_from);
    celx.registerMethod("getcoordinatesystem", frame_getcoordinatesystem);
    celx.registerMethod("getrefobject", frame_getrefobject);
    celx.registerMethod("gettargetobject", frame_gettargetobject);
    
    lua_pop(l, 1); // remove metatable from stack
}

