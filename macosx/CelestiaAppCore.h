//
//  CelestiaCore.h
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 05 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "CelestiaDestination.h"
#import "CelestiaFavorite.h"
#import "CelestiaSimulation.h"
#import "CelestiaRenderer.h"

@interface CelestiaAppCore : NSObject {
    CelestiaFavorites* _favorites;
    CelestiaDestinations* _destinations;
}
+(CelestiaAppCore *)sharedAppCore;
-(BOOL)initSimulation;
-(BOOL)initRenderer;
-(void)start:(NSDate *)date withTimeZone:(NSTimeZone *)timeZone;
-(void)charEntered:(char)c;
-(void)keyDown:(int)c;
-(void)keyUp:(int)c;
-(void)mouseWheel:(float)motion modifiers:(int)modifiers;
-(void)mouseButtonDown:(NSPoint)coord button:(int)button;
-(void)mouseButtonUp:(NSPoint)coord button:(int)button;
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
@end
