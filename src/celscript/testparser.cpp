// testparser.cpp
//
// Copyright (C) 2002 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/parser.h>

using namespace std;
using namespace celx;


int main(int argc, char* argv[])
{
    Scanner scanner(&cin);
    Parser parser(scanner);
    GlobalEnvironment* env = new GlobalEnvironment();
    ExecutionContext context(env);

    cout << "Testing parser . . .\n";

    while (scanner.nextToken() != Scanner::TokenEnd)
    {
        scanner.pushBack();
        Statement* statement = parser.parseStatement();
        if (statement != NULL)
        {
            cout << "Valid\n";
            statement->execute(context);
        }
        else
        {
            cout << "Invalid\n";
        }
    }

    return 0;
}
