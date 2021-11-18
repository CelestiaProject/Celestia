#include <celutil/winutil.h>

#include <catch.hpp>

TEST_CASE("CurrentCPToWide", "[CurrentCPToWide]")
{
    REQUIRE(CurrentCPToWide("").empty() == true);
    REQUIRE(CurrentCPToWide("").length() == 0);
    REQUIRE(CurrentCPToWide("foo") == L"foo");
    REQUIRE(CurrentCPToWide("foo").length() == 3);
}

TEST_CASE("WideToCurrentCP", "[WideToCurrentCP]")
{
    REQUIRE(WideToCurrentCP(L"").empty() == true);
    REQUIRE(WideToCurrentCP(L"").length() == 0);
    REQUIRE(WideToCurrentCP(L"foo") == "foo");
    REQUIRE(WideToCurrentCP(L"foo").length() == 3);
}

TEST_CASE("WideToUTF8","[WideToUTF8]")
{
    REQUIRE(WideToUTF8(L"").empty() == true);
    REQUIRE(WideToUTF8(L"").length() == 0);
    REQUIRE(WideToUTF8(L"foo") == "foo");
    REQUIRE(WideToUTF8(L"foo").length() == 3);
    REQUIRE(WideToUTF8(L"\u0442\u044d\u0441\u0442") == "\321\202\321\215\321\201\321\202"); // тэст
    REQUIRE(WideToUTF8(L"\u0422\u044d\u0441\u0442").length() == 8);
    REQUIRE(WideToUTF8(L"\u2079") == "\342\201\271"); // superscript 9
    REQUIRE(WideToUTF8(L"\u2079").length() == 3);
}

TEST_CASE("UTF8ToWide", "[UTF8ToWide]")
{
    REQUIRE(UTF8ToWide("").empty() == true);
    REQUIRE(UTF8ToWide("").length() == 0);
    REQUIRE(UTF8ToWide("foo") == L"foo");
    REQUIRE(UTF8ToWide("foo").length() == 3);
    REQUIRE(UTF8ToWide("\321\202\321\215\321\201\321\202") == L"\u0442\u044d\u0441\u0442");
    REQUIRE(UTF8ToWide("\321\202\321\215\321\201\321\202").length() == 4);
    REQUIRE(UTF8ToWide("\342\201\271") == L"\u2079");
    REQUIRE(UTF8ToWide("\342\201\271").length() == 1);
}
