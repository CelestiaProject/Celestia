//
//  CelestiaCore.mm
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 05 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#import "CelestiaAppCore.h"
#import "CelestiaAppCore_PrivateAPI.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaDestination_PrivateAPI.h"
#import "CelestiaFavorite_PrivateAPI.h"
#import "CelestiaSelection_PrivateAPI.h"
#import "CelestiaSimulation_PrivateAPI.h"
#import "CelestiaRenderer_PrivateAPI.h"
#import "Astro.h"

#import "CelestiaController.h"
#include "celestiacore.h"
#include "render.h"
#include "qtcapture.h"
#import <Carbon/Carbon.h>

class MacSettingsWatcher : public CelestiaWatcher, public RendererWatcher
{
private:
    CelestiaSettings *settings;
    CelestiaCore *appCore;

public:
    MacSettingsWatcher(CelestiaAppCore *_appCore,
                       CelestiaSettings* _settings) :
        CelestiaWatcher(*[_appCore appCore]), settings(_settings), appCore([_appCore appCore])
    {
        appCore->getRenderer()->addWatcher(this);
    };
    
    virtual ~MacSettingsWatcher()
    {
        appCore->getRenderer()->removeWatcher(this);
    };

    void notifyChange(CelestiaCore *, int flags)
    {
        if ( 0 != (flags & (
//              CelestiaCore::LabelFlagsChanged
//            | CelestiaCore::RenderFlagsChanged
            CelestiaCore::VerbosityLevelChanged
            | CelestiaCore::TimeZoneChanged
//            | CelestiaCore::AmbientLightChanged
            | CelestiaCore::FaintestChanged
            )) )
        {
            [settings validateItems];
        }
    };

    virtual void notifyRenderSettingsChanged(const Renderer *renderer)
    {
        [settings validateItems];
    };
};

class MacOSXAlerter : public CelestiaCore::Alerter
{
public:
    MacOSXAlerter() {};
    virtual ~MacOSXAlerter() {};

    virtual void fatalError(const std::string& msg)
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        NSLog(@"alerter fatalError!");
        [[CelestiaController shared] fatalError: [NSString stringWithStdString: msg] ];
        NSLog(@"alerter fatalError finis");
        [pool release];
    }
};

class MacOSXSplashProgressNotifier : public ProgressNotifier
{
public:
    MacOSXSplashProgressNotifier() {};
    virtual ~MacOSXSplashProgressNotifier() {};

    virtual void update(const string& msg)
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        [[[CelestiaController shared] valueForKey: @"splashWindowController"] performSelector: @selector(setStatusText:) withObject: [NSString stringWithStdString: msg]];
        [pool release];
    };
};

class MacOSXCursorHandler : public CelestiaCore::CursorHandler
{
public:
    MacOSXCursorHandler() : cursor(kThemeArrowCursor),
        shape(CelestiaCore::ArrowCursor) {};
    virtual ~MacOSXCursorHandler() {};
    virtual void setCursorShape(CelestiaCore::CursorShape aShape)
    {
        ThemeCursor changedCursor;

        switch (aShape)
        {
        case CelestiaCore::SizeVerCursor:
            changedCursor =
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
            kThemeResizeUpDownCursor
#else
            kThemeClosedHandCursor
#endif
            ;
            break;
        case CelestiaCore::SizeHorCursor:
            changedCursor =
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
            kThemeResizeLeftRightCursor
#else
            kThemeClosedHandCursor
#endif
            ;
            break;
        default:
            changedCursor = kThemeArrowCursor;
            break;                
        }

        if (changedCursor != cursor)
        {
            SetThemeCursor(changedCursor);
            cursor = changedCursor;
            shape = aShape;
        }
    };

    virtual CelestiaCore::CursorShape getCursorShape() const
    {
        return shape;
    };
    
private:
    ThemeCursor cursor;
    CelestiaCore::CursorShape shape;
};


CelestiaAppCore *_sharedCelestiaAppCore;
CelestiaCore *appCore;

@implementation CelestiaAppCore

-(CelestiaCore*) appCore
    {return appCore; };

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


+(void)initialize
{
    _sharedCelestiaAppCore = nil;
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
    appCore = NULL;
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
    if (_destinations != nil) {
        [_destinations release];
        _destinations = nil;
    }
    if (appCore != NULL) {
        // appCore doesn't own the custom alerter and cursor
        // handler for some reason so we assume responsibility
        // for cleanup
        if (appCore->getAlerter())
            delete appCore->getAlerter();
        if (appCore->getCursorHandler())
            delete appCore->getCursorHandler();
        delete appCore;
        appCore = NULL;
    }
    _sharedCelestiaAppCore = nil;
    [super dealloc];
}

-(BOOL)initSimulation
{
    BOOL result = NO;
    appCore = new CelestiaCore();
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSString *confFileSetting;
    std::string confFile;
    NSArray *existingResourceDirsSetting;
    NSArray *extrasDirsSetting;
    std::vector<std::string> extrasDirs;
    NSString *extrasDir = nil;
    MacOSXSplashProgressNotifier progressNotifier;

    if ((confFileSetting = [prefs stringForKey:@"conf"]))
    {
        confFile = [confFileSetting stdString];
    }

    if ((existingResourceDirsSetting = [prefs stringArrayForKey:@"existingResourceDirs"]))
    {
        NSFileManager *fm = [NSFileManager defaultManager];
        BOOL isFolder = NO;
        NSEnumerator *resouceDirEnum = [existingResourceDirsSetting objectEnumerator];
        NSString *resourceDir = nil;
        NSString *existingConfFile = nil;
        while ((resourceDir = [resouceDirEnum nextObject]))
        {
            existingConfFile = [resourceDir stringByAppendingPathComponent:@"celestia.cfg"];
            CelestiaConfig *config = ReadCelestiaConfig([existingConfFile stdString], NULL);
            if (config)
            {
                for (vector<string>::const_iterator iter = config->extrasDirs.begin();
                     iter != config->extrasDirs.end(); iter++)
                {
                    if (*iter != "")
                    {
                        extrasDir = [NSString stringWithStdString: (*iter)];
                        if (([fm fileExistsAtPath: extrasDir = [extrasDir stringByStandardizingPath] isDirectory: &isFolder] && isFolder) ||
                            [fm fileExistsAtPath: extrasDir = [resourceDir stringByAppendingPathComponent:extrasDir] isDirectory: &isFolder] && isFolder)
                        {
                            extrasDirs.push_back([extrasDir stdString]);
                        }
                    }
                }
                delete config;
            }
            else
            {
                if ([fm fileExistsAtPath: extrasDir = [resourceDir stringByAppendingPathComponent: @"extras"] isDirectory: &isFolder] && isFolder)
                    extrasDirs.push_back([extrasDir stdString]);
            }

        }
    }
    if ((extrasDirsSetting = [prefs stringArrayForKey:@"extrasDirs"]))
    {
        NSEnumerator *iter = [extrasDirsSetting objectEnumerator];
        while ((extrasDir = [iter nextObject]))
            extrasDirs.push_back([extrasDir stdString]);
    }

    appCore->setAlerter(new MacOSXAlerter());
    appCore->setCursorHandler(new MacOSXCursorHandler());
    result = appCore->initSimulation(!confFile.empty() ? &confFile : nil,
                                     &extrasDirs,
                                     &progressNotifier);
    if (result)
    {
        CelestiaSettings *settings = [CelestiaSettings shared];
        new MacSettingsWatcher(self, settings); // adds itself to the appCore
    }

    return result;
}

-(BOOL)initRenderer
{
    return (BOOL)appCore->initRenderer();
}

-(void)start:(NSDate *)date
{
    appCore->start([[Astro julianDate:date] doubleValue]);
}

-(void)charEntered:(char)c withModifiers:(int)modifiers
{
// moved to CelestiaOpenGLView...    if (c == 127) c = 8; 
    appCore->charEntered(c, modifiers);
}

-(void)charEntered:(NSString *)string
{
    appCore->charEntered([string UTF8String]);
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

-(void)mouseMove:(NSPoint)coord
{
    appCore->mouseMove(coord.x,coord.y);
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
    appCore->setTimeZoneName([[timeZone abbreviationForDate:date] stdString]);
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

-(void)back
{
    appCore->back();
}

-(void)forward
{
    std::vector<Url>::size_type historySize = appCore->getHistory().size();
    if (historySize < 2) return;
    if (appCore->getHistoryCurrent() > historySize-2) return;
    appCore->forward();
}

-(NSString *) currentURL
{
    CelestiaState appState;
    appState.captureState(appCore);
    Url currentUrl(appState, Url::CurrentVersion);
    
    NSString *url = [ NSString stringWithStdString: currentUrl.getAsString() ];
    return url;
}

-(void)goToUrl:(NSString *)url
{
    appCore->goToUrl([url stdString]);
}

-(void)setStartURL:(NSString *)url
{
    appCore->setStartURL([url stdString]);
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
    appCore->runScript([fileName stdString]);
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
            url = sel.star()->getInfoURL();
            if (url.empty())
            {
                char name[32];
                sprintf(name, "HIP%d", sel.star()->getCatalogNumber() & ~0xf0000000);                
                url = string("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=") + name;
            }
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


    if (!url.empty())
    {
        NSString *unescUrl = [NSString stringWithStdString: url];
        NSString *escUrl =
            (NSString *)CFURLCreateStringByAddingPercentEscapes(NULL, (CFStringRef)unescUrl, NULL, NULL, kCFStringEncodingUTF8);

        if (escUrl)
        {
            if ([escUrl length] > 0)
            {
                NSURL *theURL = [NSURL URLWithString: escUrl];

                if (theURL != nil)
                {
                    if (nil == [theURL scheme])
                        theURL = [NSURL fileURLWithPath: unescUrl];
                    if (theURL)
                        [[NSWorkspace sharedWorkspace] openURL: theURL];
                }
            }

            [escUrl release];
        }
    }
}

-(unsigned int)aaSamples
{
    return appCore->getConfig()->aaSamples;
}

-(BOOL)glExtensionIgnored:(NSString *)extension
{
    CelestiaConfig *cfg = appCore->getConfig();
    return std::find(cfg->ignoreGLExtensions.begin(), cfg->ignoreGLExtensions.end(), [extension stdString]) != cfg->ignoreGLExtensions.end();
}

-(void)setGLExtensionIgnored:(NSString *)extension
{
    appCore->getConfig()->ignoreGLExtensions.push_back([extension stdString]);
}

- (BOOL) captureMovie: (NSString*) filename width: (int) width height: (int) height
                              frameRate: (float) framerate
{
    MovieCapture* movieCapture = new QTCapture();

    bool success = movieCapture->start([filename stdString], width, height, framerate);
    if (success)
        appCore->initMovieCapture(movieCapture);
    else
        delete movieCapture;

    return success;
}

@end
