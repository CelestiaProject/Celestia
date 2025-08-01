// appleutils.mm
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// Miscellaneous useful Apple platform functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <Foundation/Foundation.h>
#include "appleutils.h"

std::filesystem::path AppleHomeDirectory()
{
    @autoreleasepool {
        return [NSHomeDirectory() UTF8String];
    }
}

std::filesystem::path AppleApplicationSupportDirectory()
{
    @autoreleasepool {
        NSArray *directories = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        if ([directories count] > 0)
            return [[directories firstObject] UTF8String];
         return "~/Library/Application Support";
    }
}
