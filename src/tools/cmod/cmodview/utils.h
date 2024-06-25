// cmodview - An application for previewing cmod and other 3D file formats
// supported by Celestia.
//
// Copyright (C) 2024, Celestia Development Team
//
// Extracted from materialwidget/modelwidget
// Copyright (C) 2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cassert>
#include <type_traits>

#include <QString>

namespace cmodview
{

template<typename T>
inline QString toQString(const T* s)
{
    static_assert(std::is_same_v<std::remove_cv_t<T>, char> ||
                  std::is_same_v<std::remove_cv_t<T>, wchar_t>);
    if constexpr (std::is_same_v<std::remove_cv_t<T>, char>)
        return QString::fromLocal8Bit(s);
    else
        return QString::fromWCharArray(s);
}

} // end namespace cmodview
