// legacy_cmdparser_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <sstream>
#include <string_view>

#include <doctest.h>

#include <celscript/common/scriptmaps.h>
#include <celscript/legacy/cmdparser.h>

namespace
{

bool
parsesSuccessfully(std::string_view source)
{
    std::istringstream input{ std::string(source) };
    celestia::scripts::ScriptMaps scriptMaps;
    celestia::scripts::CommandParser parser(input, scriptMaps);
    auto sequence = parser.parse();
    return sequence.size() == 1 && parser.getErrors().empty();
}

} // namespace

TEST_SUITE_BEGIN("Legacy command parser");

TEST_CASE("Time command validates UTC dates")
{
    CHECK(parsesSuccessfully(R"({ time { utc "2000-01-01T12:00:00" } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ time { utc "not-a-date" } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ time { utc "2000-13-01T12:00:00" } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ time { utc "2000-01-01T12:00:00junk" } })"));
}

TEST_CASE("Setorientation rejects invalid rotations")
{
    CHECK(parsesSuccessfully(R"({ setorientation { angle 10 axis [1 0 0] } })"));
    CHECK(parsesSuccessfully(R"({ setorientation { ow 1 } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ setorientation { angle 10 } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ setorientation { angle 10 axis [0 0 0] } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ setorientation { angle 10 axis [1e-50 0 0] } })"));
    CHECK_FALSE(parsesSuccessfully(R"({ setorientation { } })"));
}

TEST_SUITE_END();
