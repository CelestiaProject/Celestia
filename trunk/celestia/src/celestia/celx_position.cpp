// celx_position.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: position object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celx.h"
#include "celx_internal.h"
#include "celx_position.h"


// ==================== Position ====================
// a 128-bit per component universal coordinate
int position_new(lua_State* l, const UniversalCoord& uc)
{
    CelxLua celx(l);

    UniversalCoord* ud = reinterpret_cast<UniversalCoord*>(lua_newuserdata(l, sizeof(UniversalCoord)));
    *ud = uc;
    
    celx.setClass(Celx_Position);
    
    return 1;
}

UniversalCoord* to_position(lua_State* l, int index)
{
    CelxLua celx(l);

    return static_cast<UniversalCoord*>(celx.checkUserData(index, Celx_Position));
}


static UniversalCoord* this_position(lua_State* l)
{
    CelxLua celx(l);

    UniversalCoord* uc = to_position(l, 1);
    if (uc == NULL)
    {
        celx.doError("Bad position object!");
    }
    
    return uc;
}


static int position_get(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Invalid access of position-component");
    UniversalCoord* uc = this_position(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in position-access");
    double value = 0.0;
    if (key == "x")
        value = uc->x;
    else if (key == "y")
        value = uc->y;
    else if (key == "z")
        value = uc->z;
    else
    {
        if (lua_getmetatable(l, 1))
        {
            lua_pushvalue(l, 2);
            lua_rawget(l, -2);
            return 1;
        }
        else
        {
            celx.doError("Internal error: couldn't get metatable");
        }
    }
    lua_pushnumber(l, (lua_Number)value);
    return 1;
}


static int position_set(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Invalid access of position-component");
    UniversalCoord* uc = this_position(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in position-access");
    double value = celx.safeGetNumber(3, AllErrors, "Position components must be numbers");
    if (key == "x")
        uc->x = value;
    else if (key == "y")
        uc->y = value;
    else if (key == "z")
        uc->z = value;
    else
    {
        celx.doError("Invalid key in position-access");
    }
    return 0;
}


static int position_getx(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for position:getx()");
    
    UniversalCoord* uc = this_position(l);
    lua_Number x;
    x = uc->x;
    lua_pushnumber(l, x);
    
    return 1;
}


static int position_gety(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for position:gety()");
    
    UniversalCoord* uc = this_position(l);
    lua_Number y;
    y = uc->y;
    lua_pushnumber(l, y);
    
    return 1;
}


static int position_getz(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for position:getz()");
    
    UniversalCoord* uc = this_position(l);
    lua_Number z;
    z = uc->z;
    lua_pushnumber(l, z);
    
    return 1;
}


static int position_vectorto(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected to position:vectorto");
    
    UniversalCoord* uc = this_position(l);
    UniversalCoord* uc2 = to_position(l, 2);
    
    if (uc2 == NULL)
    {
        celx.doError("Argument to position:vectorto must be a position");
    }

    celx.newVector(*uc2 - *uc);

    return 1;
}


static int position_orientationto(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for position:orientationto");
    
    UniversalCoord* src = this_position(l);
    UniversalCoord* target = to_position(l, 2);
    
    if (target == NULL)
    {
        celx.doError("First argument to position:orientationto must be a position");
    }
    
    Vec3d* upd = celx.toVector(3);
    if (upd == NULL)
    {
        celx.doError("Second argument to position:orientationto must be a vector");
    }
    
    Vec3d src2target = *target - *src;
    src2target.normalize();
    Vec3d v = src2target ^ *upd;
    v.normalize();
    Vec3d u = v ^ src2target;
    Quatd qd = Quatd(Mat3d(v, u, -src2target));
    celx.newRotation(qd);
    
    return 1;
}


static int position_tostring(lua_State* l)
{
    // TODO: print out the coordinate as it would appear in a cel:// URL
    lua_pushstring(l, "[Position]");
    
    return 1;
}


static int position_distanceto(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected to position:distanceto()");
    
    UniversalCoord* uc = this_position(l);
    UniversalCoord* uc2 = to_position(l, 2);
    if (uc2 == NULL)
    {
        celx.doError("Position expected as argument to position:distanceto");
    }
    
    Vec3d v = *uc2 - *uc;
    lua_pushnumber(l, astro::microLightYearsToKilometers(v.length()));
    
    return 1;
}


static int position_add(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for addition");
    UniversalCoord* p1 = NULL;
    UniversalCoord* p2 = NULL;
    Vec3d* v2 = NULL;
    
    if (celx.isType(1, Celx_Position) && celx.isType(2, Celx_Position))
    {
        p1 = celx.toPosition(1);
        p2 = celx.toPosition(2);
        // this is not very intuitive, as p1-p2 is a vector
        celx.newPosition(*p1 + *p2);
    }
    else
        if (celx.isType(1, Celx_Position) && celx.isType(2, Celx_Vec3))
        {
            p1 = celx.toPosition(1);
            v2 = celx.toVector(2);
            celx.newPosition(*p1 + *v2);
        }
    else
    {
        celx.doError("Bad position addition!");
    }
    return 1;
}


static int position_sub(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for subtraction");
    UniversalCoord* p1 = NULL;
    UniversalCoord* p2 = NULL;
    Vec3d* v2 = NULL;
    
    if (celx.isType(1, Celx_Position) && celx.isType(2, Celx_Position))
    {
        p1 = celx.toPosition(1);
        p2 = celx.toPosition(2);
        celx.newVector(*p1 - *p2);
    }
    else
        if (celx.isType(1, Celx_Position) && celx.isType(2, Celx_Vec3))
        {
            p1 = celx.toPosition(1);
            v2 = celx.toVector(2);
            celx.newPosition(*p1 - *v2);
        }
    else
    {
        celx.doError("Bad position subtraction!");
    }
    return 1;
}


static int position_addvector(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected to position:addvector()");
    UniversalCoord* uc = this_position(l);
    Vec3d* v3d = celx.toVector(2);
    if (v3d == NULL)
    {
        celx.doError("Vector expected as argument to position:addvector");
    }
    else
        if (uc != NULL && v3d != NULL)
        {
            UniversalCoord ucnew = *uc + *v3d;
            position_new(l, ucnew);
        }
    return 1;
}


void CreatePositionMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Position);
    
    celx.registerMethod("__tostring", position_tostring);
    celx.registerMethod("distanceto", position_distanceto);
    celx.registerMethod("vectorto", position_vectorto);
    celx.registerMethod("orientationto", position_orientationto);
    celx.registerMethod("addvector", position_addvector);
    celx.registerMethod("__add", position_add);
    celx.registerMethod("__sub", position_sub);
    celx.registerMethod("__index", position_get);
    celx.registerMethod("__newindex", position_set);
    celx.registerMethod("getx", position_getx);
    celx.registerMethod("gety", position_gety);
    celx.registerMethod("getz", position_getz);
    
    lua_pop(l, 1); // remove metatable from stack
}
