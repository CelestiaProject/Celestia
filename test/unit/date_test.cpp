#include <array>
#include <string_view>

#include <doctest.h>

#include <celastro/date.h>

using namespace std::string_view_literals;

TEST_SUITE_BEGIN("Date tests");

TEST_CASE("Date parsing (success)")
{
    struct DateTest
    {
        std::string_view input;
        int year;
        int month;
        int day;
        int hour;
        int minute;
        double seconds;
    };

    std::array testCases
    {
        DateTest { "2026-10-24"sv, 2026, 10, 24, 0, 0, 0 },
        DateTest { "2026-10-24T15:04:31.5"sv, 2026, 10, 24, 15, 4, 31.5 },
        DateTest { "2026-10-24T15:04:31.5Z"sv, 2026, 10, 24, 15, 4, 31.5 },
        DateTest { "2026-10-24T15:04:31.5+00:00"sv, 2026, 10, 24, 15, 4, 31.5 },
        DateTest { "  2026-10-24T09:35:21.25"sv, 2026, 10, 24, 9, 35, 21.25 },
        DateTest { "2000\t02\t29"sv, 2000, 2, 29, 0, 0, 0 },
        DateTest { "-350\t5\t15 11:15"sv, -350, 5, 15, 11, 15, 0 },
        DateTest { "1900\t2 28 15:20:04.75"sv, 1900, 2, 28, 15, 20, 4.75 },
    };

    for (const auto& testCase : testCases)
    {
        celestia::astro::Date actual;
        bool success = celestia::astro::parseDate(testCase.input, actual);
        REQUIRE(success);
        if (success)
        {
            REQUIRE(actual.year == testCase.year);
            REQUIRE(actual.month == testCase.month);
            REQUIRE(actual.day == testCase.day);
            REQUIRE(actual.hour == testCase.hour);
            REQUIRE(actual.minute == testCase.minute);
            REQUIRE(actual.seconds == testCase.seconds);
        }
    }
}

TEST_CASE("Date parsing (failures)")
{
    std::array testCases
    {
        "2026-13-05"sv,
        "2026-13-05T15:04:31.5"sv,
        "2026-09-15T36:05:11.2"sv,
        "1900-02-29T12:00:00"sv,
        "1900 02 29 12:00:00"sv,
        "1900 2 28 xkjlyz"sv,
    };

    for (const auto& testCase : testCases)
    {
        celestia::astro::Date actual;
        REQUIRE(!celestia::astro::parseDate(testCase, actual));
    }
}

TEST_SUITE_END();
