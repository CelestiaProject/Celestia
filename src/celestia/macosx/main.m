//
//  main.m
//  celestia
//
//  Created by Bob Ippolito on Fri May 17 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#include <Carbon/Carbon.h>

int main(int argc, const char *argv[])
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
    CFLocaleRef curLocale = CFLocaleCopyCurrent();
    if (curLocale)
    {
        CFStringRef localeName = CFLocaleGetIdentifier(curLocale);
        if (localeName)
        {
            char localeBuf[32];
            if (CFStringGetCString(localeName, localeBuf, 32, kCFStringEncodingUTF8))
            {
                setenv("LANG", localeBuf, 1);
            }
        }
        CFRelease(curLocale);
    }
#endif
    setlocale(LC_ALL, "");
    return NSApplicationMain(argc,argv);
}
