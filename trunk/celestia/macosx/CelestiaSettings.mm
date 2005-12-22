//
//  CelestiaSettings.mm
//  celestia
//
//  Created by Hank Ramsey on Fri Oct 29 2004.
//  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
//

#import "CelestiaSettings.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaController.h"
#import "CelestiaAppCore.h"
#include "celestiacore.h"
#include "celestia.h"
#include "render.h"

class MacOSXWatcher : public CelestiaWatcher
{
    private:
        CelestiaSettings *control;

    public:
    
        MacOSXWatcher(CelestiaCore* _appCore, CelestiaSettings* _control) : CelestiaWatcher(*_appCore), control(_control)
        {};
            
    public:

    void notifyChange(CelestiaCore* core, int i)
    {
        if ( 0 == i & (
        CelestiaCore::LabelFlagsChanged 
        | CelestiaCore::RenderFlagsChanged 
        | CelestiaCore::VerbosityLevelChanged 
        | CelestiaCore::TimeZoneChanged       
        | CelestiaCore::AmbientLightChanged   
        | CelestiaCore::FaintestChanged       
        )) { return; } 
        [control validateItems];
    };
};

@implementation CelestiaSettings

        static CelestiaCore *appCore;

#define CS_DefaultsName @"Celestia-1.4.0"
#define CS_NUM_PREV_VERSIONS 1
#define TAGDEF(tag,key) key, [NSNumber numberWithInt: tag], 

static NSString *CS_PREV_VERSIONS[CS_NUM_PREV_VERSIONS] = {
    @"Celestia-1.3.2"
};

CelestiaSettings* sharedInstance;


+(CelestiaSettings*) shared
{
    // class method to get single shared instance
    if (sharedInstance == nil) {
        sharedInstance = [[CelestiaSettings alloc]init];
    }
    return sharedInstance;
}

static CelestiaController *control;
static NSMutableDictionary* tagMap;

-(CelestiaSettings*)init
{
    if (sharedInstance != nil) {
        [[super init] release];
        return sharedInstance;
    }
    
    self = [super init];
    
    tagMap = [[ NSMutableDictionary dictionaryWithCapacity: 100 ] retain];
    
    appCore = (CelestiaCore*) [[CelestiaAppCore sharedAppCore] appCore];

     MacOSXWatcher* theWatcher = new MacOSXWatcher(appCore,self);
     appCore->addWatcher(theWatcher);

    tagDict = [[NSDictionary dictionaryWithObjectsAndKeys:
//        TAGDEF(101,@"time")
//        TAGDEF(999,@"timeScale")
        TAGDEF(300,@"fullScreenMode")
        // render flags
        TAGDEF(400,@"showStars")
        TAGDEF(401,@"showPlanets")
        TAGDEF(402,@"showGalaxies")
        TAGDEF(403,@"showDiagrams")
        TAGDEF(413,@"showBoundaries")
        TAGDEF(404,@"showCloudMaps")
        TAGDEF(407,@"showNightMaps")
        TAGDEF(408,@"showAtmospheres")
        TAGDEF(415,@"showCometTails")
        TAGDEF(416,@"showMarkers")
        TAGDEF(405,@"showOrbits")
        TAGDEF(417,@"showNebulae")
        TAGDEF(418,@"showOpenClusters")
//	TAGDEF(999,@"showPartialTrajectories")
        TAGDEF(409,@"showSmoothLines")
        TAGDEF(410,@"showEclipseShadows")
        TAGDEF(412,@"showRingShadows")
        TAGDEF(411,@"showStarsAsPoints")
        TAGDEF(414,@"showAutoMag")
        TAGDEF(406,@"showCelestialSphere")
        // object labels
        TAGDEF(500,@"showStarLabels")
        TAGDEF(501,@"showPlanetLabels")
        TAGDEF(502,@"showMoonLabels")
        TAGDEF(503,@"showConstellationLabels")
        TAGDEF(504,@"showGalaxyLabels")
        TAGDEF(505,@"showAsteroidLabels")
        TAGDEF(506,@"showSpacecraftLabels")
        TAGDEF(507,@"showLocationLabels")
        TAGDEF(508,@"showCometLabels")
        TAGDEF(509,@"showNebulaLabels")
        TAGDEF(510,@"showOpenClusterLabels")
        // popups
        TAGDEF(600,@"altSurface")
        TAGDEF(610,@"hudDetail")
        TAGDEF(620,@"starStyle")
        TAGDEF(630,@"renderPath")
        TAGDEF(640,@"resolution")
        // orbits
//        TAGDEF(999,@"minimumOrbitSize")
        TAGDEF(700,@"showPlanetOrbits")
        TAGDEF(701,@"showMoonOrbits")
        TAGDEF(702,@"showAsteroidOrbits")
        TAGDEF(704,@"showSpacecraftOrbits")
        TAGDEF(703,@"showCometOrbits")
        // feature labels
        TAGDEF(903,@"minimumFeatureSize")
        TAGDEF(800,@"showCityLabels")
        TAGDEF(801,@"showObservatoryLabels")
        TAGDEF(802,@"showLandingSiteLabels")
        TAGDEF(803,@"showCraterLabels")
        TAGDEF(804,@"showVallisLabels")
        TAGDEF(805,@"showMonsLabels")
        TAGDEF(806,@"showPlanumLabels")
        TAGDEF(807,@"showChasmaLabels")
        TAGDEF(808,@"showPateraLabels")
        TAGDEF(809,@"showMareLabels")
        TAGDEF(810,@"showRupesLabels")
        TAGDEF(811,@"showTesseraLabels")
        TAGDEF(812,@"showRegioLabels")
        TAGDEF(813,@"showChaosLabels")
        TAGDEF(814,@"showTerraLabels")
        TAGDEF(815,@"showAstrumLabels")
        TAGDEF(816,@"showCoronaLabels")
        TAGDEF(817,@"showDorsumLabels")
        TAGDEF(818,@"showFossaLabels")
        TAGDEF(819,@"showCatenaLabels")
        TAGDEF(820,@"showMensaLabels")
        TAGDEF(821,@"showRimaLabels")
        TAGDEF(822,@"showUndaeLabels")
        TAGDEF(824,@"showReticulumLabels")
        TAGDEF(825,@"showPlanitiaLabels")
        TAGDEF(826,@"showLineaLabels")
        TAGDEF(827,@"showFluctusLabels")
        TAGDEF(828,@"showFarrumLabels")
        TAGDEF(831,@"showOtherLabels")
        // stars
//        TAGDEF(999,@"distanceLimit")
        TAGDEF(900,@"ambientLightLevel")
        TAGDEF(901,@"brightnessBias")
        TAGDEF(902,@"faintestVisible")
        TAGDEF(904,@"galaxyBrightness")
//        TAGDEF(999,@"saturationMagnitude")

        nil ] retain];
    keyArray = [[tagDict allValues] retain];
    

    return self;
}

-(void)addWatcher: (CelestiaController*) control
{
     MacOSXWatcher* theWatcher = new MacOSXWatcher(appCore,self);
     appCore->addWatcher(theWatcher);
}

-(void) setControl: (CelestiaController*) _control
{
    control = _control;
}

- (NSDictionary *) defaultsDictionary
{	
	NSMutableDictionary* theDictionary = [NSMutableDictionary dictionaryWithCapacity: [keyArray count]];
	NSEnumerator* keys = [ keyArray objectEnumerator];
        id key;
	while ( nil != (key = [keys nextObject]) )
	{
//                NSLog([NSString stringWithFormat: @"default dict entry %@ %@", key, [self valueForKey: key]]);
		[ theDictionary setObject: [self valueForKey: key] forKey: key];
	}
	return theDictionary;
}

-(NSDictionary*) findUserDefaults
{
	NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
	NSDictionary* dict = [defs objectForKey: CS_DefaultsName ];
    if (dict == nil )
    {
        // Scan for older versions
        int i = 0;
        for (; i < CS_NUM_PREV_VERSIONS; ++i)
        {
            if ((dict = [defs objectForKey:CS_PREV_VERSIONS[i]]))
                break;
        }

        if (dict)
        {
            [self upgradeUserDefaults: dict fromVersion: CS_PREV_VERSIONS[i]];
        }
    }
    return dict;
}

-(void) loadUserDefaults 
{
	id key;
//        NSLog(@"loading user defaults");
	NSDictionary* defaultsDictionary = [self findUserDefaults ];
	if ( defaultsDictionary == nil ) return;
	NSEnumerator* keys = [ defaultsDictionary keyEnumerator];
	while ( nil != (key = [keys nextObject]) )
	{
//                NSLog([NSString stringWithFormat: @"loading default dict entry %@ %@", key, [defaultsDictionary objectForKey: key] ]);
		[self takeValue: [defaultsDictionary objectForKey: key] forKey: key];
//                NSLog([NSString stringWithFormat: @"loaded default dict entry %@ %@", key, [self valueForKey: key] ]);
	}
//        NSLog(@"loaded user defaults");
}

-(void) storeUserDefaults 
{
//        NSLog(@"storing user defaults");
	NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
	[ defs setObject: [self defaultsDictionary] forKey: CS_DefaultsName ];
//        NSLog(@"stored user defaults");
}

-(void) upgradeUserDefaults: (NSDictionary *)dict fromVersion: (NSString *)old
{
    NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
    [defs setObject: dict forKey: CS_DefaultsName];
    [defs removeObjectForKey: old];
}

-(id) valueForTag: (int) tag { 
    return [self valueForKey: [tagDict objectForKey: [NSNumber numberWithInt: tag] ] ];
    };

-(void) takeValue: (id) value forTag: (int) tag 
{
//      NSLog([NSString stringWithFormat: @"take value for tag %@ %d", value, tag]);
      id key = [tagDict objectForKey: [NSNumber numberWithInt: tag] ];
//      NSLog([NSString stringWithFormat: @"take value for key %@ %@", value, key]);
     if (key!= nil) [self takeValue: value forKey: key ];
};

-(id)handleQueryWithUnboundKey: (NSString*) key
{
    if ( key != NULL) NSLog([NSString stringWithFormat: @"unbound key query for %@",key]);
    return nil;
};

-(void)handleTakeValue: (id) value forUnboundKey: (NSString*) key
{
    NSLog([NSString stringWithFormat: @"unbound key set for %@",key]);
};

-(void)unableToSetNilForKey: (NSString*) key
{
    NSLog([NSString stringWithFormat: @"nl key for %@",key]);
};

// Time Settings

-(double) time { return appCore->getSimulation()->getTime(); };
-(void) setTime: (double) value { appCore->getSimulation()->setTime(value); };

-(double) timeScale { return appCore->getSimulation()->getTimeScale(); };
-(void) setTimeScale: (double) value { appCore->getSimulation()->setTimeScale(value); };

-(BOOL) synchTime { return appCore->getSimulation()->getSyncTime(); };
-(void) setSynchTime: (BOOL) value { appCore->getSimulation()->setSyncTime(value); };

// Gaze Settings

-(float) fieldOfView { return appCore->getSimulation()->getObserver().getFOV(); };
-(void)  setFieldOfView: (float) value { appCore->getSimulation()->getObserver().setFOV(value); };

// Observer Settings

// Cruise Settings

// Velocity
// AngularVelocity

-(int) setValue: (BOOL) value forBit: (int) bit inSet: (int) set
{
    int result = value
        ? ((bit&set) ? set : (set|bit) )
        : ((bit&set) ?  (set^bit) : set);
//    NSLog([NSString stringWithFormat: @"setValue %d forBit: %d inSet: %d = %d",value,bit,set,result]);
//    NSLog([NSString stringWithFormat: @"bit was: %d bit is: %d ",(set&bit),(result&bit)]);
    return result;
};

// Visibility Settings

#define RENDERMETHODS(flag)  -(BOOL) show##flag { return (appCore->getRenderer()->getRenderFlags()&Renderer::Show##flag) != 0; } -(void) setShow##flag: (BOOL) value  { appCore->getRenderer()->setRenderFlags( [self setValue: value forBit: Renderer::Show##flag inSet: appCore->getRenderer()->getRenderFlags() ] ); } 

RENDERMETHODS(Stars)
RENDERMETHODS(Planets)
RENDERMETHODS(Galaxies)
RENDERMETHODS(Diagrams)
RENDERMETHODS(Boundaries)
RENDERMETHODS(CloudMaps)
RENDERMETHODS(NightMaps)
RENDERMETHODS(Atmospheres)
RENDERMETHODS(CometTails)
RENDERMETHODS(Markers)
RENDERMETHODS(Orbits)
RENDERMETHODS(PartialTrajectories)
RENDERMETHODS(SmoothLines)
RENDERMETHODS(EclipseShadows)
RENDERMETHODS(RingShadows)
RENDERMETHODS(StarsAsPoints)
RENDERMETHODS(AutoMag)
RENDERMETHODS(CelestialSphere)
RENDERMETHODS(Nebulae)
RENDERMETHODS(OpenClusters)

// Label Settings

#define LABELMETHODS(flag)  -(BOOL) show##flag##Labels { return (appCore->getRenderer()->getLabelMode()&Renderer::flag##Labels) != 0; } -(void) setShow##flag##Labels : (BOOL) value  { appCore->getRenderer()->setLabelMode( [self setValue: value forBit: Renderer::flag##Labels inSet: appCore->getRenderer()->getLabelMode()] ); } 

LABELMETHODS(Star)
LABELMETHODS(Planet)
LABELMETHODS(Moon)
LABELMETHODS(Constellation)
LABELMETHODS(Galaxy)
LABELMETHODS(Asteroid)
LABELMETHODS(Spacecraft)
LABELMETHODS(Location)
LABELMETHODS(Comet)
LABELMETHODS(Nebula)
LABELMETHODS(OpenCluster)

// Orbit Settings

#define ORBITMETHODS(flag)  -(BOOL) show##flag##Orbits { return (appCore->getRenderer()->getOrbitMask()&Body::flag) != 0; } -(void) setShow##flag##Orbits: (BOOL) value  { appCore->getRenderer()->setOrbitMask([self setValue: value forBit: Body::flag inSet: appCore->getRenderer()->getOrbitMask()]); } 

ORBITMETHODS(Planet)
ORBITMETHODS(Moon)
ORBITMETHODS(Asteroid)
ORBITMETHODS(Spacecraft)
ORBITMETHODS(Comet)


-(float) minimumOrbitSize { return appCore->getRenderer()->getMinimumOrbitSize(); };
-(void)  setMinimumOrbitSize: (float) value { appCore->getRenderer()->setMinimumOrbitSize(value); };

// Feature Settings

#define FEATUREMETHODS(flag)  -(BOOL) show##flag##Labels { return (appCore->getSimulation()->getObserver().getLocationFilter()&Location::flag) != 0; } -(void) setShow##flag##Labels: (BOOL) value  { appCore->getSimulation()->getObserver().setLocationFilter([self setValue: value forBit: Location::flag inSet: appCore->getSimulation()->getObserver().getLocationFilter()]); } 

FEATUREMETHODS(City)
FEATUREMETHODS(Observatory)
FEATUREMETHODS(LandingSite)
FEATUREMETHODS(Crater)
FEATUREMETHODS(Vallis)
FEATUREMETHODS(Mons)
FEATUREMETHODS(Planum)
FEATUREMETHODS(Chasma)
FEATUREMETHODS(Patera)
FEATUREMETHODS(Mare)
FEATUREMETHODS(Rupes)
FEATUREMETHODS(Tessera)
FEATUREMETHODS(Regio)
FEATUREMETHODS(Chaos)
FEATUREMETHODS(Terra)
FEATUREMETHODS(Astrum)
FEATUREMETHODS(Corona)
FEATUREMETHODS(Dorsum)
FEATUREMETHODS(Fossa)
FEATUREMETHODS(Catena)
FEATUREMETHODS(Mensa)
FEATUREMETHODS(Rima)
FEATUREMETHODS(Undae)
FEATUREMETHODS(Reticulum)
FEATUREMETHODS(Planitia)
FEATUREMETHODS(Linea)
FEATUREMETHODS(Fluctus)
FEATUREMETHODS(Farrum)
FEATUREMETHODS(Other)

-(float) minimumFeatureSize { return appCore->getRenderer()->getMinimumFeatureSize(); };
-(void)  setMinimumFeatureSize: (float) value { appCore->getRenderer()->setMinimumFeatureSize(value); };

// Lighting Settings

-(float) ambientLightLevel { return appCore->getRenderer()->getAmbientLightLevel(); };
-(void)  setAmbientLightLevel: (float) value { appCore->getRenderer()->setAmbientLightLevel(value); };

-(float) galaxyBrightness { return Galaxy::getLightGain(); };
-(void)  setGalaxyBrightness: (float) value { Galaxy::setLightGain(value); };

// Star Settings

-(float) distanceLimit { return appCore->getRenderer()->getDistanceLimit(); };
-(void)  setDistanceLimit: (float) value { appCore->getRenderer()->setDistanceLimit(value); };

-(float) faintestVisible 
{ 
//    return appCore->getSimulation()->getFaintestVisible(); 
    if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    {
        return appCore->getSimulation()->getFaintestVisible();
    }
    else
    {
        return appCore->getRenderer()->getFaintestAM45deg();
    }                
};

-(void)  setFaintestVisible: (float) value 
{ 
//    appCore->getSimulation()->setFaintestVisible(value); 
    if ((appCore->getRenderer()->getRenderFlags() & Renderer::ShowAutoMag) == 0)
    {
        appCore->setFaintest(value);
    }
    else
    {
        appCore->getRenderer()->setFaintestAM45deg(value);
        appCore->setFaintestAutoMag();
    }                
};

// Brightness Settings

-(float) saturationMagnitude { return appCore->getRenderer()->getSaturationMagnitude(); } ;
-(void)  setSaturationMagnitude: (float) value { appCore->getRenderer()->setSaturationMagnitude(value); };

-(float) brightnessBias { return appCore->getRenderer()->getBrightnessBias(); } ;
-(void)  setBrightnessBias: (float) value { appCore->getRenderer()->setBrightnessBias(value); };

-(int)  starStyle { return appCore->getRenderer()->getStarStyle(); }  ;
-(void) setStarStyle: (int) value { appCore->getRenderer()->setStarStyle((Renderer::StarStyle)value); };

// Texture Settings

-(int)  resolution { return appCore->getRenderer()->getResolution(); }  ;
-(void) setResolution: (int) value { appCore->getRenderer()->setResolution(value); };

// Overlay Settings

-(int)  hudDetail { return appCore->getHudDetail(); }  ;
-(void) setHudDetail: (int) value { appCore->setHudDetail(value); };

// Other Settings

-(int)  renderPath { return appCore->getRenderer()->getGLContext()->getRenderPath(); }  ;
-(void) setRenderPath: (int) value { appCore->getRenderer()->getGLContext()->setRenderPath((GLContext::GLRenderPath)value); };

-(int)  fullScreenMode { return [[control valueForKey: @"isFullScreen"] intValue]; }
-(void) setFullScreenMode: (int) value { [control takeValue: [NSNumber numberWithBool: (value != 0)] forKey: @"isFullScreen"]; }

// Alt Surface Setting

/*- (BOOL) validateAltSurface: (int) index
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
*/

- (int) altSurface
{
    // return alternate surface index
    string displayedSurfName = appCore->getSimulation()->getActiveObserver()->getDisplayedSurface();
    Selection sel = appCore->getSimulation()->getSelection();
    if (sel.body() != NULL)
    {
        vector<string>* surfNames = sel.body()->getAlternateSurfaceNames();
        if (surfNames != NULL)
        {
            int index;
            string surfName;
            for (index = 0 ; index < (int)surfNames->size(); index++)
            {
                surfName = surfNames->at(index);
                if (surfName  == displayedSurfName )
                { 
                    delete surfNames;
                    return index+1;
                }
            }
        }
    }
    return 0;
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
            [newItem setTitle: [ NSString stringWithStdString: (*surfaces)[i] ] ];
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

/* not yet...

// Shader Settings

    bool getFragmentShaderEnabled() const;
    void setFragmentShaderEnabled(bool);
    bool fragmentShaderSupported() const;
    bool getVertexShaderEnabled() const;
    void setVertexShaderEnabled(bool);
    bool vertexShaderSupported() const;

*/

// GUI Tag Methods ----------------------------------------------------------


- (void) selectPopUpButtonItem: (id) item withIndex: (int) index
{
    id item2 = [ tagMap objectForKey: [NSNumber numberWithInt: ([item tag]-index) ]];
    if ([item2 isKindOfClass: [NSPopUpButton class]])
    {
        int popUpIndex = [item2 indexOfItemWithTag: [item tag]];
        if (popUpIndex >= 0)
            [item2 selectItemAtIndex: popUpIndex];
    }
}

- (void) actionForItem: (id) item
{
    int tag = [item tag];
    id value;

    if ( tag <= 128 ) tag = [self tagForKey: tag ]; // handle menu item for setting; obsolete?
    if ( tag <= 128 ) { appCore->charEntered(tag); return; };
    switch ( tag/100)
    {
        case 4: case 5: case 7: case 8: // 400, 500, 700, 800
            [self takeValue: [NSNumber numberWithInt: (1-[[self valueForTag: tag] intValue])] forTag: tag ];
        break;
        case 6: // 600
            value = [NSNumber numberWithInt: tag%10 ];
            [self takeValue: value forTag: (tag/10)*10 ];
            [self selectPopUpButtonItem: item withIndex: tag%10];
            //[self setPopUpButtonWithTag: ((tag/10)*10) toIndex: [value intValue]];
            if ([[tagDict objectForKey:[NSNumber numberWithInt:((tag/10)*10)]] isEqualToString: @"renderPath"])
            {
                value = [self valueForTag: (tag/10)*10];
                if ([value intValue] != (tag%10))
                    [[control valueForKey:@"renderPanelController"] performSelector:@selector(displayRenderPathWarning:) withObject:[item title]];
                else
                    [[control valueForKey:@"renderPanelController"] performSelector:@selector(hideRenderPathWarning)];
            }
        break;
        case 9: // 900
            [self takeValue: [NSNumber numberWithFloat: [item floatValue]] forTag: tag];
        break;
    }
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
//        [item setState:  (fabs(appCore->getSimulation()->getTimeScale()) == 0.0 ) ? NSOnState : NSOffState ]; 
    }
    else if ( tag <= 128 ) 
    {
        [item setState: NSOffState ];
    }
    else 
    {
	switch ( tag/100)
	{
		case 4: case 5: case 7: case 8:
		// 400s, 500s, 700s, 800s (checkboxes)
		        [item setState: [[self valueForTag: tag] intValue] ? NSOnState : NSOffState ];
		break;
		case 6:
		// 600s (popups)
                    if ( tag >= 600 && tag < 610 ) // altSurface menu
                    {
                        int index = tag-600;
                        [item setState:  ([self altSurface] == index ) ? NSOnState: NSOffState ];            
                    } 
                    else
                        [self validateButton: item atIndex: tag%10 withValue: [[self valueForTag: tag-(tag%10)] intValue] ];
		break;
		case 9:
		// 900s (floats)
	                [item setFloatValue: [[self valueForTag: tag] floatValue] ];                
		break;
	}
    }
    return YES;
}


- (void) defineKeyForItem: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        NSNumber* itemKey = [NSNumber numberWithInt: tag ];
        if ( [ tagMap objectForKey: itemKey ] == 0 )
        {
            [tagMap setObject: item forKey: itemKey];
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
            case  87: tag = 508; break;  // LabelComets
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

- (void) validateItemForTag: (int) tag
{
    if ( tag <= 128 ) tag = [self tagForKey: tag ];
    id item = [ tagMap objectForKey: [NSNumber numberWithInt: tag ]];
    if ( [item isKindOfClass: [NSMenuItem class]] ) return; // auto-validated
    if ( item != 0 )
    {
        [ self validateItem: item ];
    }
}

- (void) validateItems
{
    id obj;
    NSEnumerator* enumerator = [[tagMap allKeys] objectEnumerator];
    while ((obj = [enumerator nextObject]) != nil)
    {
        [self validateItemForTag: [obj intValue]];
    }
}


- (BOOL)     validateMenuItem: (id) item
{
//    if ( [startupCondition condition] == 0 ) return NO;
    if ( [item tag] == 0 )
    {
        return [item isEnabled];
    }
    else
    {
        return [self validateItem: item ];
    }
}

- (IBAction) activateMenuItem: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        if ( tag < 0 ) // simulate key press and hold
        {
            [control keyPress: -tag hold: 2];
        } 
        else 
        {
            [self actionForItem: item ];
        }
        [self validateItemForTag: tag];
    }
}

- (IBAction) activateSwitchButton: (id) item
{
    int tag = [item tag];
    if ( tag != 0 )
    {
        [self actionForItem: item];
        [control forceDisplay]; 
    }
}

- (IBAction) activateMatrixButton: (id) item
{
    item = [item selectedCell];
    int tag = [item tag];
    if ( tag != 0 )
    {
        [self actionForItem: item];
    }
}

- (void) scanForKeys: (id) item
{
//    NSLog(@"scanning item %@\n", [item description]);

    if ( [item isKindOfClass: [NSTabViewItem class]] )
    {
//	 NSLog(@"scanning viewitem\n");
        item = [item view];
        [ self scanForKeys: item ];
        return;
    }


    if ([item isKindOfClass: [NSMenuItem class]] && [item tag] != 0)
        {
//	 NSLog(@"scanning menuItem\n");
            [self defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateMenuItem:)];
            return; 
        }
    else if ([item isKindOfClass: [NSSlider class]] && [item tag] != 0)
        {
//            NSLog(@"scanning cell\n");
            [self defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateSwitchButton:)];
            return; 
        }

    if ( [item isKindOfClass: [NSTabView class]] )
    {
//	 NSLog(@"scanning tabview\n");
        item = [ [item tabViewItems] objectEnumerator ];
    }
    else if ( [item isKindOfClass: [NSPopUpButton class]] )
    {
        [self defineKeyForItem: item];
//	 NSLog(@"scanning popupbutton\n");
        item = [ [item itemArray] objectEnumerator ];
    }
    else if ([item isKindOfClass: [NSControl class]] && [item tag] != 0)
        {
//	 NSLog(@"scanning control\n");
            [self defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateSwitchButton:)];
            return; 
        }
    else if ( [item isKindOfClass: [NSMatrix class]] )
    {
//	 NSLog(@"scanning matrix\n");
        item = [ [item cells] objectEnumerator ];
    }
    else if ( [item isKindOfClass: [NSView class]] )
    {
//	 NSLog(@"scanning view\n");
//        NSLog(@"subviews items = %@\n", [item subviews]);
        item = [ [item subviews] objectEnumerator ];
//        NSLog(@"view items = %@\n", item);
    };

    if ( [item isKindOfClass: [NSEnumerator class] ] )
    {
//	 NSLog(@"scanning array\n");
        id subitem;
        while(subitem = [item nextObject])
        {
//            NSLog(@"scanning array  item\n");
            [ self scanForKeys: subitem ];
        }
        return;
    }

    if ([item isKindOfClass: [NSCell class]] && [item tag] != 0)
        {
//	 NSLog(@"scanning cell\n");
            [self defineKeyForItem: item];
            [item setTarget: self];
            [item setAction: @selector(activateMatrixButton:)];
            return; 
        }

    if ( [item isKindOfClass: [NSWindow class]] )
    {
//        NSLog(@"scanning window\n");
        item = [item contentView ];
        [ self scanForKeys: item ];
        return;
    }
}

@end
