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

#import "CelestiaController.h"
#include "celestiacore.h"
#include "qtcapture.h"

class MacOSXAlerter : public CelestiaCore::Alerter
{
public:
    MacOSXAlerter() {};
    ~MacOSXAlerter() {};

    void fatalError(const std::string& msg)
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
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSString *confFileSetting;
    std::string confFile;
    NSArray *extrasDirsSetting;
    std::vector<std::string> extrasDirs;
    NSString *extrasDir;
    MacOSXSplashProgressNotifier progressNotifier;

    if ((confFileSetting = [prefs stringForKey:@"conf"]))
    {
        confFile = [confFileSetting stdString];
    }

#ifndef NO_VP_WORKAROUND

    std::string VP_PROBLEM_EXT      = "GL_ARB_vertex_program";
    NSString    *VP_PATCH_SCRIPT    = @"vp_patch.sh";
    NSString    *VP_PATCH_SHELL     = @"/bin/zsh";
    NSString    *CELESTIA_CFG       = @"celestia.cfg";

    const char *VP_PROBLEM_RENDERERS[] = { "ATI Radeon 9200" };
    const char *glRenderer = (const char *) glGetString(GL_RENDERER);
    BOOL shouldWorkaround = NO;
    size_t i = 0;

    if (glRenderer)
    {
        for (; i < (sizeof VP_PROBLEM_RENDERERS)/sizeof(char *); ++i)
        {
            if (strstr(glRenderer, VP_PROBLEM_RENDERERS[i]))
            {
                shouldWorkaround = YES;
                break;
            }
        }
    }

    if (shouldWorkaround)
    {
        // Check if workaround is already installed
        CelestiaConfig *cfg = ReadCelestiaConfig([CELESTIA_CFG stdString]);

        if (cfg && std::find(cfg->ignoreGLExtensions.begin(), cfg->ignoreGLExtensions.end(), VP_PROBLEM_EXT) == cfg->ignoreGLExtensions.end())
        {
            delete cfg;
            if (NSRunAlertPanel([NSString stringWithFormat: NSLocalizedString(@"It appears you are running Celestia on %s hardware. Do you wish to install a workaround?",nil), VP_PROBLEM_RENDERERS[i]],
                                [NSString stringWithFormat: NSLocalizedString(@"A shell script will be run to modify your %@, adding an IgnoreGLExtensions directive. This can prevent freezing issues.",nil), CELESTIA_CFG],
                                NSLocalizedString(@"Yes",nil),
                                NSLocalizedString(@"No",nil),
                                nil) == NSAlertDefaultReturn)
            {
                // Install it
                NSFileManager *fm = [NSFileManager defaultManager];
                NSString *cfgPath = [[fm currentDirectoryPath] stringByAppendingPathComponent: CELESTIA_CFG];
                NSString *toolPath = [[NSBundle mainBundle] pathForResource: VP_PATCH_SCRIPT ofType: @""];
                BOOL patchInstalled = NO;

                if ([fm isWritableFileAtPath: cfgPath] && toolPath)
                {
                    NSArray *taskArgs = [NSArray arrayWithObjects:
                        toolPath, cfgPath, nil];
                    NSTask *theTask = [NSTask launchedTaskWithLaunchPath: VP_PATCH_SHELL
                                                               arguments: taskArgs];
                    if (theTask)
                    {
                        [theTask waitUntilExit];
                        patchInstalled = ([theTask terminationStatus] == 0);
                    }
                }

                if (patchInstalled)
                {
                    NSRunAlertPanel(NSLocalizedString(@"Workaround successfully installed.",nil),
                                    [NSString stringWithFormat: NSLocalizedString(@"Your original %@ has been backed up.",nil), CELESTIA_CFG],
                                    nil, nil, nil);
                }
                else
                {
                    [[CelestiaController shared] fatalError: NSLocalizedString(@"There was a problem installing the workaround. You may be running from a disk image, which is write-protected. Please try copying the CelestiaResources folder to your home directory as described in the README. You can also attempt to perform the workaround manually by following the instructions in the README.",nil)];
                    [[CelestiaController shared] fatalError: nil];
                }
            }
        }
    }
#endif NO_VP_WORKAROUND

    if ((extrasDirsSetting = [prefs stringArrayForKey:@"extrasDirs"]))
    {
        NSEnumerator *iter = [extrasDirsSetting objectEnumerator];
        while ((extrasDir = [iter nextObject]))
            extrasDirs.push_back([extrasDir stdString]);
    }

    return (BOOL)appCore->initSimulation(!confFile.empty() ? &confFile : nil,
                                         extrasDirsSetting ? &extrasDirs : nil,
                                         &progressNotifier);
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

-(void)charEntered:(char)c withModifiers:(int)modifiers
{
// moved to CelestiaOpenGLView...    if (c == 127) c = 8; 
    appCore->charEntered(c, modifiers);
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
    std::vector<Url>::size_type historySize = appCore->getHistory().size();
    if (historySize < 2) return;
    if (appCore->getHistoryCurrent() > historySize-2) return;
    appCore->forward();
}

- (NSString *) currentURL
{
     Url currentUrl = Url(appCore, Url::Absolute);
//   NSURL *url = [NSURL URLWithString: [ NSString stringWithStdString: currentUrl.getAsString() ]];
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
    appCore->runScript([fileName cString]);
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
                    [[NSWorkspace sharedWorkspace] openURL: theURL];
            }

            [escUrl release];
        }
    }
}

-(unsigned int)aaSamples
{
    return appCore->getConfig()->aaSamples;
}

- (BOOL) captureMovie: (NSString*) filename width: (int) width height: (int) height
                              frameRate: (float) framerate
{
    MovieCapture* movieCapture = new QTCapture();

    bool success = movieCapture->start([filename cString], width, height, framerate);
    if (success)
        appCore->initMovieCapture(movieCapture);
    else
        delete movieCapture;

    return success;
}

@end

