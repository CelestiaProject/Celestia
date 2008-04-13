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


int rotation_new(lua_State* l, const Quatd& qd)
{
    CelxLua celx(l);
    
    Quatd* q = reinterpret_cast<Quatd*>(lua_newuserdata(l, sizeof(Quatd)));
    *q = qd;
    
    celx.setClass(Celx_Rotation);
    
    return 1;
}


Quatd* to_rotation(lua_State* l, int index)
{
    CelxLua celx(l);

    return static_cast<Quatd*>(celx.checkUserData(index, Celx_Rotation));
}


static Quatd* this_rotation(lua_State* l)
{
    CelxLua celx(l);

    Quatd* q = to_rotation(l, 1);
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
    Quatd* q1 = to_rotation(l, 1);
    Quatd* q2 = to_rotation(l, 2);
    if (q1 == NULL || q2 == NULL)
    {
        celx.doError("Addition only defined for two rotations");
    }
    else
    {
        Quatd result = *q1 + *q2;
        rotation_new(l, result);
    }
    return 1;
}


static int rotation_mult(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Need two operands for multiplication");
    Quatd* r1 = NULL;
    Quatd* r2 = NULL;
    //Vec3d* v = NULL;
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
            rotation_new(l, *r1 * s);
        }
    else
        if (lua_isnumber(l, 1) && celx.isType(2, Celx_Rotation))
        {
            s = lua_tonumber(l, 1);
            r1 = to_rotation(l, 2);
            rotation_new(l, *r1 * s);
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
    Quatd* q = this_rotation(l);
    vector_new(l, imag(*q));
    return 1;
}


static int rotation_real(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for rotation_real");
    Quatd* q = this_rotation(l);
    lua_pushnumber(l, real(*q));
    return 1;
}


static int rotation_transform(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected for rotation:transform()");
    Quatd* q = this_rotation(l);
    Vec3d* v = to_vector(l, 2);
    if (v == NULL)
    {
        celx.doError("Argument to rotation:transform() must be a vector");
    }
    vector_new(l, *v * q->toMatrix3());
    return 1;
}


static int rotation_setaxisangle(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for rotation:setaxisangle()");
    Quatd* q = this_rotation(l);
    Vec3d* v = to_vector(l, 2);
    if (v == NULL)
    {
        celx.doError("setaxisangle: first argument must be a vector");
    }
    double angle = celx.safeGetNumber(3, AllErrors, "second argument to rotation:setaxisangle must be a number");
    q->setAxisAngle(*v, angle);
    return 0;
}


static int rotation_slerp(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(3, 3, "Two arguments expected for rotation:slerp()");
    Quatd* q1 = this_rotation(l);
    Quatd* q2 = to_rotation(l, 2);
    if (q2 == NULL)
    {
        celx.doError("slerp: first argument must be a rotation");
    }
    double t = celx.safeGetNumber(3, AllErrors, "second argument to rotation:slerp must be a number");
    rotation_new(l, Quatd::slerp(*q1, *q2, t));
    return 1;
}


static int rotation_get(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "Invalid access of rotation-component");
    Quatd* q3 = this_rotation(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in rotation-access");
    double value = 0.0;
    if (key == "x")
        value = imag(*q3).x;
    else if (key == "y")
        value = imag(*q3).y;
    else if (key == "z")
        value = imag(*q3).z;
    else if (key == "w")
        value = real(*q3);
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
    Quatd* q3 = this_rotation(l);
    string key = celx.safeGetString(2, AllErrors, "Invalid key in rotation-access");
    double value = celx.safeGetNumber(3, AllErrors, "Rotation components must be numbers");
    Vec3d v = imag(*q3);
    double w = real(*q3);
    if (key == "x")
        v.x = value;
    else if (key == "y")
        v.y = value;
    else if (key == "z")
        v.z = value;
    else if (key == "w")
        w = value;
    else
    {
        celx.doError("Invalid key in rotation-access");
    }
    *q3 = Quatd(w, v);
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

