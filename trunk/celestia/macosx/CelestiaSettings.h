//
//  CelestiaSettings.h
//  celestia
//
//  Created by Hank Ramsey on Fri Oct 29 2004.
//  Copyright (C) 2007, Celestia Development Team
//

@interface CelestiaSettings : NSObject {
    @private
    NSMutableDictionary* tagDict;
    NSArray* keyArray;
}

+(CelestiaSettings*) shared ;

-(CelestiaSettings*)init ;

-(void) setControl: (id) _control;

-(id) valueForTag: (int) tag; 
-(void) takeValue: (id) value forTag: (int) tag; 

// User Defaults 

- (NSDictionary *) defaultsDictionary ;

-(NSDictionary*) findUserDefaults ;
-(NSDictionary*) findAppDefaults ;

-(void) loadUserDefaults ;
-(void) loadAppDefaults ;

-(void) storeUserDefaults ;

-(void) upgradeUserDefaults: (NSDictionary *)dict fromVersion: (NSString *)old ;

// Time Settings

-(double) time ;
-(void) setTime: (double) value ;
-(int)  timeZone ;
-(void) setTimeZone: (int) value ;

-(double) timeScale ;
-(void) setTimeScale: (double) value ;

-(BOOL) synchTime;
-(void) setSynchTime: (BOOL) value ;

// Gaze Settings

-(float) fieldOfView ;
-(void)  setFieldOfView: (float) value ;

// situation

// Cruise Settings

// Velocity
// AngularVelocity

// Visibility Settings

-(BOOL) showStars ;
-(void) setShowStars: (BOOL) value ; 

-(BOOL) showPlanets ;
-(void) setShowPlanets: (BOOL) value ; 

-(BOOL) showGalaxies ; 
-(void) setShowGalaxies: (BOOL) value ; 

-(BOOL) showDiagrams ; 
-(void) setShowDiagrams: (BOOL) value ; 

-(BOOL) showCloudMaps ; 
-(void) setShowCloudMaps: (BOOL) value ; 

-(BOOL) showOrbits ;
-(void) setShowOrbits: (BOOL) value ; 

-(BOOL) showCelestialSphere ;
-(void) setShowCelestialSphere: (BOOL) value ; 

-(BOOL) showNightMaps ;
-(void) setShowNightMaps: (BOOL) value ; 

-(BOOL) showAtmospheres ;
-(void) setShowAtmospheres: (BOOL) value ; 

-(BOOL) showSmoothLines ;
-(void) setShowSmoothLines: (BOOL) value ; 

-(BOOL) showEclipseShadows ;
-(void) setShowEclipseShadows: (BOOL) value ; 

-(BOOL) showRingShadows ;
-(void) setShowRingShadows: (BOOL) value ; 

-(BOOL) showCloudShadows ;
-(void) setShowCloudShadows: (BOOL) value ;

-(BOOL) showBoundaries ;
-(void) setShowBoundaries: (BOOL) value ; 

-(BOOL) showAutoMag ;
-(void) setShowAutoMag: (BOOL) value ; 

-(BOOL) showCometTails ;
-(void) setShowCometTails: (BOOL) value ; 

-(BOOL) showMarkers ;
-(void) setShowMarkers: (BOOL) value ; 

-(BOOL) showPartialTrajectories ; 
-(void) setShowPartialTrajectories: (BOOL) value ; 

// Label Settings

// -(BOOL) showNoLabels ;
// -(void) setShowNoLabels: (BOOL) value ;

-(BOOL) showStarLabels ;
-(void) setShowStarLabels: (BOOL) value ; 

-(BOOL) showPlanetLabels ;
-(void) setShowPlanetLabels: (BOOL) value ; 

-(BOOL) showMoonLabels ;
-(void) setShowMoonLabels: (BOOL) value ; 

-(BOOL) showConstellationLabels ; 
-(void) setShowConstellationLabels: (BOOL) value ; 

-(BOOL) showGalaxyLabels ;
-(void) setShowGalaxyLabels: (BOOL) value ; 

-(BOOL) showAsteroidLabels ;
-(void) setShowAsteroidLabels: (BOOL) value ; 

-(BOOL) showSpacecraftLabels ;
-(void) setShowSpacecraftLabels: (BOOL) value ; 

-(BOOL) showLocationLabels ;
-(void) setShowLocationLabels: (BOOL) value ; 

-(BOOL) showCometLabels ;
-(void) setShowCometLabels: (BOOL) value ; 

// -(BOOL) showBodyLabels ;
// -(void) setShowBodyLabels: (BOOL) value ; 

// Orbit Settings

-(BOOL) showPlanetOrbits ;
-(void) setShowPlanetOrbits: (BOOL) value ; 

-(BOOL) showMoonOrbits ;
-(void) setShowMoonOrbits: (BOOL) value ; 

-(BOOL) showAsteroidOrbits ; 
-(void) setShowAsteroidOrbits: (BOOL) value ; 

-(BOOL) showCometOrbits ;
-(void) setShowCometOrbits: (BOOL) value ; 

-(BOOL) showSpacecraftOrbits ;
-(void) setShowSpacecraftOrbits: (BOOL) value ;

-(BOOL) showStellarOrbits ;
-(void) setShowStellarOrbits: (BOOL) value ;

-(float) minimumOrbitSize ;
-(void)  setMinimumOrbitSize: (float) value ; 

// Location Visibility Settings

// Feature Settings

-(float) minimumFeatureSize ;
-(void)  setMinimumFeatureSize: (float) value ; 

// Lighting Settings

-(float) ambientLightLevel ;
-(void)  setAmbientLightLevel: (float) value ; 

// Star Settings

-(float) distanceLimit ;
-(void)  setDistanceLimit: (float) value ; 

-(float) faintestVisible ;
-(void)  setFaintestVisible: (float) value ; 

-(int)  starStyle ;
-(void) setStarStyle: (int) value ;

// Texture Settings

-(int)  resolution;
-(void) setResolution: (int) value ;

// Full screen

-(int)  fullScreenMode;
-(void) setFullScreenMode: (int) value ;

// GUI Methods
- (void) actionForItem: (id) item;
- (BOOL) validateItem: (id) item;
- (void) validateItems;
- (void) validateItemForTag: (int) tag;
- (int)  tagForKey: (int) key;
- (void) scanForKeys: (id) item;

@end
