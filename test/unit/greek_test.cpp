#include <celutil/greek.h>

#include <catch.hpp>

TEST_CASE("Greek", "[Greek]")
{
    SECTION("ReplaceGreekLetterAbbr")
    {
        REQUIRE(ReplaceGreekLetterAbbr("XI") == "\316\276");
        REQUIRE(ReplaceGreekLetterAbbr("XI12") == "\316\276\302\271\302\262");
        REQUIRE(ReplaceGreekLetterAbbr("XI Foo") == "\316\276 Foo");
        REQUIRE(ReplaceGreekLetterAbbr("XI12 Bar") == "\316\276\302\271\302\262 Bar");

        REQUIRE(ReplaceGreekLetterAbbr("xi") == "xi");
        REQUIRE(ReplaceGreekLetterAbbr("xi12") == "xi12");
        REQUIRE(ReplaceGreekLetterAbbr("xi Foo") == "xi Foo");
        REQUIRE(ReplaceGreekLetterAbbr("xi12 Bar") == "xi12 Bar");

        REQUIRE(ReplaceGreekLetterAbbr("alpha") == "alpha");
    }

    SECTION("ReplaceGreekLetter")
    {
        REQUIRE(ReplaceGreekLetter("XI") == "\316\276");
        REQUIRE(ReplaceGreekLetter("XI12") == "\316\276\302\271\302\262");
        REQUIRE(ReplaceGreekLetter("XI Foo") == "\316\276 Foo");
        REQUIRE(ReplaceGreekLetter("XI12 Bar") == "\316\276\302\271\302\262 Bar");

        REQUIRE(ReplaceGreekLetter("xi") == "\316\276");
        REQUIRE(ReplaceGreekLetter("xi12") == "\316\276\302\271\302\262");
        REQUIRE(ReplaceGreekLetter("xi Foo") == "\316\276 Foo");
        REQUIRE(ReplaceGreekLetter("xi12 Bar") == "\316\276\302\271\302\262 Bar");

        REQUIRE(ReplaceGreekLetter("alpha") == "\316\261");
    }

    SECTION("GetCanonicalGreekAbbreviation")
    {
        REQUIRE(GetCanonicalGreekAbbreviation("xi") == "XI");
        REQUIRE(GetCanonicalGreekAbbreviation("alpha") == "ALF");
    }
}
