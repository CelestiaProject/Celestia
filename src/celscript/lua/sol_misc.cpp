// celx_misc.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#define SOL_ALL_SAFETIES_ON 1

#include <celengine/textlayout.h>
#include <celestia/celestiacore.h>
#include <celscript/legacy/cmdparser.h>
#include <celscript/legacy/execenv.h>
#include <celscript/legacy/execution.h>
#include <fmt/format.h>
#include <memory>
#include <sol/sol.hpp>
#include <string_view>
//#include <celttf/truetypefont.h>
//#include <celutil/gettext.h>
#include "celx_internal.h" // TODO
//#include "glcompat.h"

using namespace std::string_view_literals;
using namespace celestia::scripts;
using namespace celestia::engine;

LuaState *getLuaStateObject(lua_State *); // TODO

namespace
{

// TODO: move to own file, rewrite without CelxLua
CelestiaCore *
getAppCore(sol::this_state s)
{
    return CelxLua(s).appCore(AllErrors);
}

template<typename T>
struct CelxProxy
{
    T *operator->() const
    {
        return wrapped;
    }
    T *get() const
    {
        return wrapped;
    }

private:
    T *wrapped;
};

// Wrapper for a CEL-script, including the needed Execution Environment
class CelScriptWrapper : public ExecutionEnvironment
{
public:
    CelScriptWrapper(CelestiaCore &appCore, std::istream &scriptfile) : core(appCore)
    {
        CommandParser   parser(scriptfile, appCore.scriptMaps());
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

    const std::string &getErrorMessage() const
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
        tickTime  = t;
        return script->tick(dt);
    }

    Simulation *getSimulation() const override
    {
        return core.getSimulation();
    }

    Renderer *getRenderer() const override
    {
        return core.getRenderer();
    }

    CelestiaCore *getCelestiaCore() const override
    {
        return &core;
    }

    void
    showText(std::string_view s, int horig, int vorig, int hoff, int voff, double duration) override
    {
        core.showText(s, horig, vorig, hoff, voff, duration);
    }

private:
    std::unique_ptr<Execution> script{ nullptr };
    CelestiaCore              &core;
    double                     tickTime{ 0.0 };
    std::string                errorMessage;
};

// ==================== Celscript-object ====================

// create a CelScriptWrapper from a string:
#if 0
int celscript_from_string(lua_State* l, const char* script_text)
{
    CelxLua celx(l);

    CelestiaCore* appCore = celx.appCore(AllErrors);

    {
        // we can't have anything with a non-trivial destructor around when
        // we do the lua_error call (which does a longjmp), so do this stuff within
        // its own block
        std::istringstream scriptfile(script_text);
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
#endif

bool
celscript_tick(CelxProxy<CelScriptWrapper> &script, sol::this_state s)
{
    LuaState *stateObject = getLuaStateObject(s.lua_state());
    double    t           = stateObject->getTime();
    return !script->tick(t);
}

// ==================== Font Object ====================

struct CelxFont
{
    std::shared_ptr<TextureFont> font;

    float render(std::string_view s) const
    {
        Eigen::Matrix4f p, m;
        glGetFloatv(GL_PROJECTION_MATRIX, p.data());
        glGetFloatv(GL_MODELVIEW_MATRIX, m.data());
        TextLayout layout;
        layout.setFont(font);
        layout.begin(p, m);
        layout.render(s);
        layout.end();
        return layout.getCurrentPosition().first;
    }

    int getheight() const
    {
        return font->getHeight();
    }

    int getmaxascent() const
    {
        return font->getMaxAscent();
    }

    int getmaxdescent() const
    {
        return font->getMaxDescent();
    }

    int gettextwidth(std::string_view s) const
    {
        return TextLayout::getTextWidth(s, font.get());
    }

    void bind() const
    {
        font->bind();
    }

    void unbind() const
    {
        font->unbind();
    }

    void gc()
    {
        std::destroy_at(&font);
    }
};

} // namespace

// ==================== Font Object ====================

void
CreateFontMetaTable(sol::state_view lua)
{
    lua.new_usertype<CelxFont>(
        "Font",
        sol::no_constructor,
        sol::meta_function::to_string,
        []() { return "[Font]"; },
        sol::meta_function::garbage_collect,
        &CelxFont::gc,
        "bind",
        &CelxFont::bind,
        "render",
        &CelxFont::render,
        "unbind",
        &CelxFont::unbind,
        "getwidth",
        &CelxFont::gettextwidth,
        "getheight",
        &CelxFont::getheight,
        "getmaxascent",
        &CelxFont::getmaxascent,
        "getmaxdescent",
        &CelxFont::getmaxdescent,
        "gettextwidth",
        &CelxFont::gettextwidth);
}

// ==================== Celscript-object ====================

void
CreateCelscriptMetaTable(sol::state_view lua)
{
    lua.new_usertype<CelxProxy<CelScriptWrapper>>(
        "Celscript",
        sol::no_constructor,
        "__tostring",
        []() { return "[Celscript]"; },
        "tick",
        celscript_tick,
        "__gc",
        [](CelxProxy<CelScriptWrapper> &script) { delete script.get(); });
}

// ==================== Image =============================================
void
CreateImageMetaTable(sol::state_view lua)
{
    lua.new_usertype<CelxProxy<Image>>(
        "Image",
        sol::no_constructor,
        "__tostring",
        [](CelxProxy<Image> &image)
        { return fmt::format("[Image:{}x{}]", image->getWidth(), image->getHeight()); },
        "getheight",
        [](CelxProxy<Image> &image) { return image->getHeight(); },
        "getwidth",
        [](CelxProxy<Image> &image) { return image->getWidth(); });
}

// ==================== Texture ============================================
void
CreateTextureMetaTable(sol::state_view lua)
{
    lua.new_usertype<CelxProxy<Texture>>(
        "Texture",
        sol::no_constructor,
        "__tostring",
        [](CelxProxy<Texture> &tex)
        { return fmt::format("[Texture:{}x{}]", tex->getWidth(), tex->getHeight()); },
        "getheight",
        [](CelxProxy<Texture> &tex) { return tex->getHeight(); },
        "getwidth",
        [](CelxProxy<Texture> &tex) { return tex->getWidth(); },
        "bind",
        [](CelxProxy<Texture> &tex) { tex->bind(); });
}
