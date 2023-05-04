#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

#include <celcompat/charconv.h>

#include <doctest.h>

namespace celcompat = celestia::compat;

template<typename T>
struct TestCase
{
    const char* source;
    std::size_t size;
    T expected;
    std::ptrdiff_t length;
};

TEST_SUITE_BEGIN("charconv");

TEST_CASE_TEMPLATE("Floating point general format: successful", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "123", 3, TestType{ 123.0 }, 3 },
        { "1234", 3, TestType{ 123.0 }, 3 },
        { "123c", 4, TestType{ 123.0 }, 3},
        { ".5", 2, TestType{ 0.5 }, 2 },
        { "108.", 4, TestType{ 108.0 }, 4 },
        { "108.5", 4, TestType{ 108.0 }, 4},
        { "23.5", 4, TestType{ 23.5 }, 4 },
        { "132e", 4, TestType{ 132 }, 3 },
        { "14e2", 4, TestType{ 1400.0 }, 4 },
        { "92.e1", 5, TestType{ 920.0 }, 5 },
        { "1.4e2", 5, TestType{ 140.0 }, 5 },
        { "14E+2", 5, TestType{ 1400.0 }, 5 },
        { ".5e3", 4, TestType{ 500.0 }, 4 },
        { "92.e+1", 6, TestType{ 920.0 }, 6 },
        { "1.4E+2", 6, TestType{ 140.0 }, 6 },
        { "5e-1", 4, TestType{ 0.5 }, 4 },
        { "5.e-1", 5, TestType{ 0.5 }, 5 },
        { "2.5e-1", 6, TestType{ 0.25 }, 6 },
        { "-123", 4, TestType{ -123.0 }, 4 },
        { "-123", 3, TestType{ -12.0 }, 3 },
        { "-108.", 5, TestType{ -108.0 }, 5 },
        { "-23.5", 5, TestType{ -23.5 }, 5 },
        { "-14e2", 5, TestType{ -1400.0 }, 5 },
        { "-14e25", 5, TestType{ -1400.0 }, 5 },
        { "-92.e1", 6, TestType{ -920.0 }, 6 },
        { "-1.4E2", 6, TestType{ -140.0 }, 6 },
        { "-14e+2", 6, TestType{ -1400.0 }, 6 },
        { "-92.e+1", 7, TestType{ -920.0 }, 7 },
        { "-1.4E+2", 7, TestType{ -140.0 }, 7 },
        { "-5e-1", 5, TestType{ -0.5 }, 5 },
        { "-5.E-1", 6, TestType{ -0.5 }, 6 },
        { "-2.5e-1", 7, TestType{ -0.25 }, 7 },
        { "inf", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "Inf", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "INF", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "infi", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "Infi", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "INFI", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "infinity", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "Infinity", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "INFINITY", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "-inf", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-Inf", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-INF", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-infi", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-Infi", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-INFI", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-infinity", 9, -std::numeric_limits<TestType>::infinity(), 9 },
        { "-Infinity", 9, -std::numeric_limits<TestType>::infinity(), 9 },
        { "-INFINITY", 9, -std::numeric_limits<TestType>::infinity(), 9 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::general);
        REQUIRE(result.ec == std::errc{});
        REQUIRE(actual == example.expected);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_CASE_TEMPLATE("Floating point general format: negative zero", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "-0", 2, TestType{ -0.0 }, 2 },
        { "-0.", 3, TestType{ -0.0 }, 3 },
        { "-0.0", 4, TestType{ -0.0 }, 4 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::general);
        REQUIRE(result.ec == std::errc{});
        REQUIRE(actual == -0.0f);
        float signum = std::copysign(1.0f, actual);
        REQUIRE(signum == -1.0f);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_CASE_TEMPLATE("Floating point general format: NaN", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "nan", 3, std::numeric_limits<TestType>::quiet_NaN(), 3 },
        { "NaN", 3, std::numeric_limits<TestType>::quiet_NaN(), 3 },
        { "NAN", 3, std::numeric_limits<TestType>::quiet_NaN(), 3 },
        { "naN()", 5, std::numeric_limits<TestType>::quiet_NaN(), 5 },
        { "nAn(abc)", 8, std::numeric_limits<TestType>::quiet_NaN(), 8 },
        { "nan(abcd", 8, std::numeric_limits<TestType>::quiet_NaN(), 3 },
        { "NaN(a^c)", 8, std::numeric_limits<TestType>::quiet_NaN(), 3 },
        { "-naN()", 6, std::numeric_limits<TestType>::quiet_NaN(), 6 },
        { "-nAn(abc)", 9, std::numeric_limits<TestType>::quiet_NaN(), 9 },
        { "-nan(abcd", 9, std::numeric_limits<TestType>::quiet_NaN(), 4 },
        { "-NaN(a^c)", 9, std::numeric_limits<TestType>::quiet_NaN(), 4 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::general);
        REQUIRE(result.ec == std::errc{});
        REQUIRE(std::isnan(actual));
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_CASE_TEMPLATE("Floating point fixed format: successful", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "123", 3, TestType{ 123.0 }, 3 },
        { "1234", 3, TestType{ 123.0 }, 3 },
        { "123c", 4, TestType{ 123.0 }, 3},
        { ".5", 2, TestType{ 0.5 }, 2 },
        { "108.", 4, TestType{ 108.0 }, 4 },
        { "108.5", 4, TestType{ 108.0 }, 4},
        { "23.5", 4, TestType{ 23.5 }, 4 },
        { "14e2", 4, TestType{ 14.0 }, 2 },
        { ".5e3", 4, TestType{ 0.5 }, 2 },
        { "92.e1", 5, TestType{ 92.0 }, 3 },
        { "1.5E2", 5, TestType{ 1.5 }, 3 },
        { "14e+2", 5, TestType{ 14.0 }, 2 },
        { "92.e+1", 6, TestType{ 92.0 }, 3 },
        { "1.5E+2", 6, TestType{ 1.5 }, 3 },
        { "5e-1", 4, TestType{ 5.0 }, 1 },
        { "5.e-1", 5, TestType{ 5.0 }, 2 },
        { "2.5E-1", 6, TestType{ 2.5 }, 3 },
        { "-123", 4, TestType{ -123.0 }, 4 },
        { "-123", 3, TestType{ -12.0 }, 3 },
        { "-108.", 5, TestType{ -108.0 }, 5 },
        { "-23.5", 5, TestType{ -23.5 }, 5 },
        { "-14e2", 5, TestType{ -14.0 }, 3 },
        { "-92.e1", 6, TestType{ -92.0 }, 4 },
        { "-1.5E2", 6, TestType{ -1.5 }, 4 },
        { "-14e+2", 6, TestType{ -14.0 }, 3 },
        { "-92.e+1", 7, TestType{ -92.0 }, 4 },
        { "-1.5e+2", 7, TestType{ -1.5 }, 4 },
        { "-5e-1", 5, TestType{ -5.0 }, 2 },
        { "-5.e-1", 6, TestType{ -5.0 }, 3 },
        { "-2.5e-1", 7, TestType{ -2.5 }, 4 },
        { "inf", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "Inf", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "INF", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "infi", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "Infi", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "INFI", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "infinity", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "Infinity", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "INFINITY", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "-inf", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-Inf", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-INF", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-infi", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-Infi", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-INFI", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-infinity", 9, -std::numeric_limits<TestType>::infinity(), 9 },
        { "-Infinity", 9, -std::numeric_limits<TestType>::infinity(), 9 },
        { "-INFINITY", 9, -std::numeric_limits<TestType>::infinity(), 9 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::fixed);
        REQUIRE(result.ec == std::errc{});
        REQUIRE(actual == example.expected);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_CASE_TEMPLATE("Floating point scientific format: successful", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "14e2", 4, TestType { 1400.0 }, 4 },
        { "92.E1", 5, TestType{ 920.0 }, 5 },
        { "1.4e2", 5, TestType{ 140.0 }, 5 },
        { "14e+2", 5, TestType{ 1400.0 }, 5 },
        { "92.e+1", 6, TestType{ 920.0 }, 6 },
        { "1.4e+2", 6, TestType{ 140.0 }, 6 },
        { "5e-1", 4, TestType{ 0.5 }, 4 },
        { "5.e-1", 5, TestType{ 0.5 }, 5 },
        { "2.5e-1", 6, TestType{ 0.25 }, 6 },
        { "-14e2", 5, TestType{ -1400.0 }, 5 },
        { ".5e3", 4, TestType{ 500.0 }, 4 },
        { "-14e25", 5, TestType{ -1400.0 }, 5 },
        { "-92.e1", 6, TestType{ -920.0 }, 6 },
        { "-1.4E2", 6, TestType{ -140.0 }, 6 },
        { "-14e+2", 6, TestType{ -1400.0 }, 6 },
        { "-92.e+1", 7, TestType{ -920.0 }, 7 },
        { "-1.4E+2", 7, TestType{ -140.0 }, 7 },
        { "-5e-1", 5, TestType{ -0.5 }, 5 },
        { "-5.E-1", 6, TestType{ -0.5 }, 6 },
        { "-2.5E-1", 7, TestType{ -0.25 }, 7 },
        { "inf", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "Inf", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "INF", 3, std::numeric_limits<TestType>::infinity(), 3 },
        { "infi", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "Infi", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "INFI", 4, std::numeric_limits<TestType>::infinity(), 3 },
        { "infinity", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "Infinity", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "INFINITY", 8, std::numeric_limits<TestType>::infinity(), 8 },
        { "-inf", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-Inf", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-INF", 4, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-infi", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-Infi", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-INFI", 5, -std::numeric_limits<TestType>::infinity(), 4 },
        { "-infinity", 9, -std::numeric_limits<TestType>::infinity(), 9 },
        { "-Infinity", 9, -std::numeric_limits<TestType>::infinity(), 9 },
        { "-INFINITY", 9, -std::numeric_limits<TestType>::infinity(), 9 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::scientific);
        REQUIRE(result.ec == std::errc{});
        REQUIRE(actual == example.expected);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_CASE_TEMPLATE("Floating point scientific format: missing exponential", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "123", 3, TestType{ 123.0 }, 3 },
        { "1234", 3, TestType{ 123.0 }, 3 },
        { "123c", 4, TestType{ 123.0 }, 3},
        { ".5", 2, TestType{ 0.5 }, 2 },
        { "108.", 4, TestType{ 108.0 }, 4 },
        { "108.5", 4, TestType{ 108.0 }, 4},
        { "23.5", 4, TestType{ 23.5 }, 4 },
        { "-123", 4, TestType{ -123.0 }, 4 },
        { "-123", 3, TestType{ -12.0 }, 3 },
        { "-108.", 5, TestType{ -108.0 }, 5 },
        { "-23.5", 5, TestType{ -23.5 }, 5 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::scientific);
        REQUIRE(result.ec == std::errc::invalid_argument);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == 0);
    }
}

TEST_CASE_TEMPLATE("Hexadecimal floating point", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "1b", 2, TestType{ 27 }, 2 },
        { ".8", 2, TestType{ 0.5 }, 2 },
        { "1.c", 3, TestType{ 1.75 }, 3 },
        { "1.3eP10", 7, TestType{ 1272 }, 7 },
        { "1.3Ep+10", 8, TestType{ 1272 }, 8 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::hex);
        REQUIRE(result.ec == std::errc{});
        REQUIRE(actual == example.expected);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_CASE_TEMPLATE("Floating point format failures", TestType, float, double, long double)
{
    TestCase<TestType> examples[] = {
        { "  0.5", 5, TestType{ }, 0 },
        { ".e3", 3, TestType{ }, 0 },
        { "+23", 3, TestType{ }, 0 },
        { "N", 1, TestType{ }, 0 },
        { "NA", 2, TestType{ }, 0 },
        { "N/A", 3, TestType{ }, 0 },
        { "in", 2, TestType{ }, 0 },
        { "-N/A", 4, TestType{ }, 0 },
        { "-in", 3, TestType{ }, 0 },
    };

    for (const auto& example : examples)
    {
        TestType actual;
        auto result = celcompat::from_chars(example.source, example.source + example.size, actual, celcompat::chars_format::general);
        REQUIRE(result.ec == std::errc::invalid_argument);
        auto length = static_cast<std::size_t>(result.ptr - example.source);
        REQUIRE(length == example.length);
    }
}

TEST_SUITE_END();
