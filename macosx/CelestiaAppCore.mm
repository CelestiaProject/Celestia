//
//  CelestiaCore.mm
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 05 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaAppCore.h"
#import "CelestiaAppCore_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaDestination_PrivateAPI.h"
#import "CelestiaFavorite_PrivateAPI.h"
#import "CelestiaSelection_PrivateAPI.h"
#import "CelestiaSimulation_PrivateAPI.h"
#import "CelestiaRenderer_PrivateAPI.h"
#import "CelestiaUniversalCoord_PrivateAPI.h"
#import "CelestiaUniverse_PrivateAPI.h"
#import "Astro.h"
#import <AppKit/AppKit.h>

/*

void initMovieCapture(MovieCapture *)
void recordBegin()
void recordPause()
void recordEnd()
bool isCaptureActive();
bool isRecording();

runScript(CommandSequence *);
CommandParser
CommandSequence

*/

CelestiaAppCore *_sharedCelestiaAppCore;
CelestiaCore *appCore;
NSInvocation *contextMenuCallbackInvocation;

void ContextMenuCallback(float x,float y, Selection selection) {
    CelestiaSelection *csel;
    NSPoint myPoint = NSMakePoint(x,y);
    if (contextMenuCallbackInvocation == nil) return;
    csel = [[[CelestiaSelection alloc] initWithSelection:selection] autorelease];
    [contextMenuCallbackInvocation setArgument:&csel atIndex:2];
    [contextMenuCallbackInvocation setArgument:&myPoint atIndex:3];
    [contextMenuCallbackInvocation invoke];
}

@implementation CelestiaAppCore

-(int)toCelestiaKey:(unichar)key {

    int	celestiaKey = 0;

    switch(key)
    {
        case NSF1FunctionKey:
            celestiaKey = CelestiaCore::Key_F1;
            break;
        case NSF2FunctionKey:
            celestiaKey = CelestiaCore::Key_F2;
            break;
        case NSF3FunctionKey:
            celestiaKey = CelestiaCore::Key_F3;
            break;
        case NSF4FunctionKey:
            celestiaKey = CelestiaCore::Key_F4;
            break;
        case NSF5FunctionKey:
            celestiaKey = CelestiaCore::Key_F5;
            break;
        case NSF6FunctionKey:
            celestiaKey = CelestiaCore::Key_F6;
            break;
        case NSF7FunctionKey:
            celestiaKey = CelestiaCore::Key_F7;
            break;
        case NSF8FunctionKey:
            celestiaKey = CelestiaCore::Key_F8;
            break;
        case NSF9FunctionKey:
            celestiaKey = CelestiaCore::Key_F9;
            break;
        case NSF10FunctionKey:
            celestiaKey = CelestiaCore::Key_F10;
            break;
        case NSF11FunctionKey:
            celestiaKey = CelestiaCore::Key_F11;
            break;
        case NSF12FunctionKey:
            celestiaKey = CelestiaCore::Key_F12;
            break;
        case NSUpArrowFunctionKey:
            celestiaKey = CelestiaCore::Key_Up;
            break;
        case NSDownArrowFunctionKey:
            celestiaKey = CelestiaCore::Key_Down;
            break;
        case NSLeftArrowFunctionKey:
            celestiaKey = CelestiaCore::Key_Left;
            break;
        case NSRightArrowFunctionKey:
            celestiaKey = CelestiaCore::Key_Right;
            break;
        case NSPageUpFunctionKey:
            celestiaKey = CelestiaCore::Key_PageUp;
            break;
        case NSPageDownFunctionKey:
            celestiaKey = CelestiaCore::Key_PageDown;
            break;
        case NSHomeFunctionKey:
            celestiaKey = CelestiaCore::Key_Home;
            break;
        case NSEndFunctionKey:
            celestiaKey = CelestiaCore::Key_End;
            break;
        case NSInsertFunctionKey:
            celestiaKey = CelestiaCore::Key_Insert;
            break;
        default:
            if ((key < 128) && (key > 32))
            {
                celestiaKey = (int) (key & 0x00FF);
            }
            break;
    }

    return celestiaKey;
}

-(int)toCelestiaModifiers:(unsigned int)modifiers buttons:(unsigned int)buttons {
    int cModifiers = 0;
    if (modifiers & NSControlKeyMask)
        cModifiers |= CelestiaCore::ControlKey;
    if (modifiers & NSShiftKeyMask)
        cModifiers |= CelestiaCore::ShiftKey;
    if (buttons & 1)
        cModifiers |= CelestiaCore::LeftButton;
    if (buttons & 2)
        cModifiers |= CelestiaCore::MiddleButton;
    if (buttons & 4)
        cModifiers |= CelestiaCore::RightButton;
    return cModifiers;
}

+(void)initialize
{
    _sharedCelestiaAppCore = nil;
    contextMenuCallbackInvocation = nil;
    appCore = NULL;
}

+(CelestiaAppCore *)sharedAppCore
{
    if (_sharedCelestiaAppCore != nil) return _sharedCelestiaAppCore;
    _sharedCelestiaAppCore = [[CelestiaAppCore alloc] init];
    return _sharedCelestiaAppCore;
}
    
-(CelestiaAppCore *)init
{
    if (_sharedCelestiaAppCore != nil) {
        [[super init] release];
        return _sharedCelestiaAppCore;
    }
    self = [super init];
    appCore = new CelestiaCore();
    contextMenuCallbackInvocation = NULL;
    _destinations = nil;
    return self;
}
-(void)archive
{
    //NSLog(@"[CelestiaAppCore archive]");
    [[CelestiaFavorites sharedFavorites] archive];
    [[self renderer] archive];
}

- (void)dealloc
{
    NSLog(@"[CelestiaAppCore dealloc]");
    if (_destinations != nil) {
        [_destinations release];
        _destinations = nil;
    }
    if (contextMenuCallbackInvocation != nil) {
        [contextMenuCallbackInvocation release];
        contextMenuCallbackInvocation = nil;
    }
    if (appCore != NULL) {
        delete appCore;
        appCore = NULL;
    }
    _sharedCelestiaAppCore = nil;
    [super dealloc];
}
    
-(BOOL)initSimulation
{
    return (BOOL)appCore->initSimulation();
}

-(BOOL)initRenderer
{
    return (BOOL)appCore->initRenderer();
}

-(void)start:(NSDate *)date withTimeZone:(NSTimeZone *)timeZone
{
    appCore->start([[Astro julianDate:date] doubleValue]);
    [self setTimeZone:timeZone withDate:date];
}

-(void)charEntered:(char)c
{
    if (c == 127) c = 8; // map del to bs
    appCore->charEntered(c);
}

-(void)keyDown:(int)c
{
    appCore->keyDown(c);
}

-(void)keyUp:(int)c
{
    appCore->keyUp(c);
}

-(void)mouseWheel:(float)motion modifiers:(int)modifiers
{
    appCore->mouseWheel(motion, modifiers);
}

-(void)mouseButtonDown:(NSPoint)coord modifiers:(int)modifiers
{
    appCore->mouseButtonDown(coord.x,coord.y,modifiers);
}

-(void)mouseButtonUp:(NSPoint)coord modifiers:(int)modifiers
{
    appCore->mouseButtonUp(coord.x,coord.y,modifiers);
}

-(void)mouseMove:(NSPoint)delta modifiers:(int)modifiers
{
    appCore->mouseMove(delta.x,delta.y,modifiers);
}

-(void)joystickAxis:(int)axis value:(float)value
{
    appCore->joystickAxis(axis,value);
}

-(void)joystickButton:(int)button state:(BOOL)state
{
    appCore->joystickButton(button,(bool)state);
}

-(void)resize:(NSRect)r
{
    appCore->resize((GLsizei)r.size.width,(GLsizei)r.size.height);
}

-(void)draw
{
    appCore->draw();
}

-(void)tick
{
    appCore->tick();
}

-(CelestiaSimulation *)simulation
{
    return [[[CelestiaSimulation alloc] initWithSimulation:appCore->getSimulation()] autorelease];
}

-(CelestiaRenderer *)renderer
{
    return [[[CelestiaRenderer alloc] initWithRenderer:appCore->getRenderer()] autorelease];
}

-(void)showText:(NSString *)text
{
    appCore->showText([text stdString]);
}

-(void)activateFavorite:(id)fav
{
    //NSLog(@"[CelestiaAppCore activateFavorite:%@]",fav);
    if ([fav isKindOfClass:[NSMenuItem class]])
        fav = [(NSMenuItem*)fav representedObject];
    appCore->activateFavorite(*[(CelestiaFavorite*)fav favorite]);
}

-(CelestiaFavorites *)favorites
{
    return [CelestiaFavorites sharedFavorites];
}

-(CelestiaDestinations *)destinations
{
    if (_destinations == nil || [_destinations destinations] != appCore->getDestinations()) {
        if (_destinations != nil)
            [_destinations release];
        _destinations = [[CelestiaDestinations alloc] initWithDestinations:appCore->getDestinations()];
    }
    return _destinations; 
}

-(NSTimeZone *)timeZone
{
    NSTimeZone *bestZone=nil;
    bestZone = [NSTimeZone timeZoneWithAbbreviation:[NSString stringWithStdString:appCore->getTimeZoneName()]];
    if (bestZone == nil) 
      bestZone = [NSTimeZone timeZoneForSecondsFromGMT:appCore->getTimeZoneBias()];
    return bestZone;
}

-(void)setTimeZone:(NSTimeZone *)timeZone withDate:(NSDate *)date
{
    appCore->setTimeZoneBias([timeZone secondsFromGMTForDate:date]);
    appCore->setTimeZoneName([[timeZone abbreviationForDate:date] cString]);
}

-(int)textEnterMode
{
    return appCore->getTextEnterMode();
}

-(void)cancelScript
{
    appCore->cancelScript();
}

-(int)hudDetail
{
    return appCore->getHudDetail();
}

-(void)setHudDetail:(int)hudDetail
{
    appCore->setHudDetail(hudDetail);
}

-(void)setContextMenuCallback:(id)cObj
{
    SEL callbackSelector = @selector(contextMenuCallback:location:);
    if (contextMenuCallbackInvocation != nil) {
        [contextMenuCallbackInvocation release];
        contextMenuCallbackInvocation = nil;
    }
    if (cObj == nil)
        appCore->setContextMenuCallback(NULL);
    else {
        appCore->setContextMenuCallback(ContextMenuCallback);
        contextMenuCallbackInvocation = [[NSInvocation invocationWithMethodSignature:[[cObj class] instanceMethodSignatureForSelector:callbackSelector]] retain];
        [contextMenuCallbackInvocation setSelector:callbackSelector];
        [contextMenuCallbackInvocation setTarget:cObj];
    }
}

-(void)back
{
    appCore->back();
}

-(void)forward
{
    appCore->forward();
}

- (NSURL *) currentURL
{
   Url currentUrl = Url(appCore, Url::Absolute);
   NSURL *url = [NSURL URLWithString: [ NSString stringWithStdString: currentUrl.getAsString() ]];
   return url;
}

-(void)goToUrl:(NSString *)url
{
    appCore->goToUrl([url stdString]);
}


@end

