#include <celutil/winutil.h>

#include <doctest.h>

TEST_SUITE_BEGIN("WinUtil");

TEST_CASE("WideToUTF8")
{
    using celestia::util::WideToUTF8;

    REQUIRE(WideToUTF8(L"").empty() == true);
    REQUIRE(WideToUTF8(L"").length() == 0);
    REQUIRE(WideToUTF8(L"foo") == "foo");
    REQUIRE(WideToUTF8(L"foo").length() == 3);
    REQUIRE(WideToUTF8(L"\u0442\u044d\u0441\u0442") == "\321\202\321\215\321\201\321\202"); // тэст
    REQUIRE(WideToUTF8(L"\u0422\u044d\u0441\u0442").length() == 8);
    REQUIRE(WideToUTF8(L"\u2079") == "\342\201\271"); // superscript 9
    REQUIRE(WideToUTF8(L"\u2079").length() == 3);
}

TEST_SUITE_END();
