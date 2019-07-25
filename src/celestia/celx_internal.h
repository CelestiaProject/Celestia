// celx_internal.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia. Internals that should only
// be needed by modules that implement a celx object.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_INTERNAL_H_
#define _CELX_INTERNAL_H_

#include <memory>
#include <map>
#include <string>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <celutil/color.h>
#include <celengine/parser.h>
#include <celengine/timelinephase.h>
#include "celx.h"

class CelestiaCore;

enum
{
    Celx_Celestia = 0,
    Celx_Observer = 1,
    Celx_Object   = 2,
    Celx_Vec3     = 3,
    Celx_Matrix   = 4,
    Celx_Rotation = 5,
    Celx_Position = 6,
    Celx_Frame    = 7,
    Celx_CelScript= 8,
    Celx_Font     = 9,
    Celx_Image    = 10,
    Celx_Texture  = 11,
    Celx_Phase    = 12,
    Celx_Category = 13
};


// select which type of error will be fatal (call lua_error) and
// which will return a default value instead
enum FatalErrors
{
    NoErrors   = 0,
    WrongType  = 1,
    WrongArgc  = 2,
    AllErrors = WrongType | WrongArgc,
};


class CelxLua;

class CelxValue
{
public:
    enum CelxType
    {
        Celx_Number,
        Celx_String,
        Celx_Nil,
    };

    CelxValue() : type(Celx_Nil) {}
    CelxValue(double d) : type(Celx_Number), value_number(d) {}
    CelxValue(const char* s) : type(Celx_String), value_cstring(s) {}

    void push(lua_State* l) const
    {
        switch (type)
        {
            case Celx_Number: lua_pushnumber(l, value_number); break;
            case Celx_String: lua_pushstring(l, value_cstring); break;
            case Celx_Nil: lua_pushnil(l); break;
        }
    }

private:
    CelxType type;
    union
    {
        double value_number;
        char* value_string;
        const char* value_cstring;
    };
};


class CelxLua
{
public:
    CelxLua(lua_State* l);
    ~CelxLua() = default;

    static int localIndex(int n) { return lua_upvalueindex(n); }

    /**** push-on-stack methods ****/

    int push()
    {
        lua_pushnil(m_lua);
        return 1;
    }
    int push(bool a)
    {
        lua_pushboolean(m_lua, a);
        return 1;
    }
    int push(int a)
    {
        lua_pushinteger(m_lua, a);
        return 1;
    }
    int push(float a)
    {
        lua_pushnumber(m_lua, a);
        return 1;
    }
    int push(double a)
    {
        lua_pushnumber(m_lua, a);
        return 1;
    }
    int push(const char *a)
    {
        lua_pushstring(m_lua, a);
        return 1;
    }
    int push(int cc(lua_State*), int n)
    {
        lua_pushcclosure(m_lua, cc, n);
        return 1;
    }

    template <typename T> T *newUserData()
    {
        return reinterpret_cast<T*>(lua_newuserdata(m_lua, sizeof(T)));
    }
    template <typename T> T *newUserData(T a)
    {
        T *p = newUserData<T>();
        if (p != nullptr)
            new (p) T(a);
        return p;
    }
    template <typename T> T *newUserDataArray(int n)
    {
        return reinterpret_cast<T*>(lua_newuserdata(m_lua, sizeof(T) * n));
    }
    template <typename T> T *newUserDataArray(T *a, int n)
    {
        T *p = reinterpret_cast<T*>(lua_newuserdata(m_lua, sizeof(T) * n));
        std::copy(a, a + n, p);
        return p;
    }
    template <typename T> int pushClass(T a)
    {
        newUserData(a);
        setClass(celxClassId(a));
        return 1;
    }

    template<typename T> static int iterator(lua_State *l)
    {
        CelxLua celx(l);

        T *c = celx.getUserData<T>(CelxLua::localIndex(1));
        int i = static_cast<int>(celx.getNumber(CelxLua::localIndex(2)));
        if (i < 0)
            return 0;
        T ret = c[i];
        i--;
        celx.push(i);
        lua_replace(l, CelxLua::localIndex(2));
        return celx.push(ret);
    };
    template<typename T> static int classIterator(lua_State *l)
    {
        CelxLua celx(l);

        T *c = celx.getUserData<T>(CelxLua::localIndex(1));
        int i = static_cast<int>(celx.getNumber(CelxLua::localIndex(2)));
        if (i < 0)
            return 0;
        T ret = c[i];
        i--;
        celx.push(i);
        lua_replace(l, CelxLua::localIndex(2));
        return celx.pushClass(ret);
    };
    template<typename V, typename K> static V value(std::pair<const K, V> &v)
    {
        return v.second;
    }
    template<typename V> static V value(V it)
    {
        return it;
    }
    template<typename T, typename C> int pushIterable(C& a)
    {
        CelxLua celx(m_lua);
        int n = a.size();
        T *array = celx.newUserDataArray<T>(n);
        for (auto &it : a)
        {
            *array = value(it);
            array++;
        }
        celx.push(n - 1);
        celx.push(iterator<T>, 2);

        return 1;
    }
    template<typename T, typename C> int pushClassIterable(C& a)
    {
        CelxLua celx(m_lua);
        int n = a.size();
        T *array = celx.newUserDataArray<T>(n);
        for (auto &it : a)
        {
            *array = value(it);
            array++;
        }
        celx.push(n - 1);
        celx.push(classIterator<T>, 2);

        return 1;
    }
    template<typename T, typename C> int pushIterable(C *a)
    {
        if (a == nullptr)
            return 0;
        return pushIterable<T>(*a);
    }
    template<typename T, typename C> int pushClassIterable(C *a)
    {
        if (a == nullptr)
            return 0;
        return pushClassIterable<T>(*a);
    }

    /**** type check methods ****/

    bool isType(int index, int type) const;
    bool isInteger(int n = 0) const { return lua_isinteger(m_lua, n) == 1; }
    bool isNumber(int n = 0) const { return lua_isnumber(m_lua, n) == 1; }
    bool isBoolean(int n = 0) const { return lua_isboolean(m_lua, n) == 1; }
    bool isString(int n = 0) const { return lua_isstring(m_lua, n) == 1; }
    bool isTable(int n = 0) const { return lua_istable(m_lua, n) == 1; }
    bool isUserData(int n = 0) const { return lua_isuserdata(m_lua, n) == 1; }
    bool isValid(int) const;

    /**** get methods ****/

    lua_Integer getInt(int n = 0) const { return lua_tointeger(m_lua, n); }
    double getNumber(int n = 0) const { return lua_tonumber(m_lua, n); }
    bool getBoolean(int n = 0) const { return lua_toboolean(m_lua, n) == 1; }
    const char *getString(int n = 0) const { return lua_tostring(m_lua, n); }
    Value *getValue(int n = 0);
    template<typename T> T *getUserData(int n = 0) const
    {
        return static_cast<T*>(lua_touserdata(m_lua, n));
    }
    template<typename T> T *getClass(int n = 0) const
    {
        T dummy;
        if (isType(n, celxClassId(dummy)))
            return getUserData<T>(n);
        return nullptr;
    }

    void pop(int n) { lua_pop(m_lua, n); }

    /**** various legacy methods ****/

    void setClass(int id);
    void pushClassName(int id);
    void* checkUserData(int index, int id);
    void doError(const char* errorMessage);
    void checkArgs(int minArgs, int maxArgs, const char* errorMessage);
    void createClassMetatable(int id);
    void registerMethod(const char* name, lua_CFunction fn);
    void registerValue(const char* name, float n);

    void setTable(const char* field, lua_Number value);
    void setTable(const char* field, const char* value);

    void newFrame(const ObserverFrame& f);
    void newVector(const Eigen::Vector3d& v);
    void newRotation(const Eigen::Quaterniond& q);
    void newPosition(const UniversalCoord& uc);
    void newObject(const Selection& sel);
    void newPhase(const TimelinePhase::SharedConstPtr& phase);

    Eigen::Vector3d* toVector(int n);
    Eigen::Quaterniond* toRotation(int n);
    UniversalCoord* toPosition(int n);
    Selection* toObject(int n);
    ObserverFrame* toFrame(int n);

    void push(const CelxValue& v1);
    void push(const CelxValue& v1, const CelxValue& v2);

    CelestiaCore* appCore(FatalErrors fatalErrors = NoErrors);

    /**** safe get methods ****/

    bool safeIsValid(int,
                     FatalErrors error = AllErrors,
                     const char *errorMsg = "Invalid stack index.");
    lua_Number safeGetNumber(int index,
                             FatalErrors fatalErrors = AllErrors,
                             const char* errorMessage = "Numeric argument expected",
                             lua_Number defaultValue = 0.0);
    const char* safeGetString(int index,
                              FatalErrors fatalErrors = AllErrors,
                              const char* errorMessage = "String argument expected");
    const char *safeGetNonEmptyString(int index,
                            FatalErrors fatalErrors = AllErrors,
                            const char *errorMessage = "Non empty string argument expected");
    bool safeGetBoolean(int index,
                        FatalErrors fatalErrors = AllErrors,
                        const char* errorMessage = "Boolean argument expected",
                        bool defaultValue = false);

    template<typename T> T *safeGetUserData(int index = 0,
                                            FatalErrors errors = AllErrors,
                                            const char *errorMessage = "User data expected")
    {
        if (!safeIsValid(index))
            return nullptr;
        if (isUserData(index))
            return reinterpret_cast<T*>(lua_touserdata(m_lua, index));

        if (errors & WrongType)
            doError(errorMessage);
        return nullptr;
    }

    template<typename T> T *safeGetClass(int i,
                                         FatalErrors fatalErrors = AllErrors,
                                         const char *msg = "Celx class expected")
    {
        T *a = safeGetUserData<T>(i, fatalErrors, msg);
        if (a != nullptr && isType(i, celxClassId(*a)))
            return a;

        if (fatalErrors & WrongType)
            doError(msg);
        return nullptr;
    }

    template<typename T> T *getThis(FatalErrors fatalErrors = AllErrors,
                                    const char *msg = "Celx class expected")
    {
        return safeGetClass<T>(1, fatalErrors, msg);
    }

    LuaState* getLuaStateObject();

    // String to flag mappings
    typedef std::map<std::string, uint32_t> FlagMap;
    typedef std::map<std::string, uint64_t> FlagMap64;
    typedef std::map<std::string, Color*> ColorMap;

    static void initMaps();
    static void initRenderFlagMap();
    static void initLabelFlagMap();
    static void initBodyTypeMap();
    static void initLocationFlagMap();
    static void initOverlayElementMap();
    static void initOrbitVisibilityMap();
    static void initLabelColorMap();
    static void initLineColorMap();

    static FlagMap64 RenderFlagMap;
    static FlagMap LabelFlagMap;
    static FlagMap64 LocationFlagMap;
    static FlagMap BodyTypeMap;
    static FlagMap OverlayElementMap;
    static FlagMap OrbitVisibilityMap;
    static ColorMap LineColorMap;
    static ColorMap LabelColorMap;
    static bool mapsInitialized;

    static const char* ClassNames[];

private:
    lua_State* m_lua;
};

void Celx_SetClass(lua_State*, int);
void Celx_CreateClassMetatable(lua_State*, int);
void Celx_RegisterMethod(lua_State*, const char*, lua_CFunction);
bool Celx_istype(lua_State*, int index, int);
void* Celx_CheckUserData(lua_State*, int, int);
CelestiaCore* getAppCore(lua_State*, FatalErrors fatalErrors = NoErrors);
void Celx_DoError(lua_State*, const char*);
void Celx_CheckArgs(lua_State*, int, int, const char*);
const char* Celx_SafeGetString(lua_State*,
                                      int,
                                      FatalErrors fatalErrors = AllErrors,
                                      const char* errorMsg = "String argument expected");
lua_Number Celx_SafeGetNumber(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                              const char* errorMsg = "Numeric argument expected",
                              lua_Number defaultValue = 0.0);
bool Celx_SafeGetBoolean(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                              const char* errorMsg = "Boolean argument expected",
                              bool defaultValue = false);

#endif // _CELX_INTERNAL_H_
