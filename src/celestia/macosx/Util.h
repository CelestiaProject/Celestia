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

#define __AIFF__
#include <TargetConditionals.h>

#ifdef __OBJC__
# include "GL/glew.h"
# import <Cocoa/Cocoa.h>
# undef DPRINTF
#endif
