// celx_category.h
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "celx_internal.h"

class UserCategory;

inline int celxClassId(UserCategory *)
{
    return Celx_Category;
}

void CreateCategoryMetaTable(lua_State *);

