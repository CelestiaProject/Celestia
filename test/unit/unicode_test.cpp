#include <celutil/flag.h>
#include <celutil/unicode.h>

#if !defined(HAVE_WIN_ICU_COMBINED_HEADER) && !defined(HAVE_WIN_ICU_SEPARATE_HEADERS)
#include <unicode/ustring.h>
#endif

#include <doctest.h>

using namespace celestia::util;

std::u16string ProcessU16String(const std::u16string& str, ConversionOption options)
{
    std::u16string result;
    if (!ApplyBidiAndShaping(str, result, options))
        return {};

    return result;
}

TEST_SUITE_BEGIN("Unicode");

// Test cases from https://github.com/mapbox/mapbox-gl-rtl-text/blob/main/test/arabic.test.js
TEST_CASE("Unicode")
{
    SUBCASE("ArabicShaping")
    {
        REQUIRE(ProcessU16String(u"\u0633\u0644\u0627\u0645\u06f3\u06f9", ConversionOption::ArabicShaping) == u"\ufeb3\ufefc\ufee1\u06f3\u06f9"); // Numbers and letters
        REQUIRE(ProcessU16String(u"\u0645\u0643\u062a\u0628\u0629\u0020\u0627\u0644\u0625\u0633\u0643\u0646\u062f\u0631\u064a\u0629\u200e\u200e Maktabat al-Iskandar\u012byah", ConversionOption::ArabicShaping) == u"\ufee3\ufedc\ufe98\ufe92\ufe94\u0020\ufe8d\ufef9\ufeb3\ufedc\ufee8\ufeaa\ufead\ufef3\ufe94\u200e\u200e Maktabat al-Iskandar\u012byah");
        REQUIRE(ProcessU16String(u"\u0627\u0644\u064a\u064e\u0645\u064e\u0646\u200e\u200e", ConversionOption::ArabicShaping) == u"\ufe8d\ufedf\ufef4\ufe77\ufee4\ufe77\ufee6\u200e\u200e"); // Tashkeel
    }

    SUBCASE("ArabicShapingWithBidi")
    {
        REQUIRE(ProcessU16String(u"\u0633\u0644\u0627\u0645\u06f3\u06f9", ConversionOption::ArabicShaping | ConversionOption::BidiReordering) == u"\u06f3\u06f9\ufee1\ufefc\ufeb3"); // Numbers and letters
        REQUIRE(ProcessU16String(u"\u0645\u0643\u062a\u0628\u0629\u0020\u0627\u0644\u0625\u0633\u0643\u0646\u062f\u0631\u064a\u0629\u200e\u200e Maktabat al-Iskandar\u012byah", ConversionOption::ArabicShaping | ConversionOption::BidiReordering) == u" Maktabat al-Iskandar\u012byah\ufe94\ufef3\ufead\ufeaa\ufee8\ufedc\ufeb3\ufef9\ufe8d\u0020\ufe94\ufe92\ufe98\ufedc\ufee3");
    }
}

TEST_SUITE_END();
