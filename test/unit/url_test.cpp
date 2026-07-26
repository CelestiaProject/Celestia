// url_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <doctest.h>

#include <celestia/url.h>

TEST_SUITE_BEGIN("URL");

TEST_CASE("URL parser rejects an empty path")
{
    Url url(nullptr);
    CHECK_FALSE(url.parse("cel://"));
    CHECK_FALSE(url.parse("cel:///"));
    CHECK_FALSE(url.parse("cel://?x=1"));
}

TEST_SUITE_END();
