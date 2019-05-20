// celx_rotation.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: rotation object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celx.h"
#include "celx_internal.h"
#include "celx_vector.h"
#include <celutil/align.h>

using namespace Eigen;

int rotation_new(lua_State* l, const Quaterniond& qd)
{
    CelxLua celx(l);
    auto q = reinterpret_cast<Quaterniond*>(lua_newuserdata(l, aligned_sizeof<Quaterniond>()));
    *aligned_addr(q) = qd;
    celx.setClass(Celx_Rotation);

    return 1;
}

Quaterniond* to_rotation(lua_State* l, int index)
{
    CelxLua celx(l);
    auto q = reinterpret_cast<Quaterniond*>(celx.checkUserData(index, Celx_Rotation));

    return aligned_addr(q);
}

static Quaterniond* this_rotation(lua_State* l)
{
    CelxLua celx(l);

    auto q = to_rotation(l, 1);

    if (q == nullptr)
    {
        celx.doError("Bad rotation object!");
    }

    return q;
}


static int rotation_add(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for add");
    auto q1 = to_rotation(l, 1);
    auto q2 = to_rotation(l, 2);
    if (q1 == nullptr || q2 == nullptr)
    {
        celx.doError("Addition only defined for two rotations");
    }
    else
    {
        Quaterniond result(q1->w() + q2->w(),
                           q1->x() + q2->x(),
                           q1->y() + q2->y(),
                           q1->z() + q2->z());
        rotation_new(l, result);
    }
    return 1;
}


static int rotation_mult(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for multiplication");
    Quaterniond* r1 = nullptr;
    Quaterniond* r2 = nullptr;
    lua_Number s = 0.0;
    if (celx.isType(1, Celx_Rotation) && celx.isType(2, Celx_Rotation))
    {
        r1 = to_rotation(l, 1);
        r2 = to_rotation(l, 2);
        rotation_new(l, *r1 * *r2);
    }
    else
        if (celx.isType(1, Celx_Rotation) && lua_isnumber(l, 2))
        {
            r1 = to_rotation(l, 1);
            s = lua_tonumber(l, 2);
            Quaterniond r(r1->w() * s, r1->x() * s, r1->y() * s, r1->x() * s);
            rotation_new(l, r);
        }
    else
        if (lua_isnumber(l, 1) && celx.isType(2, Celx_Rotation))
        {
            s = lua_tonumber(l, 1);
            r1 = to_rotation(l, 2);
            Quaterniond r(r1->w() * s, r1->x() * s, r1->y() * s, r1->x() * s);
            rotation_new(l, r);
        }
    else
    {
        celx.doError("Bad rotation multiplication!");
    }
    return 1;
}


static int rotation_imag(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for rotation_imag");
    auto q = this_rotation(l);
    vector_new(l, q->vec());
    return 1;
}


static int rotation_real(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for rotation_real");
    auto q = this_rotation(l);
    lua_pushnumber(l, q->w());
    return 1;
}


static int rotation_transform(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected for rotation:transform()");
    auto q = this_rotation(l);
    auto v = to_vector(l, 2);
    if (v == nullptr)
    {
        celx.doError("Argument to rotation:transform() must be a vector");
        return 0;
    }
    // XXX or transpose() instead of .adjoint()?
    vector_new(l, q->toRotationMatrix().adjoint() * (*v));
    return 1;
}


static int rotation_setaxisangle(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for rotation:setaxisangle()");
    auto q = this_rotation(l);
    auto v = to_vector(l, 2);
    if (v == nullptr)
    {
        celx.doError("setaxisangle: first argument must be a vector");
        return 0;
    }
    double angle = celx.safeGetNumber(3, AllErrors, "second argument to rotation:setaxisangle must be a number");
    *q = Quaterniond(AngleAxisd(angle, v->normalized()));
    return 0;
}


static int rotation_slerp(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for rotation:slerp()");
    auto q1 = this_rotation(l);
    auto q2 = to_rotation(l, 2);
    if (q2 == nullptr)
    {
        celx.doError("slerp: first argument must be a rotation");
        return 0;
    }
    double t = celx.safeGetNumber(3, AllErrors, "second argument to rotation:slerp must be a number");
    rotation_new(l, q1->slerp(t, *q2));
    return 1;
}


static int rotation_get(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Invalid access of rotation-component");
    auto q3 = this_rotation(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in rotation-access");
    double value = 0.0;
    if (key == "x")
        value = q3->x();
    else if (key == "y")
        value = q3->y();
    else if (key == "z")
        value = q3->z();
    else if (key == "w")
        value = q3->w();
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


static int rotation_set(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Invalid access of rotation-component");
    auto q3 = this_rotation(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in rotation-access");
    double value = celx.safeGetNumber(3, AllErrors, "Rotation components must be numbers");
    Vector3d v = q3->vec();
    double w = q3->w();
    if (key == "x")
        v.x() = value;
    else if (key == "y")
        v.y() = value;
    else if (key == "z")
        v.z() = value;
    else if (key == "w")
        w = value;
    else
    {
        celx.doError("Invalid key in rotation-access");
    }
    *q3 = Quaterniond(w, v.x(), v.y(), v.z());
    return 0;
}


static int rotation_tostring(lua_State* l)
{
    CelxLua celx(l);

    lua_pushstring(l, "[Rotation]");
    return 1;
}


void CreateRotationMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Rotation);

    celx.registerMethod("real", rotation_real);
    celx.registerMethod("imag", rotation_imag);
    celx.registerMethod("transform", rotation_transform);
    celx.registerMethod("setaxisangle", rotation_setaxisangle);
    celx.registerMethod("slerp", rotation_slerp);
    celx.registerMethod("__tostring", rotation_tostring);
    celx.registerMethod("__add", rotation_add);
    celx.registerMethod("__mul", rotation_mult);
    celx.registerMethod("__index", rotation_get);
    celx.registerMethod("__newindex", rotation_set);

    lua_pop(l, 1); // remove metatable from stack
}

