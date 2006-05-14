//
//  Util.h
//  celestia
//
//  Created by Da Woon Jung on 05/07/24.
//  Copyright 2005 Chris Laurel. All rights reserved.
//

#ifndef gettext
#include "POSupport.h"
#define gettext(s)      localizedUTF8String(s)
#define dgettext(d,s)   localizedUTF8StringWithDomain(d,s)
#endif

#ifdef __OBJC__
# define __AIFF__
# import <Cocoa/Cocoa.h>
# undef DPRINTF
#endif
