#pragma once

#include "celx_internal.h"

class UserCategory;

inline int celxClassId(UserCategory *)
{
    return Celx_Category;
}

void CreateCategoryMetaTable(lua_State *);

