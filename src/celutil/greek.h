// greek.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//               2018-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string      ReplaceGreekLetterAbbr(std::string_view str);
std::string      ReplaceGreekLetter(std::string_view str);
std::string_view GetCanonicalGreekAbbreviation(std::string_view letter);
