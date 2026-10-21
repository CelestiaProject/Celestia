#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

class CelestiaCore;

void CreateObserverMetaTable(sol::state_view lua);
void CreateCelestiaMetaTable(sol::state_view lua, CelestiaCore*);
void CreateVectorMetaTable(sol::state_view lua);
void CreatePositionMetaTable(sol::state_view lua);
void CreateRotationMetaTable(sol::state_view lua);
void CreateFrameMetaTable(sol::state_view lua);
void CreatePhaseMetaTable(sol::state_view lua);

void CreateSolMetaTables(lua_State* state, CelestiaCore* appCore)
{
    sol::state_view lua(state);

    CreateObserverMetaTable(lua);
    CreateCelestiaMetaTable(lua, appCore);
    CreatePositionMetaTable(lua);
    CreateVectorMetaTable(lua);
    CreateRotationMetaTable(lua);
    CreateFrameMetaTable(lua);
    CreatePhaseMetaTable(lua);
}
