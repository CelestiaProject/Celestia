#include <cmath>
#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>

#include <celutil/tokenizer.h>
#include <celutil/utf8.h>

#include <doctest.h>

TEST_SUITE_BEGIN("Tokenizer");

TEST_CASE("Tokenizer parses names")
{
    SUBCASE("Separated names")
    {
        std::istringstream input("Normal "
                                "Number2 "
                                "Number3Number "
                                "snake_case "
                                "_prefixed");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Normal");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Number2");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Number3Number");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "snake_case");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "_prefixed");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Followed by units")
    {
        std::istringstream input("Quantity<unit>");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Quantity");

        REQUIRE(tok.nextToken() == Tokenizer::TokenBeginUnits);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "unit");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEndUnits);
        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Buffer boundary")
    {
        std::string tests[] = {
            "Foo"
            "Foo2",
            "_Foo",
            "Foo_",
        };

        for (const auto& input : tests)
        {
            for (std::size_t spaces = 0; spaces < 10; ++spaces)
            {
                std::string spacedStr(spaces, ' ');
                spacedStr.append(input);

                std::istringstream in(spacedStr);
                Tokenizer tok(&in, 8);

                REQUIRE(tok.nextToken() == Tokenizer::TokenName);
                REQUIRE(tok.getNameValue() == input);
            }
        }
    }
}

TEST_CASE("Tokenizer parses strings")
{
    SUBCASE("ASCII strings")
    {
        std::istringstream input("\"abc 123.456 {}<>\" "
                                 "\"\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "abc 123.456 {}<>");

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue().value().empty());

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Standard escapes")
    {
        std::istringstream input("\"abc\\\\def\\nghi\\\"jkl\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "abc\\def\nghi\"jkl");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Unicode escapes")
    {
        std::istringstream input("\"\\u00ef\" "
                                 "\"\\u0900\" "
                                 "\"\\udabc\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "\303\257");

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "\340\244\200");

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == UTF8_REPLACEMENT_CHAR);

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Invalid escape")
    {
        std::istringstream input("\"abcdefghijklmnop\\qrstuvwxyz\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenError);
    }

    SUBCASE("UTF-8 sequences")
    {
        std::istringstream input("\"\303\257\340\244\200\" "
                                 "\"\300\" "
                                 "\"\303x\" "
                                 "\"\340\240x\" "
                                 "\"\340x\260\" "
                                 "\"\303\257\340\240x\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "\303\257\340\244\200");

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == UTF8_REPLACEMENT_CHAR);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == UTF8_REPLACEMENT_CHAR);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == UTF8_REPLACEMENT_CHAR);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == UTF8_REPLACEMENT_CHAR UTF8_REPLACEMENT_CHAR);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "\303\257" UTF8_REPLACEMENT_CHAR);

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Buffer boundary")
    {
        std::tuple<std::string, std::string> tests[] = {
            {"\"\"", ""},
            {"\"abc\"", "abc"},
            {"\"a\\\\b\"", "a\\b"},
            {"\"a\\\"b\"", "a\"b"},
            {"\"a\u0042c\"", "aBc"},
            {"\"\303\257\340\244\200\"", "\303\257\340\244\200"},
        };

        for (const auto& [input, expected] : tests)
        {
            for (std::size_t spaces = 0; spaces < 10; ++spaces)
            {
                std::string spacedStr(spaces, ' ');
                spacedStr.append(input);

                std::istringstream in(spacedStr);
                Tokenizer tok(&in, 8);

                REQUIRE(tok.nextToken() == Tokenizer::TokenString);
                REQUIRE(tok.getStringValue() == expected);
            }
        }
    }
}

TEST_CASE("Tokenizer parses numbers")
{
    SUBCASE("No leading sign")
    {
        std::istringstream input("0 "
                                 "0.0 "
                                 "12345 "
                                 "12345.0 "
                                 "32.75 "
                                 "1.2e6 "
                                 "2.3e+6 "
                                 "7.5e-1 "
                                 "1.2E6 "
                                 "2.3E+6 "
                                 "7.5E-1 ");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.0);
        REQUIRE(tok.getIntegerValue() == 0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 12345.0);
        REQUIRE(tok.getIntegerValue() == 12345);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 12345.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 32.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 1200000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 2300000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 1200000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 2300000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Explicit positive sign")
    {
        std::istringstream input("+0"
                                 "+0.0"
                                 "+12345 "
                                 "+12345.0 "
                                 "+32.75 "
                                 "+1.2e6 "
                                 "+2.3e+6 "
                                 "+7.5e-1");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.0);
        REQUIRE(tok.getIntegerValue() == 0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 12345.0);
        REQUIRE(tok.getIntegerValue() == 12345);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 12345.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 32.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 1200000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 2300000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 0.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Negative sign")
    {
        std::istringstream input("-0"
                                 "-0.0"
                                 "-12345 "
                                 "-12345.0 "
                                 "-32.75 "
                                 "-1.2e6 "
                                 "-2.3e+6 "
                                 "-7.5e-1");
        Tokenizer tok(&input);

        // negative zero is not considered to be an integer value
        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        std::optional<double> numberValue = tok.getNumberValue();
        REQUIRE(numberValue.has_value());
        REQUIRE(numberValue.value() == -0.0);
        REQUIRE(std::signbit(numberValue.value()));
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        numberValue = tok.getNumberValue();
        REQUIRE(numberValue.has_value());
        REQUIRE(numberValue.value() == -0.0);
        REQUIRE(std::signbit(numberValue.value()));
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == -12345.0);
        REQUIRE(tok.getIntegerValue() == -12345);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == -12345.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == -32.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == -1200000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == -2300000.0);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == -0.75);
        REQUIRE(!tok.getIntegerValue().has_value());

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Buffer boundary")
    {
        std::tuple<std::string, double, bool> tests[] = {
            { "1", 1.0, true },
            { "-1", -1.0, true },
            { "123", 123.0, true },
            { "-123", -123.0, true },
            { "1.", 1.0, false },
            { "-1.", -1.0, false },
            { ".5", 0.5, false },
            { "-.5", -0.5, false },
            { "1e0", 1.0, false },
            { "1E0", 1.0, false },
            { "1e1", 10.0, false },
            { "1e+0", 1.0, false },
            { "1E+0", 1.0, false },
            { "1e+1", 10.0, false },
            { "5e-1", 0.5, false },
            { "-1e0", -1.0, false },
            { "-1E0", -1.0, false },
            { "-1e1", -10.0, false },
            { "-1e+0", -1.0, false },
            { "-1E+0", -1.0, false },
            { "-1e+1", -10.0, false },
            { "-5e-1", -0.5, false },
        };

        for (auto [input, value, isInteger] : tests)
        {
            for (std::size_t spaces = 0; spaces < 10; ++spaces)
            {
                std::string spacedStr(spaces, ' ');
                spacedStr.append(input);

                std::istringstream in(spacedStr);
                Tokenizer tok(&in, 8);

                REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
                REQUIRE(tok.getNumberValue() == value);
                REQUIRE(tok.getIntegerValue().has_value() == isInteger);
            }
        }
    }

    SUBCASE("Invalid numbers")
    {
        std::string tests[] = {
            "+",
            "-",
            "+e",
            "+E",
            "-e",
            "-E",
        };

        for (const auto& test : tests)
        {
            std::istringstream input(test);
            Tokenizer tok(&input);
            REQUIRE(tok.nextToken() == Tokenizer::TokenError);
        }
    }

    SUBCASE("Trailing exponents")
    {
        std::string tests[] = {
            "1.25e",
            "1.25E",
            "1.25e+",
            "1.25e-",
            "1.25E+",
            "1.25E-",
        };

        for (const auto& test : tests)
        {
            std::istringstream input(test);
            Tokenizer tok(&input);
            REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
            REQUIRE(tok.getNumberValue() == 1.25);
            REQUIRE(!tok.getIntegerValue().has_value());

            // Exponent is treated as a name token (e or E)
            REQUIRE(tok.nextToken() == Tokenizer::TokenName);
            REQUIRE(tok.getNameValue() == std::string_view(test.data() + 4, 1));
        }
    }

    SUBCASE("Ending separator")
    {
        std::istringstream input("123{");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.getNumberValue() == 123.0);
        REQUIRE(tok.getIntegerValue() == 123);

        REQUIRE(tok.nextToken() == Tokenizer::TokenBeginGroup);
        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }
}

TEST_CASE("Tokenizer parses symbols and groups")
{
    std::istringstream input("={}|[]<>");
    Tokenizer tok(&input);

    REQUIRE(tok.nextToken() == Tokenizer::TokenEquals);
    REQUIRE(tok.nextToken() == Tokenizer::TokenBeginGroup);
    REQUIRE(tok.nextToken() == Tokenizer::TokenEndGroup);
    REQUIRE(tok.nextToken() == Tokenizer::TokenBar);
    REQUIRE(tok.nextToken() == Tokenizer::TokenBeginArray);
    REQUIRE(tok.nextToken() == Tokenizer::TokenEndArray);
    REQUIRE(tok.nextToken() == Tokenizer::TokenBeginUnits);
    REQUIRE(tok.nextToken() == Tokenizer::TokenEndUnits);
    REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
}

TEST_CASE("Tokenizer skips comments")
{
    SUBCASE("Comment within buffer")
    {
        std::istringstream input("Token1 # comment\n"
                                "Token2 # \300\n"
                                "Token3 # blah");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Token1");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Token2");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getNameValue() == "Token3");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SUBCASE("Buffer boundary")
    {
        for (std::size_t spaces = 0; spaces < 10; ++spaces)
        {
            std::string spacedStr(spaces, ' ');
            spacedStr.append("# really long comment here\n"
                             "{");

            std::istringstream in(spacedStr);
            Tokenizer tok(&in, 8);

            REQUIRE(tok.nextToken() == Tokenizer::TokenBeginGroup);
        }
    }
}

TEST_SUITE_END();
