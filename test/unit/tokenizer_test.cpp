#include <sstream>

#include <celutil/tokenizer.h>
#include <celutil/utf8.h>

#include <catch.hpp>

TEST_CASE("Tokenizer parses names", "[Tokenizer]")
{
    SECTION("Separated names")
    {
        std::istringstream input("Normal "
                                "Number2 "
                                "Number3Number "
                                "snake_case "
                                "_prefixed");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "Normal");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "Number2");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "Number3Number");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "snake_case");

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "_prefixed");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SECTION("Followed by units")
    {
        std::istringstream input("Quantity<unit>");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "Quantity");

        REQUIRE(tok.nextToken() == Tokenizer::TokenBeginUnits);

        REQUIRE(tok.nextToken() == Tokenizer::TokenName);
        REQUIRE(tok.getStringValue() == "unit");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEndUnits);
        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }
}

TEST_CASE("Tokenizer parses strings", "[Tokenizer]")
{
    SECTION("ASCII strings")
    {
        std::istringstream input("\"abc 123.456 {}<>\" "
                                 "\"\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "abc 123.456 {}<>");

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue().empty());

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SECTION("Standard escapes")
    {
        std::istringstream input("\"abc\\\\def\\nghi\\\"jkl\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenString);
        REQUIRE(tok.getStringValue() == "abc\\def\nghi\"jkl");

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SECTION("Unicode escapes")
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

    SECTION("Invalid escape")
    {
        std::istringstream input("\"abcdefghijklmnop\\qrstuvwxyz\"");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenError);
    }

    SECTION("UTF-8 sequences")
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
}

TEST_CASE("Tokenizer parses numbers", "[Tokenizer]")
{
    SECTION("No leading sign")
    {
        std::istringstream input("12345 "
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
        REQUIRE(tok.isInteger());
        REQUIRE(tok.getNumberValue() == 12345.0);
        REQUIRE(tok.getIntegerValue() == 12345);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 12345.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 32.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 1200000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 2300000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 0.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 1200000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 2300000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 0.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SECTION("Explicit positive sign")
    {
        std::istringstream input("+12345 "
                                 "+12345.0 "
                                 "+32.75 "
                                 "+1.2e6 "
                                 "+2.3e+6 "
                                 "+7.5e-1");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.isInteger());
        REQUIRE(tok.getNumberValue() == 12345.0);
        REQUIRE(tok.getIntegerValue() == 12345);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 12345.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 32.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 1200000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 2300000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == 0.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SECTION("Negative sign")
    {
        std::istringstream input("-12345 "
                                 "-12345.0 "
                                 "-32.75 "
                                 "-1.2e6 "
                                 "-2.3e+6 "
                                 "-7.5e-1");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.isInteger());
        REQUIRE(tok.getNumberValue() == -12345.0);
        REQUIRE(tok.getIntegerValue() == -12345);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == -12345.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == -32.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == -1200000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == -2300000.0);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(!tok.isInteger());
        REQUIRE(tok.getNumberValue() == -0.75);

        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }

    SECTION("Invalid numbers")
    {
        std::string tests[] = {
            "+",
            "-",
            "+e",
            "+E",
            "-e",
            "-E",
            "1.23e",
            "1.23E",
            "1.23e+",
            "1.23e-",
        };

        for (const auto& test : tests)
        {
            std::istringstream input(test);
            Tokenizer tok(&input);
            REQUIRE(tok.nextToken() == Tokenizer::TokenError);
        }
    }

    SECTION("Ending separator")
    {
        std::istringstream input("123{");
        Tokenizer tok(&input);

        REQUIRE(tok.nextToken() == Tokenizer::TokenNumber);
        REQUIRE(tok.isInteger());
        REQUIRE(tok.getNumberValue() == 123.0);
        REQUIRE(tok.getIntegerValue() == 123);

        REQUIRE(tok.nextToken() == Tokenizer::TokenBeginGroup);
        REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
    }
}

TEST_CASE("Tokenizer parses symbols and groups", "[Tokenizer]")
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

TEST_CASE("Tokenizer skips comments", "[Tokenizer]")
{
    std::istringstream input("Token1 # comment\n"
                             "Token2 # \300\n"
                             "Token3 # blah");
    Tokenizer tok(&input);

    REQUIRE(tok.nextToken() == Tokenizer::TokenName);
    REQUIRE(tok.getStringValue() == "Token1");

    REQUIRE(tok.nextToken() == Tokenizer::TokenName);
    REQUIRE(tok.getStringValue() == "Token2");

    REQUIRE(tok.nextToken() == Tokenizer::TokenName);
    REQUIRE(tok.getStringValue() == "Token3");

    REQUIRE(tok.nextToken() == Tokenizer::TokenEnd);
}
