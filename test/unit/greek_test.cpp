#include <string_view>

#include <celutil/greek.h>

#include <doctest.h>

using namespace std::string_view_literals;

TEST_SUITE_BEGIN("Greek");

TEST_CASE("Greek")
{
    SUBCASE("ReplaceGreekLetterAbbr")
    {
        REQUIRE(ReplaceGreekLetterAbbr("XI") == "\316\276"sv);
        REQUIRE(ReplaceGreekLetterAbbr("XI12") == "\316\276\302\271\302\262"sv);
        REQUIRE(ReplaceGreekLetterAbbr("XI Foo") == "\316\276 Foo"sv);
        REQUIRE(ReplaceGreekLetterAbbr("XI12 Bar") == "\316\276\302\271\302\262 Bar"sv);

        REQUIRE(ReplaceGreekLetterAbbr("xi") == "xi"sv);
        REQUIRE(ReplaceGreekLetterAbbr("xi12") == "xi12"sv);
        REQUIRE(ReplaceGreekLetterAbbr("xi Foo") == "xi Foo"sv);
        REQUIRE(ReplaceGreekLetterAbbr("xi12 Bar") == "xi12 Bar"sv);

        REQUIRE(ReplaceGreekLetterAbbr("alpha") == "alpha"sv);
    }

    SUBCASE("ReplaceGreekLetter")
    {
        REQUIRE(ReplaceGreekLetter("XI") == "\316\276"sv);
        REQUIRE(ReplaceGreekLetter("XI12") == "\316\276\302\271\302\262"sv);
        REQUIRE(ReplaceGreekLetter("XI Foo") == "\316\276 Foo"sv);
        REQUIRE(ReplaceGreekLetter("XI12 Bar") == "\316\276\302\271\302\262 Bar"sv);

        REQUIRE(ReplaceGreekLetter("xi") == "\316\276"sv);
        REQUIRE(ReplaceGreekLetter("xi12") == "\316\276\302\271\302\262"sv);
        REQUIRE(ReplaceGreekLetter("xi Foo") == "\316\276 Foo"sv);
        REQUIRE(ReplaceGreekLetter("xi12 Bar") == "\316\276\302\271\302\262 Bar"sv);

        REQUIRE(ReplaceGreekLetter("alpha") == "\316\261"sv);
    }

    SUBCASE("GetCanonicalGreekAbbreviation")
    {
        REQUIRE(GetCanonicalGreekAbbreviation("xi") == "XI"sv);
        REQUIRE(GetCanonicalGreekAbbreviation("alpha") == "ALF"sv);
    }
}

TEST_SUITE_END();
