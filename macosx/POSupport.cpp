/*
 * POSupport.cpp
 * Celestia
 *
 * Created by Da Woon Jung on 2008-01-13.
 * Copyright 2008 Da Woon Jung. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <Carbon/Carbon.h>
#include "POSupport.h"
#include <string>
#include <map>
#include <iostream>

#define LOCALIZED_STR_BUFSIZE   1024

typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, StringMap> MapMap;
MapMap domainDict;

const char *localizedUTF8String(const char *key)
{
    return localizedUTF8StringWithDomain("po", key);
}

const char *localizedUTF8StringWithDomain(const char *domain, const char *key)
{
    std::string domainStr(domain);
    std::string keyStr(key);
    MapMap::iterator iter = domainDict.find(domainStr);
    if (iter == domainDict.end())
    {
        StringMap strMap;
        domainDict[domainStr] = strMap;
        iter = domainDict.find(domainStr);
    }
    StringMap &localizedDict = iter->second;
    StringMap::iterator strIter = localizedDict.find(keyStr);
    if (strIter == localizedDict.end())
    {
        CFStringRef domainRef = CFStringCreateWithCStringNoCopy(NULL, domain, kCFStringEncodingUTF8, kCFAllocatorNull);
        CFStringRef keyRef = CFStringCreateWithCStringNoCopy(NULL, key, kCFStringEncodingUTF8, kCFAllocatorNull);
        CFStringRef localizedRef = CFCopyLocalizedStringFromTable(keyRef, domainRef, "");
        char localizedBuf[LOCALIZED_STR_BUFSIZE];
        CFStringGetCString(localizedRef, localizedBuf, LOCALIZED_STR_BUFSIZE, kCFStringEncodingUTF8);
        std::string localizedStr(localizedBuf);
        localizedDict[keyStr] = localizedStr;
        CFRelease(localizedRef);
        if (keyRef)
        {
            CFRelease(keyRef);
        }
        CFRelease(domainRef);
        strIter = localizedDict.find(keyStr);
    }
    std::string &localized = strIter->second;
    return localized.c_str();
}
