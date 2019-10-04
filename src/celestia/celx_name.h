#pragma once

#include "celx_internal.h"
#include <celengine/astroname.h>

inline int celxClassId(NameInfo::SharedConstPtr)
{
    return Celx_Name;
}

void CreateNameMetaTable(lua_State *);
