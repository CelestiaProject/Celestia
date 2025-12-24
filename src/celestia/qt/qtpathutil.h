// qtpathutil.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful filesystem-related functions. Extracted
// from cmodview/fsutils.h
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#pragma once

#include <filesystem>
#include <string_view>
#include <type_traits>

#include <QtGlobal>
#include <QString>

namespace celestia::qt
{

inline std::filesystem::path
QStringToPath(const QString& str)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    return static_cast<std::u16string_view>(str);
#else
    return str.toStdU16String();
#endif
}

// We make this a template function so the unused if constexpr branch is removed
template<typename T,
         std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, std::filesystem::path>, int> = 0>
QString
PathToQString(const T& path)
{
    const auto& native = path.native();
    if constexpr (std::is_same_v<std::remove_cv_t<typename T::value_type>, char>)
        return QString::fromLocal8Bit(native.data());
    else
        return QString::fromWCharArray(native.data());
}

} // end namespace celestia::qt
