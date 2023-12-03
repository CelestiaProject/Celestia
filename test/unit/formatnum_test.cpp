#include <array>
#include <locale>
#include <string>
#include <string_view>
#include <tuple>

#include <fmt/format.h>

#include <doctest.h>

#include <celutil/formatnum.h>

using namespace std::string_view_literals;
using celestia::util::NumberFormat;
using celestia::util::NumberFormatter;

namespace
{

class TestNumpunct : public std::numpunct<wchar_t>
{
protected:
    // Custom separators
    wchar_t do_decimal_point() const override { return L'x'; }
    wchar_t do_thousands_sep() const override { return L'_'; }
    // Simulate Indian grouping rules
    std::string do_grouping() const override { return "\003\002"; }
};

using TestCase = std::tuple<double, unsigned int, std::string_view>;

} // end unnamed namespace

TEST_SUITE_BEGIN("Formatnum");

TEST_CASE("NumberFormat None")
{
    constexpr auto format = NumberFormat::None;

    std::locale testloc{ std::locale("C"), new TestNumpunct() };

    NumberFormatter formatter(testloc);

    constexpr std::array testCases
    {
        TestCase{ 0.0, 0, "0"sv },
        TestCase{ 0.0, 2, "0x00"sv },
        TestCase{ -0.0, 0, "-0"sv },
        TestCase{ -0.0, 3, "-0x000"sv },
        TestCase{ 12.345, 1, "12x3"sv },
        TestCase{ -12.346, 2, "-12x35"sv },
    };

    for (const auto& [value, precision, expected] : testCases)
    {
        auto actual = fmt::format("{}", formatter.format(value, precision, format));
        REQUIRE(expected == actual);
    }
}

TEST_CASE("NumberFormat GroupThousands")
{
    constexpr auto format = NumberFormat::GroupThousands;

    std::locale testloc{ std::locale("C"), new TestNumpunct() };

    NumberFormatter formatter(testloc);

    constexpr std::array testCases
    {
        TestCase{ 0.0, 0, "0"sv },
        TestCase{ 0.0, 2, "0x00"sv },
        TestCase{ -0.0, 0, "-0"sv },
        TestCase{ -0.0, 3, "-0x000"sv },
        TestCase{ 12.345, 1, "12x3"sv },
        TestCase{ -12.346, 2, "-12x35"sv },
        TestCase{ 123.98, 1, "124x0"sv },
        TestCase{ -123.98, 1, "-124x0"sv },
        TestCase{ 1234.12, 1, "1_234x1"sv },
        TestCase{ -1234.12, 1, "-1_234x1"sv },
        TestCase{ 12345.12, 2, "12_345x12"sv },
        TestCase{ -12345.12, 2, "-12_345x12"sv },
        TestCase{ 123456.1, 0, "1_23_456"sv },
        TestCase{ -123456.1, 0, "-1_23_456"sv },
        TestCase{ 192837123, 0, "19_28_37_123"sv },
        TestCase{ -192837123, 0, "-19_28_37_123"sv },
    };

    for (const auto& [value, precision, expected] : testCases)
    {
        auto actual = fmt::format("{}", formatter.format(value, precision, format));
        REQUIRE(expected == actual);
    }
}

TEST_CASE("NumberFormat SignificantFigures")
{
    constexpr auto format = NumberFormat::SignificantFigures;

    std::locale testloc{ std::locale("C"), new TestNumpunct() };

    NumberFormatter formatter(testloc);

    constexpr std::array testCases
    {
        TestCase{ 1.5292, 2, "1x5"sv },
        TestCase{ -1.5292, 2, "-1x5"sv },
        TestCase{ 15.292, 2, "15"sv },
        TestCase{ -15.292, 2, "-15"sv },
        TestCase{ 152.92, 2, "150"sv },
        TestCase{ -152.92, 2, "-150"sv },
        TestCase{ 1529.2, 2, "1500"sv },
        TestCase{ -1529.2, 2, "-1500"sv },
        TestCase{ 1529200, 2, "1500000"sv},
        TestCase{ -1529200, 2, "-1500000"sv},
        TestCase{ 0.00015292, 2, "0x00015"sv },
        TestCase{ -0.00015292, 2, "-0x00015"sv },
    };

    for (const auto& [value, precision, expected] : testCases)
    {
        auto actual = fmt::format("{}", formatter.format(value, precision, format));
        REQUIRE(expected == actual);
    }
}

TEST_CASE("NumberFormat GroupThousands+SignificantFigures")
{
    constexpr auto format = NumberFormat::GroupThousands | NumberFormat::SignificantFigures;

    std::locale testloc{ std::locale("C"), new TestNumpunct() };

    NumberFormatter formatter(testloc);

    constexpr std::array testCases
    {
        TestCase{ 1.5292, 2, "1x5"sv },
        TestCase{ -1.5292, 2, "-1x5"sv },
        TestCase{ 15.292, 2, "15"sv },
        TestCase{ -15.292, 2, "-15"sv },
        TestCase{ 152.92, 2, "150"sv },
        TestCase{ -152.92, 2, "-150"sv },
        TestCase{ 1529.2, 2, "1_500"sv },
        TestCase{ -1529.2, 2, "-1_500"sv },
        TestCase{ 1529200, 2, "15_00_000"sv},
        TestCase{ -1529200, 2, "-15_00_000"sv},
        TestCase{ 0.00015292, 2, "0x00015"sv },
        TestCase{ -0.00015292, 2, "-0x00015"sv },
    };

    for (const auto& [value, precision, expected] : testCases)
    {
        auto actual = fmt::format("{}", formatter.format(value, precision, format));
        REQUIRE(expected == actual);
    }
}

TEST_SUITE_END();
