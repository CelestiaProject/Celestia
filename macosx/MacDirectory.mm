//
// MacDirectory.mm
// celestia
//
// Created by Da Woon Jung on Sun Apr 24 2004.
// Copyright (c) 2005 Chris Laurel. All rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <dirent.h>
#import "directory.h"
#import "NSString_ObjCPlusPlus.h"


class MacDirectory : public Directory
{
public:
    MacDirectory(NSString *dirName);
    virtual ~MacDirectory();
    
    bool nextFile(std::string &);
    bool enumFiles(EnumFilesHandler &handler, bool deep);

    enum {
        DirGood = 0,
        DirBad = 1
    };
    
private:
    char *_dirstring;
    int _status;
    DIR *_dir;
};


MacDirectory::MacDirectory(NSString *dirName) :
_status(DirGood), _dir(NULL)
{
    NSString *fullPath = [dirName stringByStandardizingPath];
    const char *fullPathStr = [fullPath UTF8String];

    unsigned dirNameLen = CFStringGetMaximumSizeForEncoding([fullPath length], kCFStringEncodingUTF8);

    if (dirNameLen > 0)
    {
        _dirstring = (char *)calloc(dirNameLen+sizeof(char), 1);
        strncpy(_dirstring, fullPathStr, dirNameLen/sizeof(char));
    }
    else
        _dirstring = NULL;
}


MacDirectory::~MacDirectory()
{
    if (_dir != NULL)
    {
        closedir(_dir);
        _dir = NULL;
    }

    if (_dirstring)
        free(_dirstring);
}


bool MacDirectory::nextFile(std::string &filename)
{
    if (_status != DirGood)
        return false;
    
    if (_dir == NULL)
    {
        _dir = opendir(_dirstring);
        if (_dir == NULL)
        {
            _status = DirBad;
            return false;
        }
    }
    
    struct dirent *ent = readdir(_dir);
    if (ent == NULL)
    {
        _status = DirBad;
        return false;
    }
    else
    {
        filename = ent->d_name;
        return true;
    }
}

bool MacDirectory::enumFiles(EnumFilesHandler &handler, bool deep)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    bool result = Directory::enumFiles(handler, deep);
    [pool release];
    return result;
}


std::string WordExp(const std::string &filename)
{
    NSString *expandedPath = [[NSString stringWithStdString: filename] stringByStandardizingPath];
    return [expandedPath stdString];
}

Directory* OpenDirectory(const std::string &dirname)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    Directory *result = new MacDirectory([NSString stringWithStdString: dirname]);
    [pool release];
    return result;
}

bool IsDirectory(const std::string &filename)
{
    BOOL isDir = NO;
    return [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithStdString: filename] isDirectory:&isDir] && isDir;
}
