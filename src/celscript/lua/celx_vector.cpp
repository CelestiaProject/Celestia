// celx_vector.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: vector object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celx.h"
#include "celx_internal.h"
#include "celx_vector.h"
#include <celutil/align.h>
#include <Eigen/Geometry>

using namespace std;
using namespace Eigen;

namespace util = celestia::util;

int vector_new(lua_State* l, const Vector3d& v)
{
    CelxLua celx(l);

    constexpr std::size_t size = util::aligned_sizeof<Vector3d>();
    auto v3 = util::aligned_addr<Vector3d>(lua_newuserdata(l, size));
    *v3 = v;
    celx.setClass(Celx_Vec3);

    return 1;
}

Vector3d* to_vector(lua_State* l, int index)
{
    CelxLua celx(l);
    return util::aligned_addr<Vector3d>(celx.checkUserData(index, Celx_Vec3));
}

static Vector3d* this_vector(lua_State* l)
{
    CelxLua celx(l);

    auto v3 = to_vector(l, 1);
    if (v3 == nullptr)
    {
        celx.doError("Bad vector object!");
    }

    return v3;
}


static int vector_sub(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for sub");
    auto op1 = celx.toVector(1);
    auto op2 = celx.toVector(2);
    if (op1 == nullptr || op2 == nullptr)
    {
        celx.doError("Subtraction only defined for two vectors");
    }
    else
    {
        auto result = *op1 - *op2;
        celx.newVector(result);
    }
    return 1;
}

static int vector_get(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Invalid access of vector-component");
    auto v3 = this_vector(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in vector-access");
    double value = 0.0;
    if (key == "x")
        value = v3->x();
    else if (key == "y")
        value = v3->y();
    else if (key == "z")
        value = v3->z();
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

static int vector_set(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Invalid access of vector-component");
    auto v3 = this_vector(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in vector-access");
    double value = celx.safeGetNumber(3, AllErrors, "Vector components must be numbers");
    if (key == "x")
        v3->x() = value;
    else if (key == "y")
        v3->y() = value;
    else if (key == "z")
        v3->z() = value;
    else
    {
        celx.doError("Invalid key in vector-access");
    }
    return 0;
}

static int vector_getx(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for vector:getx");
    auto v3 = this_vector(l);
    lua_Number x;
    x = static_cast<lua_Number>(v3->x());
    lua_pushnumber(l, x);

    return 1;
}

static int vector_gety(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for vector:gety");
    auto v3 = this_vector(l);
    lua_Number y;
    y = static_cast<lua_Number>(v3->y());
    lua_pushnumber(l, y);

    return 1;
}

static int vector_getz(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for vector:getz");
    auto v3 = this_vector(l);
    lua_Number z;
    z = static_cast<lua_Number>(v3->z());
    lua_pushnumber(l, z);

    return 1;
}

static int vector_normalize(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for vector:normalize");
    auto v = this_vector(l);
    celx.newVector(v->normalized());
    return 1;
}

static int vector_length(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for vector:length");
    auto v = this_vector(l);
    double length = v->norm();
    lua_pushnumber(l, (lua_Number)length);
    return 1;
}

static int vector_add(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for addition");
    Vector3d* v1 = nullptr;
    Vector3d* v2 = nullptr;
    UniversalCoord* p = nullptr;

    if (celx.isType(1, Celx_Vec3) && celx.isType(2, Celx_Vec3))
    {
        v1 = celx.toVector(1);
        v2 = celx.toVector(2);
        celx.newVector(*v1 + *v2);
    }
    else
        if (celx.isType(1, Celx_Vec3) && celx.isType(2, Celx_Position))
        {
            v1 = celx.toVector(1);
            p = celx.toPosition(2);
            celx.newPosition(p->offsetUly(*v1));
        }
    else
    {
        celx.doError("Bad vector addition!");
    }
    return 1;
}

static int vector_mult(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for multiplication");
    Vector3d* v1 = nullptr;
    Vector3d* v2 = nullptr;
    Quaterniond* q = nullptr;
    lua_Number s = 0.0;
    if (celx.isType(1, Celx_Vec3) && celx.isType(2, Celx_Vec3))
    {
        v1 = celx.toVector(1);
        v2 = celx.toVector(2);
        lua_pushnumber(l, (lua_Number)v1->dot(*v2));
    }
    else
        if (celx.isType(1, Celx_Vec3) && lua_isnumber(l, 2))
        {
            v1 = celx.toVector(1);
            s = lua_tonumber(l, 2);
            celx.newVector(*v1 * s);
        }
    else
        if (celx.isType(1, Celx_Vec3) && celx.isType(2, Celx_Rotation))
        {
            v1 = celx.toVector(1);
            q = celx.toRotation(2);
            celx.newRotation(Quaterniond(0, v1->x(), v1->y(), v1->z()) * *q);
        }
    else
        if (lua_isnumber(l, 1) && celx.isType(2, Celx_Vec3))
        {
            s = lua_tonumber(l, 1);
            v1 = celx.toVector(2);
            celx.newVector(*v1 * s);
        }
    else
    {
        celx.doError("Bad vector multiplication!");
    }
    return 1;
}

static int vector_cross(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for multiplication");
    Vector3d* v1 = nullptr;
    Vector3d* v2 = nullptr;
    if (celx.isType(1, Celx_Vec3) && celx.isType(2, Celx_Vec3))
    {
        v1 = celx.toVector(1);
        v2 = celx.toVector(2);
        celx.newVector(v1->cross(*v2));
    }
    else
    {
        celx.doError("Bad vector multiplication!");
    }
    return 1;

}

static int vector_tostring(lua_State* l)
{
    lua_pushstring(l, "[Vector]");
    return 1;
}

void CreateVectorMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Vec3);

    celx.registerMethod("__tostring", vector_tostring);
    celx.registerMethod("__add", vector_add);
    celx.registerMethod("__sub", vector_sub);
    celx.registerMethod("__mul", vector_mult);
    celx.registerMethod("__pow", vector_cross);
    celx.registerMethod("__index", vector_get);
    celx.registerMethod("__newindex", vector_set);
    celx.registerMethod("getx", vector_getx);
    celx.registerMethod("gety", vector_gety);
    celx.registerMethod("getz", vector_getz);
    celx.registerMethod("normalize", vector_normalize);
    celx.registerMethod("length", vector_length);

    lua_pop(l, 1); // remove metatable from stack
}


