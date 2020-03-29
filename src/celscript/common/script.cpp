// script.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "script.h"

namespace celestia
{
namespace scripts
{

bool IScript::handleMouseButtonEvent(float /*x*/, float /*y*/, int /*button*/, bool /*down*/)
{
    return false;
}

bool IScript::charEntered(const char*)
{
    return false;
}

bool IScript::handleKeyEvent(const char* /*key*/)
{
    return false;
}

bool IScript::handleTickEvent(double /*dt*/)
{
    return false;
}

}
}
