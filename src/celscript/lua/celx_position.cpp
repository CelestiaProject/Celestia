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
#include <Eigen/Geometry>


using namespace std;
using namespace Eigen;

// ==================== Position ====================
// a 128-bit per component universal coordinate
int position_new(lua_State* l, const UniversalCoord& uc)
{
    CelxLua celx(l);

    UniversalCoord* ud = static_cast<UniversalCoord*>(lua_newuserdata(l, sizeof(UniversalCoord)));
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
    if (uc == nullptr)
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
        value = static_cast<double>(uc->x);
    else if (key == "y")
        value = static_cast<double>(uc->y);
    else if (key == "z")
        value = static_cast<double>(uc->z);
    else
    {
        if (!lua_getmetatable(l, 1))
        {
            celx.doError("Internal error: couldn't get metatable");
            return 0;
        }

        lua_pushvalue(l, 2);
        lua_rawget(l, -2);
        return 1;
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
        uc->x = BigFix(value);
    else if (key == "y")
        uc->y = BigFix(value);
    else if (key == "z")
        uc->z = BigFix(value);
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
    x = static_cast<double>(uc->x);
    lua_pushnumber(l, x);

    return 1;
}


static int position_gety(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for position:gety()");

    UniversalCoord* uc = this_position(l);
    lua_Number y;
    y = static_cast<double>(uc->y);
    lua_pushnumber(l, y);

    return 1;
}


static int position_getz(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for position:getz()");

    UniversalCoord* uc = this_position(l);
    lua_Number z;
    z = static_cast<double>(uc->z);
    lua_pushnumber(l, z);

    return 1;
}


static int position_vectorto(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected to position:vectorto");

    UniversalCoord* uc = this_position(l);
    UniversalCoord* uc2 = to_position(l, 2);

    if (uc2 == nullptr)
    {
        celx.doError("Argument to position:vectorto must be a position");
        return 0;
    }

    celx.newVector(uc2->offsetFromUly(*uc));

    return 1;
}


static int position_orientationto(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for position:orientationto");

    UniversalCoord* src = this_position(l);
    UniversalCoord* target = to_position(l, 2);

    if (target == nullptr)
    {
        celx.doError("First argument to position:orientationto must be a position");
        return 1;
    }

    auto upd = celx.toVector(3);
    if (upd == nullptr)
    {
        celx.doError("Second argument to position:orientationto must be a vector");
        return 1;
    }

    Vector3d src2target = target->offsetFromKm(*src).normalized();
    Vector3d v = src2target.cross(*upd).normalized();
    Vector3d u = v.cross(src2target);
    Matrix3d m;
    m.row(0) = v; m.row(1) = u; m.row(2) = src2target * (-1);
    Quaterniond qd(m);
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
    if (uc2 == nullptr)
    {
        celx.doError("Position expected as argument to position:distanceto");
        return 0;
    }

    lua_pushnumber(l, uc2->offsetFromKm(*uc).norm());

    return 1;
}


static int position_add(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for addition");
    UniversalCoord* p1 = nullptr;
    UniversalCoord* p2 = nullptr;
    Vector3d* v2 = nullptr;

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
            celx.newPosition(p1->offsetUly(*v2));
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
    UniversalCoord* p1 = nullptr;
    UniversalCoord* p2 = nullptr;
    Vector3d* v2 = nullptr;

    if (celx.isType(1, Celx_Position) && celx.isType(2, Celx_Position))
    {
        p1 = celx.toPosition(1);
        p2 = celx.toPosition(2);
        celx.newVector(p1->offsetFromUly(*p2));
    }
    else
        if (celx.isType(1, Celx_Position) && celx.isType(2, Celx_Vec3))
        {
            p1 = celx.toPosition(1);
            v2 = celx.toVector(2);
            celx.newPosition(p1->offsetUly(*v2));
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
    if (uc == nullptr)
        return 0;

    auto v3d = celx.toVector(2);
    if (v3d == nullptr)
    {
        celx.doError("Vector expected as argument to position:addvector");
        return 0;
    }

    UniversalCoord ucnew = uc->offsetUly(*v3d);
    position_new(l, ucnew);
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
