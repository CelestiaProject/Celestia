#ifndef __SRC__CELESTIA_CELX__H__ 
#define __SRC__CELESTIA_CELX__H__ 

#include <lua.hpp>

#include <celengine/observer.h>

class LuaState;

class CelestiaCoreApplication;

class TextureFont;

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

CelestiaCoreApplication* getAppCore(lua_State*, FatalErrors fatalErrors = NoErrors);

void openLuaLibrary(lua_State*, const char*, lua_CFunction);

void loadLuaLibs(lua_State*);

bool Celx_istype(lua_State*, int, int);
void Celx_SetClass(lua_State*, int);
void* Celx_CheckUserData(lua_State*, int, int);
void Celx_DoError(lua_State*, const char*);
void Celx_CheckArgs(lua_State*, int, int, const char*);
void Celx_CreateClassMetatable(lua_State* l, int id);
void Celx_RegisterMethod(lua_State* l, const char* name, lua_CFunction fn);
const char* Celx_SafeGetString(lua_State* l,
                                      int index,
                                      FatalErrors fatalErrors = AllErrors,
                                      const char* errorMsg = "String argument expected");
lua_Number Celx_SafeGetNumber(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                              const char* errorMsg = "Numeric argument expected",
                              lua_Number defaultValue = 0.0);
bool Celx_SafeGetBoolean(lua_State* l, int index, FatalErrors fatalErrors = AllErrors,
                              const char* errorMsg = "Boolean argument expected",
                              bool defaultValue = false);

CelestiaCoreApplication* appCore(FatalErrors fatalErrors = NoErrors);

void setTable(lua_State*, const char*, lua_Number);

LuaState* getLuaStateObject(lua_State*);

ObserverFrame::CoordinateSystem parseCoordSys(const string&);

extern const char* CleanupCallback;
extern const char* KbdCallback;

extern const char* EventHandlers;
extern const char* KeyHandler;
extern const char* TickHandler;
extern const char* MouseDownHandler;
extern const char* MouseUpHandler;

int celscript_from_string(lua_State* , string& );
int texture_new(lua_State* , Texture* );
int font_new(lua_State* , TextureFont* );
void PushClass(lua_State* , int);

#endif
