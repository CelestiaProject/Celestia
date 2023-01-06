// parser.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
#include <map>
#include <string_view>
#include <utility>
#include <variant>

#include <celutil/tokenizer.h>
#include "astro.h"
#include "parser.h"

using namespace std::string_view_literals;


namespace
{

using MeasurementUnit = std::variant<std::monostate,
                                     astro::LengthUnit,
                                     astro::TimeUnit,
                                     astro::AngleUnit,
                                     astro::MassUnit>;

class UnitsVisitor
{
public:
    explicit UnitsVisitor(Value::Units* _units) : units(_units) {}

    void operator()(std::monostate) const { /* nothing to update */ }
    void operator()(astro::LengthUnit unit) const { units->length = unit; }
    void operator()(astro::TimeUnit unit) const { units->time = unit; }
    void operator()(astro::AngleUnit unit) const { units->angle = unit; }
    void operator()(astro::MassUnit unit) const { units->mass = unit; }

private:
    Value::Units* units;
};


MeasurementUnit parseUnit(std::string_view name)
{
    static const std::map<std::string_view, MeasurementUnit>* unitMap = nullptr;

    if (!unitMap)
    {
        unitMap = new std::map<std::string_view, MeasurementUnit>
        {
            { "km"sv, astro::LengthUnit::Kilometer },
            { "m"sv, astro::LengthUnit::Meter },
            { "rE"sv, astro::LengthUnit::EarthRadius },
            { "rJ"sv, astro::LengthUnit::JupiterRadius },
            { "rS"sv, astro::LengthUnit::SolarRadius },
            { "AU"sv, astro::LengthUnit::AstronomicalUnit },
            { "ly"sv, astro::LengthUnit::LightYear },
            { "pc"sv, astro::LengthUnit::Parsec },
            { "kpc"sv, astro::LengthUnit::Kiloparsec },
            { "Mpc"sv, astro::LengthUnit::Megaparsec },

            { "s"sv, astro::TimeUnit::Second },
            { "min"sv, astro::TimeUnit::Minute },
            { "h"sv, astro::TimeUnit::Hour },
            { "d"sv, astro::TimeUnit::Day },
            { "y"sv, astro::TimeUnit::JulianYear },

            { "mas"sv, astro::AngleUnit::Milliarcsecond },
            { "arcsec"sv, astro::AngleUnit::Arcsecond },
            { "arcmin"sv, astro::AngleUnit::Arcminute },
            { "deg"sv, astro::AngleUnit::Degree },
            { "hRA"sv, astro::AngleUnit::Hour },
            { "rad"sv, astro::AngleUnit::Radian },

            { "kg"sv, astro::MassUnit::Kilogram },
            { "mE"sv, astro::MassUnit::EarthMass },
            { "mJ"sv, astro::MassUnit::JupiterMass },
        };
    }

    auto it = unitMap->find(name);
    return it == unitMap->end()
        ? MeasurementUnit(std::in_place_type<std::monostate>)
        : it->second;
}


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

        MeasurementUnit unit = parseUnit(*tokenValue);
        std::visit(visitor, unit);
    }

    return units;
}

} // end unnamed namespace


/****** Parser method implementation ******/

Parser::Parser(Tokenizer* _tokenizer) :
    tokenizer(_tokenizer)
{
}


std::unique_ptr<ValueArray> Parser::readArray()
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


std::unique_ptr<Hash> Parser::readHash()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginGroup)
    {
        tokenizer->pushBack();
        return nullptr;
    }

    auto hash = std::make_unique<Hash>();

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


Value Parser::readValue()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    switch (tok)
    {
    case Tokenizer::TokenNumber:
        return Value(*tokenizer->getNumberValue());

    case Tokenizer::TokenString:
        return Value(*tokenizer->getStringValue());

    case Tokenizer::TokenName:
        if (tokenizer->getNameValue() == "false")
            return Value(false);
        else if (tokenizer->getNameValue() == "true")
            return Value(true);
        else
        {
            tokenizer->pushBack();
            return Value();
        }

    case Tokenizer::TokenBeginArray:
        tokenizer->pushBack();
        {
            std::unique_ptr<ValueArray> array = readArray();
            if (array == nullptr)
                return Value();
            else
                return Value(std::move(array));
        }

    case Tokenizer::TokenBeginGroup:
        tokenizer->pushBack();
        {
            std::unique_ptr<Hash> hash = readHash();
            if (hash == nullptr)
                return Value();
            else
                return Value(std::move(hash));
        }

    default:
        tokenizer->pushBack();
        return Value();
    }
}
