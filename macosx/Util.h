//
//  Util.h
//  celestia
//
//  Created by Da Woon Jung on 05/07/24.
//  Copyright 2005 Chris Laurel. All rights reserved.
//

#if __has_include(<libintl.h>)
#include <libintl.h>
#else
#include "POSupport.h"
#define gettext(s)      localizedUTF8String(s)
#define dgettext(d,s)   localizedUTF8StringWithDomain(d,s)
#endif

#define __AIFF__
#include <TargetConditionals.h>

#ifdef __OBJC__
# import <Cocoa/Cocoa.h>
# undef DPRINTF
#endif
