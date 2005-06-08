//
//  CelestiaCore.h
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 05 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#define __AIFF__
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#import "CelestiaDestination.h"
#import "CelestiaFavorite.h"
#import "CelestiaFavorites.h"
#import "CelestiaSimulation.h"
#import "CelestiaRenderer.h"


#ifdef __cplusplus
class CelestiaCore;
#else
@class CelestiaCore;
#endif

@interface CelestiaAppCore : NSObject {
    CelestiaDestinations* _destinations;
}
-(CelestiaCore*) appCore;
-(int)toCelestiaKey:(NSEvent*)theEvent;
-(int)toCelestiaModifiers:(unsigned int)modifiers buttons:(unsigned int)buttons;
-(void)archive;
+(CelestiaAppCore *)sharedAppCore;
    -(BOOL)initSimulation;
-(BOOL)initRenderer;
-(void)start:(NSDate *)date withTimeZone:(NSTimeZone *)timeZone;
-(void)charEntered:(char)c withModifiers:(int)modifiers;
-(void)keyDown:(int)c;
-(void)keyUp:(int)c;
-(void)mouseWheel:(float)motion modifiers:(int)modifiers;
-(void)mouseButtonDown:(NSPoint)coord modifiers:(int)modifiers;
-(void)mouseButtonUp:(NSPoint)coord modifiers:(int)modifiers;
-(void)mouseMove:(NSPoint)delta modifiers:(int)modifiers;
-(void)joystickAxis:(int)axis value:(float)value;
-(void)joystickButton:(int)button state:(BOOL)state;
-(void)resize:(NSRect)r;
-(void)draw;
-(void)tick;
-(CelestiaSimulation *)simulation;
-(CelestiaRenderer *)renderer;
-(void)showText:(NSString *)text;
-(void)activateFavorite:(id)fav;
-(CelestiaFavorites *)favorites;
-(CelestiaDestinations *)destinations;
-(NSTimeZone *)timeZone;
-(void)setTimeZone:(NSTimeZone *)timeZone withDate:(NSDate *)date;
-(int)textEnterMode;
-(void)cancelScript;
-(int)hudDetail;
-(void)setHudDetail:(int)hudDetail;
-(void)setContextMenuCallback:(id)cObj;
-(void)back;
-(void)forward;
- (NSString *) currentURL;
-(void)goToUrl:(NSString *)url;
-(void)setStartURL:(NSString *)url;
-(unsigned int) getLocationFilter;
-(void) setLocationFilter: (unsigned int) filter;
-(void)runScript:(NSString *)fileName;
-(void)showInfoURL;
-(void)keyDown:(int)c withModifiers:(int)m;
-(void)keyUp:(int)c withModifiers:(int)m;
- (BOOL) captureMovie: (NSString*)filename width: (int)width height: (int)height
                              frameRate: (float)framerate;


@end
