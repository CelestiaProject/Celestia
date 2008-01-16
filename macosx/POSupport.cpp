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

#define LOCALIZED_STR_BUFSIZE   1024
char localizedStr[LOCALIZED_STR_BUFSIZE];

const char *localizedUTF8String(const char *key)
{
    return localizedUTF8StringWithDomain("po", key);
}

const char *localizedUTF8StringWithDomain(const char *domain, const char *key)
{
    const char *result = key;
    CFStringRef keyStr = CFStringCreateWithCStringNoCopy(NULL, key, kCFStringEncodingUTF8, kCFAllocatorNull);
    CFStringRef domainStr = CFStringCreateWithCStringNoCopy(NULL, domain, kCFStringEncodingUTF8, kCFAllocatorNull);
    CFStringRef str = CFCopyLocalizedStringFromTable(keyStr, domainStr, "");
    CFRelease(keyStr);
    CFRelease(domainStr);

    if (str)
    {
        if (CFStringGetCString(str, localizedStr, LOCALIZED_STR_BUFSIZE, kCFStringEncodingUTF8))
        {
            result = localizedStr;
        }
    }

    CFRelease(str);
    return result;
}
