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

#ifndef __CELVEC__
using namespace Eigen;
#endif

#ifdef __CELVEC__
int rotation_new(lua_State* l, const Quatd& qd)
#else
int rotation_new(lua_State* l, const Quaterniond& qd)
#endif
{
    CelxLua celx(l);

#ifdef __CELVEC__
    Quatd* q = reinterpret_cast<Quatd*>(lua_newuserdata(l, sizeof(Quatd)));
#else
    auto q = reinterpret_cast<Quaterniond*>(lua_newuserdata(l, sizeof(Quaterniond)));
#endif

    *q = qd;

    celx.setClass(Celx_Rotation);

    return 1;
}

#ifdef __CELVEC__
Quatd* to_rotation(lua_State* l, int index)
#else
Quaterniond* to_rotation(lua_State* l, int index)
#endif
{
    CelxLua celx(l);

#ifdef __CELVEC__
    return static_cast<Quatd*>(celx.checkUserData(index, Celx_Rotation));
#else
    return static_cast<Quaterniond*>(celx.checkUserData(index, Celx_Rotation));
#endif
}

#ifdef __CELVEC__
static Quatd* this_rotation(lua_State* l)
#else
static Quaterniond* this_rotation(lua_State* l)
#endif
{
    CelxLua celx(l);

    auto q = to_rotation(l, 1);

    if (q == NULL)
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
    if (q1 == NULL || q2 == NULL)
    {
        celx.doError("Addition only defined for two rotations");
    }
    else
    {
#ifdef __CELVEC__
        auto result = *q1 + *q2;
#else
        Quaterniond result(q1->w() + q2->w(),
                           q1->x() + q2->x(),
                           q1->y() + q2->y(),
                           q1->z() + q2->z());
#endif
        rotation_new(l, result);
    }
    return 1;
}


static int rotation_mult(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for multiplication");
#ifdef __CELVEC__
    Quatd* r1 = NULL;
    Quatd* r2 = NULL;
#else
    Quaterniond* r1 = nullptr;
    Quaterniond* r2 = nullptr;
#endif
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
#ifdef __CELVEC__
            rotation_new(l, *r1 * s);
#else
            Quaterniond r(r1->w() * s, r1->x() * s, r1->y() * s, r1->x() * s);
            rotation_new(l, r);
#endif
        }
    else
        if (lua_isnumber(l, 1) && celx.isType(2, Celx_Rotation))
        {
            s = lua_tonumber(l, 1);
            r1 = to_rotation(l, 2);
#ifdef __CELVEC__
            rotation_new(l, *r1 * s);
#else
            Quaterniond r(r1->w() * s, r1->x() * s, r1->y() * s, r1->x() * s);
            rotation_new(l, r);
#endif
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
#ifdef __CELVEC__
    vector_new(l, imag(*q));
#else
    vector_new(l, q->vec());
#endif
    return 1;
}


static int rotation_real(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for rotation_real");
    auto q = this_rotation(l);
#ifdef __CELVEC__
    lua_pushnumber(l, real(*q));
#else
    lua_pushnumber(l, q->w());
#endif
    return 1;
}


static int rotation_transform(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected for rotation:transform()");
    auto q = this_rotation(l);
    auto v = to_vector(l, 2);
    if (v == NULL)
    {
        celx.doError("Argument to rotation:transform() must be a vector");
        return 0;
    }
#ifdef __CELVEC__
    vector_new(l, *v * q->toMatrix3());
#else
    // XXX or transpose() instead of .adjoint()?
    vector_new(l, q->toRotationMatrix().adjoint() * (*v));
#endif
    return 1;
}


static int rotation_setaxisangle(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for rotation:setaxisangle()");
    auto q = this_rotation(l);
    auto v = to_vector(l, 2);
    if (v == NULL)
    {
        celx.doError("setaxisangle: first argument must be a vector");
        return 0;
    }
    double angle = celx.safeGetNumber(3, AllErrors, "second argument to rotation:setaxisangle must be a number");
#ifdef __CELVEC__
    q->setAxisAngle(*v, angle);
#else
    *q = Quaterniond(AngleAxisd(angle, v->normalized()));
#endif
    return 0;
}


static int rotation_slerp(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for rotation:slerp()");
    auto q1 = this_rotation(l);
    auto q2 = to_rotation(l, 2);
    if (q2 == NULL)
    {
        celx.doError("slerp: first argument must be a rotation");
        return 0;
    }
    double t = celx.safeGetNumber(3, AllErrors, "second argument to rotation:slerp must be a number");
#ifdef __CELVEC__
    rotation_new(l, Quatd::slerp(*q1, *q2, t));
#else
    rotation_new(l, q1->slerp(t, *q2));
#endif
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
#ifdef __CELVEC__
        value = imag(*q3).x;
#else
        value = q3->x();
#endif
    else if (key == "y")
#ifdef __CELVEC__
        value = imag(*q3).y;
#else
        value = q3->y();
#endif
    else if (key == "z")
#ifdef __CELVEC__
        value = imag(*q3).z;
#else
        value = q3->z();
#endif
    else if (key == "w")
#ifdef __CELVEC__
        value = real(*q3);
#else
        value = q3->w();
#endif
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


static int rotation_set(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Invalid access of rotation-component");
    auto q3 = this_rotation(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in rotation-access");
    double value = celx.safeGetNumber(3, AllErrors, "Rotation components must be numbers");
#ifdef __CELVEC__
    Vec3d v = imag(*q3);
    double w = real(*q3);
#else
    Vector3d v = q3->vec();
    double w = q3->w();
#endif
    if (key == "x")
#ifdef __CELVEC__
        v.x = value;
#else
        v.x() = value;
#endif
    else if (key == "y")
#ifdef __CELVEC__
        v.y = value;
#else
        v.y() = value;
#endif
    else if (key == "z")
#ifdef __CELVEC__
        v.z = value;
#else
        v.z() = value;
#endif
    else if (key == "w")
        w = value;
    else
    {
        celx.doError("Invalid key in rotation-access");
    }
#ifdef __CELVEC__
    *q3 = Quatd(w, v);
#else
    *q3 = Quaterniond(w, v.x(), v.y(), v.z());
#endif
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

