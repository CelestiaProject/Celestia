//
//  Util.h
//  celestia
//
//  Created by Da Woon Jung on 05/07/24.
//  Copyright 2005 Chris Laurel. All rights reserved.
//

#include <libintl.h>

#define __AIFF__
#include <TargetConditionals.h>

#ifdef __OBJC__
# import <Cocoa/Cocoa.h>
# undef DPRINTF
#endif
