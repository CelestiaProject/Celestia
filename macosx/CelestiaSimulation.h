//
//  CelestiaSimulation.h
//  celestia
//
//  Created by Bob Ippolito on Fri Jun 07 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "CelestiaSelection.h"
#import "CelestiaUniverse.h"
#import "CelestiaUniversalCoord.h"
#import "CelestiaObserver.h"
#import "CelestiaVector.h"

@interface CelestiaSimulation : NSObject
{
    NSValue* _data;
}
-(NSNumber*)julianDate;
-(void)setDate:(NSNumber*)t;
-(NSNumber*)realTime;
-(NSNumber*)arrivalTime;
-(void)update:(NSNumber*)dt;
-(CelestiaSelection*)pickObject:(CelestiaVector*)pickRay tolerance:(NSNumber*)tolerance;
-(CelestiaSelection*)pickObject:(CelestiaVector*)pickRay;
-(CelestiaUniverse*)universe;
-(void)orbit:(CelestiaVector*)q;
-(void)rotate:(CelestiaVector*)q;
-(void)changeOrbitDistance:(NSNumber*)d;
-(void)setTargetSpeed:(NSNumber*)s;
-(NSNumber*)targetSpeed;
-(CelestiaSelection*)selection;
-(void)setSelection:(CelestiaSelection*)sel;
-(CelestiaSelection*)trackedObject;
-(void)setTrackedObject:(CelestiaSelection*)sel;
-(void)selectPlanet:(NSNumber*)planetNo;
-(CelestiaSelection*)findObject:(NSString*)s;
-(CelestiaSelection*)findObjectFromPath:(NSString*)s;
-(void)gotoSelection:(NSNumber*)gotoTime up:(CelestiaVector*)up coordinateSystem:(NSString*)csysName;
-(void)gotoSelection:(NSNumber*)gotoTime distance:(NSNumber*)distance up:(CelestiaVector*)up coordinateSystem:(NSString*)csysName;
-(void)gotoSelection:(NSNumber*)gotoTime distance:(NSNumber*)distance longitude:(NSNumber*)longitude latitude:(NSNumber*)latitude up:(CelestiaVector*)up;
-(NSArray*)getSelectionLongLat;
-(void)centerSelection;
-(void)centerSelection:(NSNumber*)centerTime;
-(void)follow;
-(void)geosynchronousFollow;
-(void)phaseLock;
-(void)chase;
-(void)cancelMotion;
-(CelestiaObserver*)observer;
-(void)setObserverPosition:(CelestiaUniversalCoord*)uc;
-(void)setObserverOrientation:(CelestiaVector*)q;
-(void)setObserverMode:(NSString*)m;
-(NSString*)observerMode;
-(void)setFrame:(NSString*)cs selection:(CelestiaSelection*)sel;
@end
