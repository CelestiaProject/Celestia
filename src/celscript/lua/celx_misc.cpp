// celx_misc.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celx_misc.h"

#include <memory>
#include <string_view>

#include "celx_internal.h"
#include <celscript/legacy/cmdparser.h>
#include <celscript/legacy/execenv.h>
#include <celscript/legacy/execution.h>
#include <celengine/textlayout.h>
#include <celestia/celestiacore.h>
#include <celttf/truetypefont.h>
#include <celutil/gettext.h>
#include "glcompat.h"

using namespace std;
using namespace celestia::engine;
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
        CommandSequence cmdSequence = parser.parse();
        if (!cmdSequence.empty())
        {
            script = std::make_unique<Execution>(std::move(cmdSequence), *this);
        }
        else
        {
            auto errors = parser.getErrors();
            if (!errors.empty())
                errorMessage = "Error while parsing CEL-script: " + errors[0];
            else
                errorMessage = "Error while parsing CEL-script.";
        }
    }

    const std::string& getErrorMessage() const
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

    Simulation* getSimulation() const override
    {
        return core.getSimulation();
    }

    Renderer* getRenderer() const override
    {
        return core.getRenderer();
    }

    CelestiaCore* getCelestiaCore() const override
    {
        return &core;
    }

    void showText(std::string_view s, int horig, int vorig, int hoff, int voff,
                  double duration) override
    {
        core.showText(s, horig, vorig, hoff, voff, duration);
    }

 private:
    std::unique_ptr<Execution> script{ nullptr };
    CelestiaCore& core;
    double tickTime { 0.0 };
    string errorMessage;
};

// ==================== Celscript-object ====================

int celxClassId(CelScriptWrapper*)
{
    return Celx_CelScript;
}

// create a CelScriptWrapper from a string:
int celscript_from_string(lua_State* l, const char* script_text)
{
    CelxLua celx(l);

    CelestiaCore* appCore = celx.appCore(AllErrors);

    {
        // we can't have anything with a non-trivial destructor around when
        // we do the lua_error call (which does a longjmp), so do this stuff within
        // its own block
        istringstream scriptfile(script_text);
        auto celscript = std::make_unique<CelScriptWrapper>(*appCore, scriptfile);
        const auto& error = celscript->getErrorMessage();
        if (error.empty())
        {
            celx.pushClass(celscript.release());
            return 1;
        }

        // we can't use Celx_DoError here since we need to destroy the error string
        // before doing the longjmp
        lua_Debug debug;
        if (lua_getstack(l, 1, &debug) && lua_getinfo(l, "l", &debug))
        {
            std::string buf = fmt::format(fmt::runtime(_("In line {}: {}")), debug.currentline, error);
            lua_pushlstring(l, buf.data(), buf.size());
        }
        else
        {
            lua_pushlstring(l, error.data(), error.size());
        }
    }

    lua_error(l);
    return 1; // not reachable but here to fix warnings
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

    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    (*font)->bind();
    return 0;
}

static int font_unbind(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:unbind()");

    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    (*font)->unbind();
    return 0;
}

static int font_render(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument required for font:render");

    const char* s = celx.safeGetString(2, AllErrors, "First argument to font:render must be a string");
    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    Eigen::Matrix4f p, m;
    glGetFloatv(GL_PROJECTION_MATRIX, p.data());
    glGetFloatv(GL_MODELVIEW_MATRIX, m.data());
    TextLayout layout;
    layout.setFont(*font);
    layout.begin(p, m);
    layout.render(s);
    layout.end();
    return celx.push(layout.getCurrentPosition().first);
}

static int font_getwidth(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument expected for font:getwidth");
    const char* s = celx.safeGetString(2, AllErrors, "Argument to font:getwidth must be a string");
    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    return celx.push(TextLayout::getTextWidth(s, font->get()));
}

static int font_getheight(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:getheight()");

    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    lua_pushnumber(l, (*font)->getHeight());
    return 1;
}

static int font_getmaxascent(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:getmaxascent()");

    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    lua_pushnumber(l, (*font)->getMaxAscent());
    return 1;
}

static int font_getmaxdescent(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for font:getmaxdescent()");

    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    lua_pushnumber(l, (*font)->getMaxDescent());
    return 1;
}

static int font_gettextwidth(lua_State* l)
{
    CelxLua celx(l);

    Celx_CheckArgs(l, 2, 2, "One argument expected to function font:gettextwidth");

    // Do not put the shared_ptr on the stack, otherwise the destructor may be skipped
    // if Celx_SafeGetString calls lua_error (longjmp)
    const auto* font = celx.getThis<std::shared_ptr<TextureFont>>();
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to font:gettextwidth must be a string");

    lua_pushnumber(l, TextLayout::getTextWidth(s, font->get()));

    return 1;
}

static int font_tostring(lua_State* l)
{
    CelxLua celx(l);
    return celx.push("[Font]");
}

static int font_gc(lua_State* l)
{
    CelxLua celx(l);
    auto *font = celx.getThis<std::shared_ptr<TextureFont>>();
    font->~shared_ptr();
    return 0;
}

void CreateFontMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Font);

    celx.registerMethod("__tostring", font_tostring);
    celx.registerMethod("__gc", font_gc);
    celx.registerMethod("bind", font_bind);
    celx.registerMethod("render", font_render);
    celx.registerMethod("unbind", font_unbind);
    celx.registerMethod("getwidth", font_getwidth);
    celx.registerMethod("getheight", font_getheight);
    celx.registerMethod("getmaxascent", font_getmaxascent);
    celx.registerMethod("getmaxdescent", font_getmaxdescent);
    celx.registerMethod("gettextwidth", font_gettextwidth);

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
    std::string s = fmt::format("[Image:{}x{}]", image->getWidth(), image->getHeight());
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
    std::string s = fmt::format("[Texture:{}x{}]", tex->getWidth(), tex->getHeight());
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
