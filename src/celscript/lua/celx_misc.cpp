// celx_misc.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/debug.h>
#include "celx_misc.h"
#include "celx_internal.h"
#include <celscript/legacy/cmdparser.h>
#include <celscript/legacy/execution.h>
#include <celestia/celestiacore.h>
#if NO_TTF
#include <celtxf/texturefont.h>
#else
#include <celttf/truetypefont.h>
#endif

using namespace celestia::scripts;

LuaState *getLuaStateObject(lua_State*);

// Wrapper for a CEL-script, including the needed Execution Environment
class CelScriptWrapper : public ExecutionEnvironment
{
 public:
    CelScriptWrapper(CelestiaCore& appCore, istream& scriptfile):
        core(appCore)
    {
        CommandParser parser(scriptfile, appCore.scriptMaps());
        cmdSequence = parser.parse();
        if (cmdSequence != nullptr)
        {
            script = new Execution(*cmdSequence, *this);
        }
        else
        {
            const vector<string>* errors = parser.getErrors();
            if (errors->size() > 0)
                errorMessage = "Error while parsing CEL-script: " + (*errors)[0];
            else
                errorMessage = "Error while parsing CEL-script.";
        }
    }

    virtual ~CelScriptWrapper()
    {
        delete script;
        delete cmdSequence;
    }

    string getErrorMessage() const
    {
        return errorMessage;
    }

    // tick the CEL-script. t is in seconds and doesn't have to start with zero
    bool tick(double t)
    {
        // use first tick to set the time
        if (tickTime == 0.0)
        {
            tickTime = t;
            return false;
        }
        double dt = t - tickTime;
        tickTime = t;
        return script->tick(dt);
    }

    Simulation* getSimulation() const
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const
    {
        return core.getRenderer();
    }

    CelestiaCore* getCelestiaCore() const
    {
        return &core;
    }

    void showText(string s, int horig, int vorig, int hoff, int voff,
                  double duration)
    {
        core.showText(std::move(s), horig, vorig, hoff, voff, duration);
    }

 private:
    Execution* script{ nullptr };
    CelestiaCore& core;
    CommandSequence* cmdSequence{ nullptr };
    double tickTime { 0.0 };
    string errorMessage;
};

// ==================== Celscript-object ====================


int celxClassId(CelScriptWrapper*)
{
    return Celx_CelScript;
}

// create a CelScriptWrapper from a string:
int celscript_from_string(lua_State* l, string& script_text)
{
    CelxLua celx(l);
    istringstream scriptfile(script_text);

    CelestiaCore* appCore = celx.appCore(AllErrors);
    CelScriptWrapper* celscript = new CelScriptWrapper(*appCore, scriptfile);
    if (celscript->getErrorMessage() != "")
    {
        string error = celscript->getErrorMessage();
        delete celscript;
        celx.doError(error.c_str());
    }
    else
    {
        celx.pushClass(celscript);
    }

    return 1;
}

static int celscript_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celscript]");

    return 1;
}

static int celscript_tick(lua_State* l)
{
    CelxLua celx(l);
    auto script = *celx.getThis<CelScriptWrapper*>();
    LuaState* stateObject = celx.getLuaStateObject();
    double t = stateObject->getTime();
    return celx.push(!(script->tick(t)));
}

static int celscript_gc(lua_State* l)
{
    CelxLua celx(l);

    auto script = *celx.getThis<CelScriptWrapper*>();
    delete script;
    return 0;
}


void CreateCelscriptMetaTable(lua_State* l)
{
    CelxLua celx(l);
    celx.createClassMetatable(Celx_CelScript);

    celx.registerMethod("__tostring", celscript_tostring);
    celx.registerMethod("tick", celscript_tick);
    celx.registerMethod("__gc", celscript_gc);

    celx.pop(1); // remove metatable from stack
}

// ==================== Font Object ====================

static int font_bind(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:bind()");

    auto font = *celx.getThis<TextureFont*>();
    font->bind();
    return 0;
}

static int font_unbind(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:unbind()");

    auto font = *celx.getThis<TextureFont*>();
    font->unbind();
    return 0;
}

static int font_render(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument required for font:render");

    const char* s = celx.safeGetString(2, AllErrors, "First argument to font:render must be a string");
    auto font = *celx.getThis<TextureFont*>();
#ifndef GL_ES
    Eigen::Matrix4f p, m;
    glGetFloatv(GL_PROJECTION_MATRIX, p.data());
    glGetFloatv(GL_MODELVIEW_MATRIX, m.data());
    font->setMVPMatrix(p * m);
#endif
    return celx.push(font->render(s));
}

static int font_getwidth(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected for font:getwidth");
    const char* s = celx.safeGetString(2, AllErrors, "Argument to font:getwidth must be a string");
    auto font = *celx.getThis<TextureFont*>();
    return celx.push(font->getWidth(s));
}

static int font_getheight(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:getheight()");

    auto font = *celx.getThis<TextureFont*>();
    lua_pushnumber(l, font->getHeight());
    return 1;
}

static int font_tostring(lua_State* l)
{
    CelxLua celx(l);
    return celx.push("[Font]");
}

void CreateFontMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Font);

    celx.registerMethod("__tostring", font_tostring);
    celx.registerMethod("bind", font_bind);
    celx.registerMethod("render", font_render);
    celx.registerMethod("unbind", font_unbind);
    celx.registerMethod("getwidth", font_getwidth);
    celx.registerMethod("getheight", font_getheight);

    celx.pop(1); // remove metatable from stack
}

// ==================== Image =============================================
#if 0
static int image_new(lua_State* l, Image* i)
{
    Image** ud = static_cast<Image**>(lua_newuserdata(l, sizeof(Image*)));
    *ud = i;

    Celx_SetClass(l, Celx_Image);

    return 1;
}
#endif

int celxClassId(Image*)
{
    return Celx_Image;
}

static int image_getheight(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for image:getheight()");

    auto image = *celx.getThis<Image*>();
    return celx.push(image->getHeight());
}

static int image_getwidth(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for image:getwidth()");

    auto* image = *celx.getThis<Image*>();
    return celx.push(image->getWidth());
}

static int image_tostring(lua_State* l)
{
    CelxLua celx(l);
    auto image = *celx.getThis<Image*>();
    std::string s = fmt::sprintf("[Image:%dx%d]", image->getWidth(), image->getHeight());
    return celx.push(s.c_str());
}

void CreateImageMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Image);

    celx.registerMethod("__tostring", image_tostring);
    celx.registerMethod("getheight", image_getheight);
    celx.registerMethod("getwidth", image_getwidth);

    celx.pop(1); // remove metatable from stack
}

// ==================== Texture ============================================

static int texture_bind(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for texture:bind()");

    auto texture = *celx.getThis<Texture*>();
    texture->bind();
    return 0;
}

static int texture_getheight(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for texture:getheight()");

    auto texture = *celx.getThis<Texture*>();
    return celx.push(texture->getHeight());
}

static int texture_getwidth(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for texture:getwidth()");

    auto texture = *celx.getThis<Texture*>();
    return celx.push(texture->getWidth());
}

static int texture_tostring(lua_State* l)
{
    CelxLua celx(l);

    auto tex = *celx.getThis<Texture*>();
    std::string s = fmt::sprintf("[Texture:%dx%d]", tex->getWidth(), tex->getHeight());
    return celx.push(s.c_str());
}

void CreateTextureMetaTable(lua_State* l)
{
    CelxLua celx(l);
    celx.createClassMetatable(Celx_Texture);

    celx.registerMethod("__tostring", texture_tostring);
    celx.registerMethod("getheight", texture_getheight);
    celx.registerMethod("getwidth", texture_getwidth);
    celx.registerMethod("bind", texture_bind);

    celx.pop(1); // remove metatable from stack
}


