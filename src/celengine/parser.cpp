// parser.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/tokenizer.h>
#include "astro.h"
#include "parser.h"


/****** Parser method implementation ******/

Parser::Parser(Tokenizer* _tokenizer) :
    tokenizer(_tokenizer)
{
}


ValueArray* Parser::readArray()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginArray)
    {
        tokenizer->pushBack();
        return nullptr;
    }

    ValueArray* array = new ValueArray();

    Value* v = readValue();
    while (v != nullptr)
    {
        array->push_back(v);
        v = readValue();
    }

    tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenEndArray)
    {
        tokenizer->pushBack();
        delete array;
        return nullptr;
    }

    return array;
}


Hash* Parser::readHash()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginGroup)
    {
        tokenizer->pushBack();
        return nullptr;
    }

    auto* hash = new Hash();

    tok = tokenizer->nextToken();
    while (tok != Tokenizer::TokenEndGroup)
    {
        if (tok != Tokenizer::TokenName)
        {
            tokenizer->pushBack();
            delete hash;
            return nullptr;
        }
        std::string name(tokenizer->getStringValue());

#ifndef USE_POSTFIX_UNITS
        readUnits(name, hash);
#endif

        Value* value = readValue();
        if (value == nullptr)
        {
            delete hash;
            return nullptr;
        }

        hash->addValue(name, *value);

#ifdef USE_POSTFIX_UNITS
        readUnits(name, hash);
#endif

        tok = tokenizer->nextToken();
    }

    return hash;
}


/**
 * Reads a units section into the hash.
 * @param[in] propertyName Name of the current property.
 * @param[in] hash Hash to add units quantities into.
 * @return True if a units section was successfully read, false otherwise.
 */
bool Parser::readUnits(const std::string& propertyName, Hash* hash)
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    if (tok != Tokenizer::TokenBeginUnits)
    {
        tokenizer->pushBack();
        return false;
    }

    tok = tokenizer->nextToken();
    while (tok != Tokenizer::TokenEndUnits)
    {
        if (tok != Tokenizer::TokenName)
        {
            tokenizer->pushBack();
            return false;
        }

        std::string_view unit = tokenizer->getStringValue();
        Value* value = new Value(unit);

        if (astro::isLengthUnit(unit))
        {
            std::string keyName(propertyName + "%Length");
            hash->addValue(keyName, *value);
        }
        else if (astro::isTimeUnit(unit))
        {
            std::string keyName(propertyName + "%Time");
            hash->addValue(keyName, *value);
        }
        else if (astro::isAngleUnit(unit))
        {
            std::string keyName(propertyName + "%Angle");
            hash->addValue(keyName, *value);
        }
        else if (astro::isMassUnit(unit))
        {
            std::string keyName(propertyName + "%Mass");
            hash->addValue(keyName, *value);
        }
        else
        {
            delete value;
            return false;
        }

        tok = tokenizer->nextToken();
    }

    return true;
}


Value* Parser::readValue()
{
    Tokenizer::TokenType tok = tokenizer->nextToken();
    switch (tok)
    {
    case Tokenizer::TokenNumber:
        return new Value(*tokenizer->getNumberValue());

    case Tokenizer::TokenString:
        return new Value(tokenizer->getStringValue());

    case Tokenizer::TokenName:
        if (tokenizer->getStringValue() == "false")
            return new Value(false);
        else if (tokenizer->getStringValue() == "true")
            return new Value(true);
        else
        {
            tokenizer->pushBack();
            return nullptr;
        }

    case Tokenizer::TokenBeginArray:
        tokenizer->pushBack();
        {
            auto* array = readArray();
            if (array == nullptr)
                return nullptr;
            else
                return new Value(array);
        }

    case Tokenizer::TokenBeginGroup:
        tokenizer->pushBack();
        {
            Hash* hash = readHash();
            if (hash == nullptr)
                return nullptr;
            else
                return new Value(hash);
        }

    default:
        tokenizer->pushBack();
        return nullptr;
    }
}
