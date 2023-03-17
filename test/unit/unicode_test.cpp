#include <unicode/unistr.h>
#include <unicode/ustring.h>

#include <celutil/flag.h>
#include <celutil/unicode.h>

#include <catch.hpp>

using namespace celestia::util;

inline std::wstring ProcessWString(const std::wstring& str, ConversionOption options)
{
    std::vector<UChar> result;
    int32_t requiredSize;
    UErrorCode error = U_ZERO_ERROR;
    u_strFromWCS(nullptr, 0, &requiredSize, str.c_str(), str.size(), &error);
    if (error != U_BUFFER_OVERFLOW_ERROR)
        return L"";

    result.resize(requiredSize + 1);
    error = U_ZERO_ERROR;

    u_strFromWCS(result.data(), result.size(), nullptr, str.c_str(), str.size(), &error);
    result.resize(requiredSize);
    if (error != U_ZERO_ERROR)
        return L"";

    std::wstring output;
    if (!UnicodeStringToWString(icu::UnicodeString(result.data(), result.size()), output, options))
        return L"";
    return output;
}

// Test cases from https://github.com/mapbox/mapbox-gl-rtl-text/blob/main/test/arabic.test.js
TEST_CASE("Unicode", "[Unicode]")
{
    SECTION("ArabicShaping")
    {
        REQUIRE(ProcessWString(L"\u0633\u0644\u0627\u0645\u06f3\u06f9", ConversionOption::ArabicShaping) == L"\ufeb3\ufefc\ufee1\u06f3\u06f9"); // Numbers and letters
        REQUIRE(ProcessWString(L"\u0645\u0643\u062a\u0628\u0629\u0020\u0627\u0644\u0625\u0633\u0643\u0646\u062f\u0631\u064a\u0629\u200e\u200e Maktabat al-Iskandar\u012byah", ConversionOption::ArabicShaping) == L"\ufee3\ufedc\ufe98\ufe92\ufe94\u0020\ufe8d\ufef9\ufeb3\ufedc\ufee8\ufeaa\ufead\ufef3\ufe94\u200e\u200e Maktabat al-Iskandar\u012byah");
        REQUIRE(ProcessWString(L"\u0627\u0644\u064a\u064e\u0645\u064e\u0646\u200e\u200e", ConversionOption::ArabicShaping) == L"\ufe8d\ufedf\ufef4\ufe77\ufee4\ufe77\ufee6\u200e\u200e"); // Tashkeel
    }

    SECTION("ArabicShapingWithBidi")
    {
        REQUIRE(ProcessWString(L"\u0633\u0644\u0627\u0645\u06f3\u06f9", ConversionOption::ArabicShaping | ConversionOption::BidiReordering) == L"\u06f3\u06f9\ufee1\ufefc\ufeb3"); // Numbers and letters
        REQUIRE(ProcessWString(L"\u0645\u0643\u062a\u0628\u0629\u0020\u0627\u0644\u0625\u0633\u0643\u0646\u062f\u0631\u064a\u0629\u200e\u200e Maktabat al-Iskandar\u012byah", ConversionOption::ArabicShaping | ConversionOption::BidiReordering) == L" Maktabat al-Iskandar\u012byah\ufe94\ufef3\ufead\ufeaa\ufee8\ufedc\ufeb3\ufef9\ufe8d\u0020\ufe94\ufe92\ufe98\ufedc\ufee3");
    }
}
