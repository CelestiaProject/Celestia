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

#import "CelestiaController.h"
#include "celestiacore.h"
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

class MacOSXAlerter : public CelestiaCore::Alerter
{
public:
    MacOSXAlerter() {};
    ~MacOSXAlerter() {};

    void fatalError(const std::string& msg)
    {
//        NSRunAlertPanel(@"Fatal Error",[NSString stringWithCString: msg.c_str()],nil,nil,nil);
        NSLog(@"alerter fatalError!");
        [[CelestiaController shared] fatalError: [NSString stringWithCString: msg.c_str()] ];
        NSLog(@"alerter fatalError finis");
    }
};

class MacOSXWatcher : public CelestiaWatcher
{
    private:
        CelestiaAppCore *celAppCore;

    public:
        MacOSXWatcher(CelestiaCore* _appCore, CelestiaAppCore* _celAppCore) : CelestiaWatcher(*_appCore), celAppCore(_celAppCore)
{};
            
    public:

    void notifyChange(CelestiaCore* core, int i)
    {
        [celAppCore validateItems];
    };
};

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

-(int)toCelestiaKey:(NSEvent*)theEvent {

    int	celestiaKey = 0;
    unichar key = [[theEvent characters] characterAtIndex: 0];

   if ( ([theEvent modifierFlags] & NSNumericPadKeyMask) && (key >= '0') && (key <= '9'))
    switch(key)
    {
        case '0':
            celestiaKey = CelestiaCore::Key_NumPad0;
            break;
        case '1':
            celestiaKey = CelestiaCore::Key_NumPad1;
            break;
        case '2':
            celestiaKey = CelestiaCore::Key_NumPad2;
            break;
        case '3':
            celestiaKey = CelestiaCore::Key_NumPad3;
            break;
        case '4':
            celestiaKey = CelestiaCore::Key_NumPad4;
            break;
        case '5':
            celestiaKey = CelestiaCore::Key_NumPad5;
            break;
        case '6':
            celestiaKey = CelestiaCore::Key_NumPad6;
            break;
        case '7':
            celestiaKey = CelestiaCore::Key_NumPad7;
            break;
        case '8':
            celestiaKey = CelestiaCore::Key_NumPad8;
            break;
        case '9':
            celestiaKey = CelestiaCore::Key_NumPad9;
            break;
        default: break;
        }
        else switch(key)
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
            if ((key < 128) && (key > 33))
            {
                celestiaKey = (int) (key & 0x00FF);
            }
            break;
    }

    return celestiaKey;
}

-(int)toCelestiaModifiers:(unsigned int)modifiers buttons:(unsigned int)buttons {
    int cModifiers = 0;
    if (modifiers & NSCommandKeyMask)
        cModifiers |= CelestiaCore::ControlKey;
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

static NSMutableDictionary* tagDict;

+(void)initialize
{
    _sharedCelestiaAppCore = nil;
    contextMenuCallbackInvocation = nil;
    appCore = NULL;
    tagDict = [[ NSMutableDictionary dictionaryWithCapacity: 100 ] retain];
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
    appCore->setAlerter(new MacOSXAlerter());
//    MacOSXWatcher* theWatcher = new MacOSXWatcher(appCore,self);
//    appCore->setWatcher(theWatcher);
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
// moved to CelestiaOpenGLView...    if (c == 127) c = 8; 
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

-(void)keyDown:(int)c withModifiers:(int)m
{
    appCore->keyDown(c,m);
}

-(void)keyUp:(int)c withModifiers:(int)m
{
    appCore->keyUp(c,m);
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

-(unsigned int) getLocationFilter
{
    return appCore->getSimulation()->getActiveObserver()->getLocationFilter();
}

-(void) setLocationFilter: (unsigned int) filter
{
    appCore->getSimulation()->getActiveObserver()->setLocationFilter(filter);
}

-(void)runScript:(NSString *)fileName
{
    ifstream scriptfile([fileName cString]);
    CommandParser parser(scriptfile);
    CommandSequence* script = parser.parse();
    if (script == NULL)
    {
       NSRunAlertPanel(@"Invalid Script File",@"Please verify that you selected a valid script file.",nil,nil,nil);
       return;
    }
    appCore->runScript(script);
}

- (void) disableSurfaceMenu: (NSMenu*) contextMenu
{
        NSMenuItem* surfaceItem = [ contextMenu itemWithTitle: @"Show Alternate Surface"];
//        [ surfaceItem setSubmenu: NULL ];
//        [ surfaceItem setAction: NULL ];
//        [ contextMenu setAutoenablesItems: NO ];
        [ surfaceItem setEnabled: NO ];
        NSLog(@"disabling surface menu\n");
}

- (void) addSurfaceMenu: (NSMenu*) contextMenu
{
    Selection sel = appCore->getSimulation()->getSelection();
    if (sel.body() == NULL)
    {
        [self disableSurfaceMenu: contextMenu ];
        return;
    }
    else
    {
        NSMenuItem* firstItem = [ contextMenu itemAtIndex: 0];
        NSMenuItem* surfaceItem = [ contextMenu itemWithTitle: @"Show Alternate Surface"];
        NSMenu* surfaceMenu = [[NSMenu alloc ] initWithTitle: @"altsurf" ];
        vector<string>* surfaces = sel.body()->getAlternateSurfaceNames();
        if ( surfaces->size() == 0 ) 
        {
            [self disableSurfaceMenu: contextMenu];
            return;
        }
            [ surfaceItem setEnabled: YES ];
            [ contextMenu setAutoenablesItems: YES ];
            [ surfaceMenu setAutoenablesItems: YES ];
            NSMenuItem* newItem = [ [NSMenuItem alloc] init ]; 
            [newItem setTitle: @"default" ];
            [newItem setTag:  600 ];
            [newItem setTarget:  [firstItem target] ];
            [newItem setAction:  [firstItem action] ];
            [ surfaceMenu addItem: newItem ];
        for (int i = 0; i < (int)surfaces->size(); i++)
        {
            NSMenuItem* newItem = [ [NSMenuItem alloc] init ]; 
            [newItem setTitle: [ NSString stringWithCString: (*surfaces)[i].c_str() ] ];
            [newItem setTag:  601+i ];
            [newItem setEnabled:  YES ];
            [newItem setTarget:  [firstItem target] ];
            [newItem setAction:  [firstItem action] ];
            [ surfaceMenu addItem: newItem ];
        }
        [ surfaceItem setSubmenu: surfaceMenu ];
        [ surfaceItem setEnabled: YES ];
        [ surfaceMenu update ];
        delete surfaces;
        return;
    }
}

- (BOOL) validateAltSurface: (int) index
{
    if (index == 0) 
    {
        string displayedSurfName = appCore->getSimulation()->getActiveObserver()->getDisplayedSurface();
        return displayedSurfName == string("");
    }
    index--;
    // Validate items for the alternate surface submenu
    Selection sel = appCore->getSimulation()->getSelection();
    if (sel.body() != NULL)
    {
        vector<string>* surfNames = sel.body()->getAlternateSurfaceNames();
        if (surfNames != NULL)
        {
            string surfName, displayedSurfName;
            if (index >= 0 && index < (int)surfNames->size())
                surfName = surfNames->at(index);
            displayedSurfName = appCore->getSimulation()->getActiveObserver()->getDisplayedSurface();
            if (surfName  == displayedSurfName )
            { 
                delete surfNames;
                return YES;
            }
        }
    }
    return NO;
}

- (void) setAltSurface: (int) index
{
    // Handle the alternate surface submenu
    Selection sel = appCore->getSimulation()->getSelection();
    if (sel.body() != NULL)
    {
        if ( index == 0 )
        {
            appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(string(""));
            return;
        }
        vector<string>* surfNames = sel.body()->getAlternateSurfaceNames();
        if (surfNames != NULL)
        {
            string surfName;
            index--;
            if (index >= 0 && index < (int)surfNames->size())
                surfName = surfNames->at(index);
            appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(surfName);
            delete surfNames;
        }
    }
}

- (void) showInfoURL
{
    Selection sel = appCore->getSimulation()->getSelection();

    string url;
    switch (sel.getType())
    {
    case Selection::Type_Body:
        {
            url = sel.body()->getInfoURL();
            if (url.empty())
            {
                string name = sel.body()->getName();
                for (int i = 0; i < (int)name.size(); i++)
                    name[i] = tolower(name[i]);

                url = string("http://www.nineplanets.org/") + name + ".html";
            }
        }
        break;

    case Selection::Type_Star:
        {
            char name[32];
            sprintf(name, "HIP%d", sel.star()->getCatalogNumber() & ~0xf0000000);

            url = string("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=") + name;
        }
        break;

    case Selection::Type_DeepSky:
        url = sel.deepsky()->getInfoURL();
        break;

    case Selection::Type_Location:
        break;

    default:
        break;
    }


    if ( url != NULL )
    {
        NSURL* theURL = [NSURL URLWithString: [NSString stringWithStdString: url ]];
        if ( theURL != NULL)
        {
            [[NSWorkspace sharedWorkspace] openURL: theURL];
        }
    }
}


- (int) tagForKey: (int) key
{
    int tag;
    switch (key)
    {
            case 112: tag = 501; break;  // LabelPlanets
            case 109: tag = 502; break;  // LabelMoons
            case 119: tag = 505; break;  // LabelAsteroids
            case  98: tag = 500; break;  // LabelStars
            case 101: tag = 504; break;  // LabelGalaxies
            case 110: tag = 506; break;  // LabelSpacecraft
            case  61: tag = 503; break;  // LabelConstellations
            case 105: tag = 404; break;  // CloudMaps
            case   1: tag = 408; break;  // Atmospheres
            case  12: tag = 407; break;  // NightMaps
            case   5: tag = 410; break;  // EclipseShadows
            case 111: tag = 405; break;  // Orbits
            case 117: tag = 402; break;  // Galaxies
            case  47: tag = 403; break;  // Diagrams
            case   2: tag = 413; break;  // Boundaries
            case  59: tag = 406; break;  // CelestialSphere
            case  25: tag = 414; break;  // AutoMag
            case  20: tag = 415; break;  // CometTails
            case  11: tag = 416; break;  // Markers
            case  19: tag = 411; break;  // StarsAsPoints
            case  24: tag = 409; break;  // SmoothLines
            default : tag = key; break; // Special or not a setting
    }
    return tag;
}

- (BOOL) validateButton: (id) item atIndex: (int) index withValue: (int) value 
{
        if ( (index==0) && [item isKindOfClass: [NSPopUpButton class] ] )
        {
            NSEnumerator* items = [ [item itemArray] objectEnumerator ];
            id menuItem;
            while (menuItem = [items nextObject])
            {
                if ( [menuItem tag] == ([item tag]+value) )
                {
                    [item selectItem: menuItem];
                    break;
                }
            }
        }
        else
            [item setState:  (value == index) ? NSOnState: NSOffState ];
        return YES;
}

- (BOOL) validateItem: (id) item
{
    int tag = [item tag];
    if ( tag <= 128 ) tag = [self tagForKey: tag ];
    if ( tag == 32 ) 
    {
        [item setState:  (fabs(appCore->getSimulation()->getTimeScale()) == 0.0 ) ? NSOnState : NSOffState ]; 
    }
    else if ( tag <= 128 ) 
    {
        [item setState: NSOffState ];
    }
    else if ( tag >= 400 && tag < 500 )
    {
        int flag = tag-400;
        int renderFlags = appCore->getRenderer()->getRenderFlags();
        [item setState: ( (renderFlags & ( 1 << flag )) != 0 ) ? NSOnState : NSOffState ];
    }    
    else if ( tag >= 500 && tag < 600 )
    {
        int flag = tag-500;
        int labelMode = appCore->getRenderer()->getLabelMode();
        [item setState: ( (labelMode & ( 1 << flag )) != 0 ) ? NSOnState : NSOffState ];
    }    
    else if ( tag >= 600 && tag < 610 )
    {
        int index = tag-600;
        [item setState:  [self validateAltSurface: index ] ? NSOnState: NSOffState ];            
    }    
    else if ( tag >= 610 && tag < 620 )
    {
        int index = tag-610;
        [self validateButton: item atIndex: index withValue: appCore->getHudDetail() ];
    }    
    else if ( tag >= 620 && tag < 630 )
    {
        int index = tag-620;
        [self validateButton: item atIndex: index withValue: appCore->getRenderer()->getStarStyle() ];
    }    
    else if ( tag >= 630 && tag < 640 )
    {
        int index = tag-630;
        [self validateButton: item atIndex: index 
            withValue: appCore->getRenderer()->getGLContext()->getRenderPath() ];
    }    
    else if ( tag >= 640 && tag < 700 )
    {
        int index = tag-640;
        [self validateButton: item atIndex: index 
            withValue: appCore->getRenderer()->getResolution() ];
    }    
    else if ( tag >= 700 && tag < 800 )
    {
        int flag = tag-700;
        int orbitMask = appCore->getRenderer()->getOrbitMask();
        [item setState:   ( (orbitMask & ( 1 << flag )) != 0 ) ? NSOnState: NSOffState ];
    }
    else if ( tag >= 800 && tag < 900 )
    {
        int flag = tag-800;
        unsigned int locationFilter = [self getLocationFilter];
        [item setState:   ( (locationFilter & ( 1 << flag )) != 0 ) ? NSOnState: NSOffState ];
    }
    else if ( tag >= 900 && tag < 1000 )
    {
        int index = tag-900;
        switch(index)
        {
            case 0:
                [item setFloatValue: appCore->getRenderer()->getAmbientLightLevel() ];                
                break;
            case 1:
                [item setFloatValue: appCore->getRenderer()->getBrightnessBias()];                
                break;
            case 2:
                if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
                {
                   [item setFloatValue: appCore->getSimulation()->getFaintestVisible()];
                }
                else
                {
                    [item setFloatValue: appCore->getRenderer()->getFaintestAM45deg()];
                }                
                break;
            case 3:
                [item setFloatValue: appCore->getRenderer()->getMinimumFeatureSize()];                
                break;
            default: break;
        }
    }
    return YES;
}


- (void) selectPopUpButtonItem: (id) item withIndex: (int) index
{
        id item2 = [ tagDict objectForKey: [NSNumber numberWithInt: ([item tag]-index) ]];
        if ([item2 isKindOfClass: [NSPopUpButton class]])
        {
            [item2 selectItem: item];
        };
}

- (void) actionForItem: (id) item
{
    int tag = [item tag];
// NSLog(@"item action for key: %d\n",tag);
    if ( tag <= 128 ) tag = [self tagForKey: tag ];
    if ( tag <= 128 ) { [ self charEntered: tag ]; return; };
    if ( tag >= 400 && tag < 500 )
    {
        int flag = tag-400;
        int renderFlags = appCore->getRenderer()->getRenderFlags();
        appCore->getRenderer()->setRenderFlags( renderFlags ^ ( 1 << flag ) );
        return;
    }    
    if ( tag >= 500 && tag < 600 )
    {
        int flag = tag-500;
        int labelMode = appCore->getRenderer()->getLabelMode();
        appCore->getRenderer()->setLabelMode( labelMode ^ ( 1 << flag ) );
        return;
    }    
    if ( tag >= 600 && tag < 610 )
    {
        int index = tag-600;
        [self setAltSurface: index ];
        return;            
    }    
    if ( tag >= 610 && tag < 620 )
    {
        int index = tag-610;
        appCore->setHudDetail(index);
        [self selectPopUpButtonItem: item withIndex: index];
/*
        id item2 = [ tagDict objectForKey: [NSNumber numberWithInt: 610 ]];
        if ([item2 isKindOfClass: [NSPopUpButton class]])
        {
            [item2 selectItem: item];
        };
*/
        return;
    }    
    if ( tag >= 620 && tag < 630 )
    {
        int index = tag-620;
        appCore->getRenderer()->setStarStyle((Renderer::StarStyle)index);
        [self selectPopUpButtonItem: item withIndex: index];
        return;            
    }    
    if ( tag >= 630 && tag < 640 )
    {
        int index = tag-630;
        appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)index);
        [self selectPopUpButtonItem: item withIndex: index];
        return;            
    }    
    if ( tag >= 640 && tag < 700 )
    {
        int index = tag-640;
        appCore->getRenderer()->setResolution(index);
        [self selectPopUpButtonItem: item withIndex: index];
        return;            
    }    
    if ( tag >= 700 && tag < 800 )
    {
        int flag = tag-700;
        int orbitMask = appCore->getRenderer()->getOrbitMask();
        appCore->getRenderer()->setOrbitMask( orbitMask ^ ( 1 << flag ) );
        return;
    }
    if ( tag >= 800 && tag < 900 )
    {
        int flag = tag-800;
        unsigned int locationFilter = [self getLocationFilter];
        [self setLocationFilter: locationFilter ^ ( 1 << flag ) ];
        return;
    }
    if ( tag >= 900 && tag < 1000 )
    {
        int index = tag-900;
        switch(index)
        {
            case 0:
                appCore->getRenderer()->setAmbientLightLevel([item floatValue]);                
                break;
            case 1:
                appCore->getRenderer()->setBrightnessBias([item floatValue]);                
                break;
            case 2:
                if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
                {
                    appCore->setFaintest([item floatValue]);
                }
                else
                {
                    appCore->getRenderer()->setFaintestAM45deg([item floatValue]);
                    appCore->setFaintestAutoMag();
                }                
                break;
            case 3:
                appCore->getRenderer()->setMinimumFeatureSize([item floatValue]);                
                break;
            default: break;
        }
        return;
    }
}

- (void) defineKeyForItem: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        NSNumber* itemKey = [NSNumber numberWithInt: tag ];
        if ( [ tagDict objectForKey: itemKey ] == 0 )
        {
            [tagDict setObject: item forKey: itemKey];
// NSLog(@"defining item for key: %d\n",tag);
        }
    }
}



- (void) validateItemForTag: (int) tag
{
//NSLog(@"validating item for key: %d\n",tag);
    if ( tag <= 128 ) tag = [self tagForKey: tag ];
    id item = [ tagDict objectForKey: [NSNumber numberWithInt: tag ]];
    if ( [item isKindOfClass: [NSMenuItem class]] ) return; // auto-validated
    if ( item != 0 )
    {
//NSLog(@"validating item for key: %d value: %d\n",tag,[self testSetting: [item tag] ]);
        [ self validateItem: item ];
    }
}

- (void) validateItems
{
	id obj;
    NSEnumerator* enumerator = [[tagDict allKeys] objectEnumerator];
	while ((obj = [enumerator nextObject]) != nil) {
		[self validateItemForTag: [obj intValue]];
	}
}

@end

