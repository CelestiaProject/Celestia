#include <string_view>
#include <tuple>

#include <doctest.h>

#include <celutil/strnatcmp.h>

using namespace std::string_view_literals;


bool comparesSame(int a, int b)
{
    return ((a < 0) == (b < 0)) && ((a > 0) == (b > 0));
}


TEST_SUITE_BEGIN("Natural string comparison");

TEST_CASE("No numbers")
{
    std::tuple<std::string_view, std::string_view, int> examples[] = {
        { "abc"sv, "abc"sv, 0 },
        { "de"sv, "def"sv, -1 },
        { "ghi"sv, "gh"sv, 1 },
        { "abc"sv, "def"sv, -1 },
        { "ab"sv, "def"sv, -1 },
        { "abc"sv, "de"sv, -1 },
        { "jkl"sv, "ghi"sv, 1 },
        { "jk"sv, "ghi"sv, 1 },
        { "jkl"sv, "gh"sv, 1 },
    };

    for (const auto& [first, second, result] : examples)
    {
        REQUIRE(comparesSame(strnatcmp(first, second), result));
    }
}

TEST_CASE("With numbers")
{
    std::tuple<std::string_view, std::string_view, int> examples[] = {
        { "123", "123", 0 },
        { "1.23", "1.23", 0},
        { "12", "123", -1 },
        { "89", "123", -1 },
        { "123", "12", 1 },
        { "123", "89", 1 },
        { "1.05", "1.2", -1 },
        { "1.2", "1.05", 1 },
        { "z123", "z123", 0 },
        { "z1.23", "z1.23", 0},
        { "z12", "z123", -1 },
        { "z89", "z123", -1 },
        { "z123", "z12", 1 },
        { "z123", "z89", 1 },
        { "z1.05", "z1.2", -1 },
        { "z1.2", "z1.05", 1 },
        { "123z", "123z", 0 },
        { "1.23z", "1.23z", 0},
        { "12z", "123z", -1 },
        { "89z", "123z", -1 },
        { "123z", "12z", 1 },
        { "123z", "89z", 1 },
        { "1.05z", "1.2z", -1 },
        { "1.2z", "1.05z", 1 },
        { "x123z", "x123z", 0 },
        { "x1.23z", "x1.23z", 0},
        { "x12z", "x123z", -1 },
        { "x89z", "x123z", -1 },
        { "x123z", "x12z", 1 },
        { "x123z", "x89z", 1 },
        { "x1.05z", "x1.2z", -1 },
        { "x1.2z", "x1.05z", 1 },
        { "987abc"sv, "987abc"sv, 0 },
        { "987de"sv, "987def"sv, -1 },
        { "987ghi"sv, "987gh"sv, 1 },
        { "987abc"sv, "987def"sv, -1 },
        { "987ab"sv, "987def"sv, -1 },
        { "987abc"sv, "987de"sv, -1 },
        { "987jkl"sv, "987ghi"sv, 1 },
        { "987jk"sv, "987ghi"sv, 1 },
        { "987jkl"sv, "987gh"sv, 1 },
        { "abc987"sv, "abc987"sv, 0 },
        { "de987"sv, "def987"sv, -1 },
        { "ghi987"sv, "gh987"sv, 1 },
        { "abc987"sv, "def987"sv, -1 },
        { "ab987"sv, "def987"sv, -1 },
        { "abc987"sv, "de987"sv, -1 },
        { "jkl987"sv, "ghi987"sv, 1 },
        { "jk987"sv, "ghi987"sv, 1 },
        { "jkl987"sv, "gh987"sv, 1 },
    };

    for (const auto& [first, second, result] : examples)
    {
        REQUIRE(comparesSame(strnatcmp(first, second), result));
    }
}

TEST_CASE("Skip leading space")
{
    std::tuple<std::string_view, std::string_view, int> examples[] = {
        { "   abc"sv, "abc"sv, 0 },
        { "   de"sv, "def"sv, -1 },
        { "   ghi"sv, "gh"sv, 1 },
        { "   abc"sv, "def"sv, -1 },
        { "   ab"sv, "def"sv, -1 },
        { "   abc"sv, "de"sv, -1 },
        { "   jkl"sv, "ghi"sv, 1 },
        { "   jk"sv, "ghi"sv, 1 },
        { "   jkl"sv, "gh"sv, 1 },
        { "abc"sv, "   abc"sv, 0 },
        { "de"sv, "   def"sv, -1 },
        { "ghi"sv, "   gh"sv, 1 },
        { "abc"sv, "   def"sv, -1 },
        { "ab"sv, "   def"sv, -1 },
        { "abc"sv, "   de"sv, -1 },
        { "jkl"sv, "   ghi"sv, 1 },
        { "jk"sv, "   ghi"sv, 1 },
        { "jkl"sv, "   gh"sv, 1 },
    };

    for (const auto& [first, second, result] : examples)
    {
        REQUIRE(comparesSame(strnatcmp(first, second), result));
    }
}

TEST_SUITE_END();
