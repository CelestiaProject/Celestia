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

#include <map>
#include <string>
#include <Eigen/Core>
#include <Eigen/Geometry>

class CelestiaCore;
class TimelinePhase;

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
    ~CelxLua();
    
    bool isType(int index, int type) const;
    
    void setClass(int id);
    void pushClassName(int id);
    void* checkUserData(int index, int id);
    void doError(const char* errorMessage);
    void checkArgs(int minArgs, int maxArgs, const char* errorMessage);
    void createClassMetatable(int id);
    void registerMethod(const char* name, lua_CFunction fn);
    void registerValue(const char* name, float value);
    
    void setTable(const char* field, lua_Number value);
    void setTable(const char* field, const char* value);
        
    void newFrame(const ObserverFrame& f);
    void newVector(const Vec3d& v);
    void newVector(const Eigen::Vector3d& v);
    void newRotation(const Quatd& q);
    void newRotation(const Eigen::Quaterniond& q);
    void newPosition(const UniversalCoord& uc);
    void newObject(const Selection& sel);
    void newPhase(const TimelinePhase& phase);

    Vec3d* toVector(int n);
    Quatd* toRotation(int n);
    UniversalCoord* toPosition(int n);
    Selection* toObject(int n);
    ObserverFrame* toFrame(int n);
    
    void push(const CelxValue& v1);
    void push(const CelxValue& v1, const CelxValue& v2);
    
    CelestiaCore* appCore(FatalErrors fatalErrors = NoErrors);
    
    lua_Number safeGetNumber(int index,
                             FatalErrors fatalErrors = AllErrors,
                             const char* errorMessage = "Numeric argument expected",
                             lua_Number defaultValue = 0.0);
    const char* safeGetString(int index,
                              FatalErrors fatalErrors = AllErrors,
                              const char* errorMessage = "String argument expected");
    bool safeGetBoolean(int index,
                        FatalErrors fatalErrors = AllErrors,
                        const char* errorMsg = "Boolean argument expected",
                        bool defaultValue = false);
    
    LuaState* getLuaStateObject();

    
    // String to flag mappings
    typedef std::map<std::string, uint32> FlagMap;
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
    
    static FlagMap RenderFlagMap;
    static FlagMap LabelFlagMap;
    static FlagMap LocationFlagMap;
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



#endif // _CELX_INTERNAL_H_
