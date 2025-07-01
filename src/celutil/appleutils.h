// appleutils.h
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// Miscellaneous useful Apple platform functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>

std::filesystem::path AppleHomeDirectory();
std::filesystem::path AppleApplicationSupportDirectory();
