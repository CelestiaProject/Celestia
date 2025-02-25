// parser.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "parser.h"

#include <variant>

#include <celastro/units.h>
#include "associativearray.h"
#include "tokenizer.h"

using namespace std::string_view_literals;

namespace celestia::util
{

namespace
{

using MeasurementUnit = std::variant<astro::LengthUnit,
                                     astro::TimeUnit,
                                     astro::AngleUnit,
                                     astro::MassUnit>;

class UnitsVisitor
{
public:
    explicit UnitsVisitor(Value::Units* _units) : units(_units) {}

    void operator()(astro::LengthUnit unit) const { units->length = unit; }
    void operator()(astro::TimeUnit unit) const { units->time = unit; }
    void operator()(astro::AngleUnit unit) const { units->angle = unit; }
    void operator()(astro::MassUnit unit) const { units->mass = unit; }

private:
    Value::Units* units;
};

#include "parser.inc"

Value::Units readUnits(Tokenizer& tokenizer)
{
    Value::Units units;

    if (tokenizer.nextToken() != Tokenizer::TokenBeginUnits)
    {
        tokenizer.pushBack();
        return units;
    }

    UnitsVisitor visitor(&units);

    while (tokenizer.nextToken() != Tokenizer::TokenEndUnits)
    {
        auto tokenValue = tokenizer.getNameValue();
        if (!tokenValue.has_value()) { continue; }

        if (const auto* unitEntry = UnitsMap::parseUnit(tokenValue->data(), tokenValue->size());
            unitEntry != nullptr)
        {
            std::visit(visitor, unitEntry->unit);
        }
    }

    return units;
}

} // end unnamed namespace

/****** Parser method implementation ******/

Parser::Parser(Tokenizer* _tokenizer) :
    tokenizer(_tokenizer)
{
}

std::unique_ptr<ValueArray>
Parser::readArray()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginArray)
    {
        tokenizer->pushBack();
        return nullptr;
    }

    auto array = std::make_unique<ValueArray>();

    Value v = readValue();
    while (!v.isNull())
    {
        array->push_back(std::move(v));
        v = readValue();
    }

    tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenEndArray)
    {
        tokenizer->pushBack();
        return nullptr;
    }

    return array;
}

std::unique_ptr<AssociativeArray>
Parser::readHash()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginGroup)
    {
        tokenizer->pushBack();
        return nullptr;
    }

    auto hash = std::make_unique<AssociativeArray>();

    tok = tokenizer->nextToken();
    while (tok != Tokenizer::TokenEndGroup)
    {
        std::string name;
        if (auto tokenValue = tokenizer->getNameValue(); tokenValue.has_value())
        {
            name = *tokenValue;
        }
        else
        {
            tokenizer->pushBack();
            return nullptr;
        }

        Value::Units units = readUnits(*tokenizer);

        Value value = readValue();
        if (value.isNull())
        {
            return nullptr;
        }

        value.setUnits(units);
        hash->addValue(std::move(name), std::move(value));

        tok = tokenizer->nextToken();
    }

    return hash;
}

Value
Parser::readValue()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    switch (tok)
    {
    case Tokenizer::TokenNumber:
        return Value(*tokenizer->getNumberValue());

    case Tokenizer::TokenString:
        return Value(*tokenizer->getStringValue());

    case Tokenizer::TokenName:
        if (auto name = *tokenizer->getNameValue(); name == "false"sv)
            return Value(false);
        else if (name == "true"sv)
            return Value(true);

        tokenizer->pushBack();
        return Value();

    case Tokenizer::TokenBeginArray:
        tokenizer->pushBack();
        if (std::unique_ptr<ValueArray> array = readArray(); array != nullptr)
            return Value(std::move(array));
        return Value();

    case Tokenizer::TokenBeginGroup:
        tokenizer->pushBack();
        if (std::unique_ptr<AssociativeArray> hash = readHash(); hash != nullptr)
            return Value(std::move(hash));
        return Value();

    default:
        tokenizer->pushBack();
        return Value();
    }
}

} // end namespace celestia::util
