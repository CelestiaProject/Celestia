// asterism.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "util.h"
#include "parser.h"
#include "asterism.h"

using namespace std;


Asterism::Asterism(string _name) : name(_name)
{
}

Asterism::~Asterism()
{
}


string Asterism::getName() const
{
    return name;
}

int Asterism::getChainCount() const
{
    return chains.size();
}

const Asterism::Chain& Asterism::getChain(int index) const
{
    return *chains[index];
}

void Asterism::addChain(Asterism::Chain& chain)
{
    chains.insert(chains.end(), &chain);
}


AsterismList* ReadAsterismList(istream& in, const StarDatabase& stardb)
{
    AsterismList* asterisms = new AsterismList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF("Error parsing asterism file.\n");
            for_each(asterisms->begin(), asterisms->end(), deleteFunc<Asterism*>());
            delete asterisms;
            return NULL;
        }

        string name = tokenizer.getStringValue();
        Asterism* ast = new Asterism(name);

        Value* chainsValue = parser.readValue();
        if (chainsValue == NULL || chainsValue->getType() != Value::ArrayType)
        {
            DPRINTF("Error parsing asterism %s\n", name.c_str());
            for_each(asterisms->begin(), asterisms->end(), deleteFunc<Asterism*>());
            delete asterisms;
            return NULL;
        }

        Array* chains = chainsValue->getArray();

        for (int i = 0; i < (int) chains->size(); i++)
        {
            if ((*chains)[i]->getType() == Value::ArrayType)
            {
                Array* a = (*chains)[i]->getArray();
                Asterism::Chain* chain = new Asterism::Chain();
                for (Array::const_iterator iter = a->begin(); iter != a->end(); iter++)
                {
                    if ((*iter)->getType() == Value::StringType)
                    {
                        Star* star = stardb.find((*iter)->getString());
                        if (star != NULL)
                            chain->insert(chain->end(), star->getPosition());
                    }
                }
                
                ast->addChain(*chain);
            }
        }

        asterisms->insert(asterisms->end(), ast);

        delete chainsValue;
    }

    return asterisms;
}
