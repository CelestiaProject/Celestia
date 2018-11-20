
#include "celx_internal.h"
#include <celengine/cmdparser.h>
#include <celengine/execenv.h>
#include <celengine/execution.h>
#include "celestiacore.h"

LuaState *getLuaStateObject(lua_State*);

// Wrapper for a CEL-script, including the needed Execution Environment
class CelScriptWrapper : public ExecutionEnvironment
{
 public:
    CelScriptWrapper(CelestiaCore& appCore, istream& scriptfile):
        core(appCore)
    {
        CommandParser parser(scriptfile);
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

// create a CelScriptWrapper from a string:
int celscript_from_string(lua_State* l, string& script_text)
{
    istringstream scriptfile(script_text);

    CelestiaCore* appCore = getAppCore(l, AllErrors);
    CelScriptWrapper* celscript = new CelScriptWrapper(*appCore, scriptfile);
    if (celscript->getErrorMessage() != "")
    {
        string error = celscript->getErrorMessage();
        delete celscript;
        Celx_DoError(l, error.c_str());
    }
    else
    {
        CelScriptWrapper** ud = reinterpret_cast<CelScriptWrapper**>(lua_newuserdata(l, sizeof(CelScriptWrapper*)));
        *ud = celscript;
        Celx_SetClass(l, Celx_CelScript);
    }

    return 1;
}

static CelScriptWrapper* this_celscript(lua_State* l)
{
    auto** script = static_cast<CelScriptWrapper**>(Celx_CheckUserData(l, 1, Celx_CelScript));
    if (script == nullptr)
    {
        Celx_DoError(l, "Bad CEL-script object!");
        return nullptr;
    }
    return *script;
}

static int celscript_tostring(lua_State* l)
{
    lua_pushstring(l, "[Celscript]");

    return 1;
}

static int celscript_tick(lua_State* l)
{
    CelScriptWrapper* script = this_celscript(l);
    LuaState* stateObject = getLuaStateObject(l);
    double t = stateObject->getTime();
    lua_pushboolean(l, !(script->tick(t)) );
    return 1;
}

static int celscript_gc(lua_State* l)
{
    CelScriptWrapper* script = this_celscript(l);
    delete script;
    return 0;
}


void CreateCelscriptMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_CelScript);

    Celx_RegisterMethod(l, "__tostring", celscript_tostring);
    Celx_RegisterMethod(l, "tick", celscript_tick);
    Celx_RegisterMethod(l, "__gc", celscript_gc);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Font Object ====================

int font_new(lua_State* l, TextureFont* f)
{
    TextureFont** ud = static_cast<TextureFont**>(lua_newuserdata(l, sizeof(TextureFont*)));
    *ud = f;

    Celx_SetClass(l, Celx_Font);

    return 1;
}

static TextureFont* to_font(lua_State* l, int index)
{
    TextureFont** f = static_cast<TextureFont**>(lua_touserdata(l, index));

    // Check if pointer is valid
    if (f != nullptr )
    {
            return *f;
    }
    return nullptr;
}

static TextureFont* this_font(lua_State* l)
{
    TextureFont* f = to_font(l, 1);
    if (f == nullptr)
        Celx_DoError(l, "Bad font object!");

    return f;
}


static int font_bind(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for font:bind()");

    TextureFont* font = this_font(l);
    font->bind();
    return 0;
}

static int font_render(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument required for font:render");

    const char* s = Celx_SafeGetString(l, 2, AllErrors, "First argument to font:render must be a string");
    TextureFont* font = this_font(l);
    font->render(s);

    return 0;
}

static int font_getwidth(lua_State* l)
{
    Celx_CheckArgs(l, 2, 2, "One argument expected for font:getwidth");
    const char* s = Celx_SafeGetString(l, 2, AllErrors, "Argument to font:getwidth must be a string");
    TextureFont* font = this_font(l);
    lua_pushnumber(l, font->getWidth(s));
    return 1;
}

static int font_getheight(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for font:getheight()");

    TextureFont* font = this_font(l);
    lua_pushnumber(l, font->getHeight());
    return 1;
}

static int font_tostring(lua_State* l)
{
    // TODO: print out the actual information about the font
    lua_pushstring(l, "[Font]");

    return 1;
}

void CreateFontMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Font);

    Celx_RegisterMethod(l, "__tostring", font_tostring);
    Celx_RegisterMethod(l, "bind", font_bind);
    Celx_RegisterMethod(l, "render", font_render);
    Celx_RegisterMethod(l, "getwidth", font_getwidth);
    Celx_RegisterMethod(l, "getheight", font_getheight);

    lua_pop(l, 1); // remove metatable from stack
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

static Image* to_image(lua_State* l, int index)
{
    Image** image = static_cast<Image**>(lua_touserdata(l, index));

    return image != nullptr ? *image : nullptr;
}

static Image* this_image(lua_State* l)
{
    Image* image = to_image(l,1);
    if (image == nullptr)
        Celx_DoError(l, "Bad image object!");

    return image;
}

static int image_getheight(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for image:getheight()");

    Image* image = this_image(l);
    lua_pushnumber(l, image->getHeight());
    return 1;
}

static int image_getwidth(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for image:getwidth()");

    Image* image = this_image(l);
    lua_pushnumber(l, image->getWidth());
    return 1;
}

static int image_tostring(lua_State* l)
{
    // TODO: print out the actual information about the image
    lua_pushstring(l, "[Image]");

    return 1;
}

void CreateImageMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Image);

    Celx_RegisterMethod(l, "__tostring", image_tostring);
    Celx_RegisterMethod(l, "getheight", image_getheight);
    Celx_RegisterMethod(l, "getwidth", image_getwidth);

    lua_pop(l, 1); // remove metatable from stack
}

// ==================== Texture ============================================

int texture_new(lua_State* l, Texture* t)
{
    Texture** ud = static_cast<Texture**>(lua_newuserdata(l, sizeof(Texture*)));
    *ud = t;

    Celx_SetClass(l, Celx_Texture);

    return 1;
}

static Texture* to_texture(lua_State* l, int index)
{
    Texture** texture = static_cast<Texture**>(lua_touserdata(l, index));

    return texture != nullptr ? *texture : nullptr;
}

static Texture* this_texture(lua_State* l)
{
    Texture* texture = to_texture(l,1);
    if (texture == nullptr)
        Celx_DoError(l, "Bad texture object!");

    return texture;
}

static int texture_bind(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for texture:bind()");

    Texture* texture = this_texture(l);
    texture->bind();
    return 0;
}

static int texture_getheight(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for texture:getheight()");

    Texture* texture = this_texture(l);
    lua_pushnumber(l, texture->getHeight());
    return 1;
}

static int texture_getwidth(lua_State* l)
{
    Celx_CheckArgs(l, 1, 1, "No arguments expected for texture:getwidth()");

    Texture* texture = this_texture(l);
    lua_pushnumber(l, texture->getWidth());
    return 1;
}

static int texture_tostring(lua_State* l)
{
    // TODO: print out the actual information about the texture
    lua_pushstring(l, "[Texture]");

    return 1;
}

void CreateTextureMetaTable(lua_State* l)
{
    Celx_CreateClassMetatable(l, Celx_Texture);

    Celx_RegisterMethod(l, "__tostring", texture_tostring);
    Celx_RegisterMethod(l, "getheight", texture_getheight);
    Celx_RegisterMethod(l, "getwidth", texture_getwidth);
    Celx_RegisterMethod(l, "bind", texture_bind);

    lua_pop(l, 1); // remove metatable from stack
}


