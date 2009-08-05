//
//  GotoWindowController.m
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//
#import "GotoWindowController.h"
#import "CelestiaAppCore.h"
#import "CelestiaSimulation.h"
#import "CelestiaBody.h"
#import "Astro.h"
#import "CelestiaVector.h"
#include <math.h>

#define UNITS_KM 0
#define UNITS_RADII 1
#define UNITS_AU 2

@implementation GotoWindowController

- (IBAction)showWindow:(id)sender
{
    CelestiaSimulation *sim;
    CelestiaSelection *csel;
    CelestiaBody *body;
    NSArray *distLongLat;
    double distance;
    //NSLog(@"-[GotoWindowController showWindow:%@]",sender);
    sim = [[CelestiaAppCore sharedAppCore] simulation];
    distLongLat = [sim getSelectionLongLat];
    //NSLog(@"distLongLat = %@",distLongLat);
    csel = [sim selection];
    body = [csel body];
    if (body != nil) {
        //NSLog(@"Body is NOT nil");
        distance = [[distLongLat objectAtIndex:0] doubleValue] - [[body radius] doubleValue];
        [distanceField setDoubleValue:distance];
        [longitudeField setDoubleValue:[[distLongLat objectAtIndex:1] doubleValue]];
        [latitudeField setDoubleValue:[[distLongLat objectAtIndex:2] doubleValue]];
        [objectField setStringValue:[body name]];
        [unitsButton selectItemAtIndex:UNITS_KM];
    }
    [super showWindow:sender];
}

- (IBAction)gotoObject:(id)sender
{
    CelestiaSimulation *sim;
    CelestiaSelection *csel;
    CelestiaVector *up;
    double distance;
    csel = nil;
    //NSLog(@"-[GotoWindowController gotoObject:%@]",sender);
    sim = [[CelestiaAppCore sharedAppCore] simulation];
    //NSLog(@"[objectField stringValue] = '%@'",[objectField stringValue]);
    if (![[objectField stringValue] length])
    {
	NSRunAlertPanel(NSLocalizedString(@"No Object Name Entered",@""),
                    NSLocalizedString(@"Please enter an object name.",@""),
                    nil,nil,nil);
        return;
    }
    csel = [sim findObjectFromPath:[objectField stringValue]];
    if ((csel == nil) || [csel isEmpty])
    {
       NSRunAlertPanel(NSLocalizedString(@"Object Not Found",@""),
                       NSLocalizedString(@"Please check that the object name is correct.",@""),
                       nil,nil,nil);
        return;
    }
    [sim setSelection:csel];
    [sim geosynchronousFollow];
    distance = [[csel radius] doubleValue]*5.0;
    if ([[distanceField stringValue] length])
    {
        //NSLog(@"distanceField is filled in");
        // convert to km if necessary 
        switch ([unitsButton indexOfSelectedItem]) {
            case UNITS_RADII:
                //NSLog(@"Converting from radii");
                distance = [distanceField doubleValue]*[[csel radius] doubleValue];
                break;
            case UNITS_AU:
                //NSLog(@"Converting from AU");
                distance = [[Astro AUtoKilometers:[NSNumber numberWithDouble:[distanceField doubleValue]]] doubleValue];
                break;
            case UNITS_KM:
                //NSLog(@"don't have to convert from km");
                distance = [distanceField doubleValue];
                break;
            default:
                //NSLog(@"I don't know what button has been selected?");
                break;
        }
    }

    up = [CelestiaVector vectorWithx:[NSNumber numberWithFloat:0.0] y:[NSNumber numberWithFloat:1.0] z:[NSNumber numberWithFloat:0.0]];
    if ([[latitudeField stringValue] length] && [[longitudeField stringValue] length])
    {
        //NSLog(@"lat/lon provided");
        [sim 
            gotoSelection:[NSNumber numberWithFloat:5.0]
            distance:[NSNumber numberWithDouble:distance]
            longitude:[NSNumber numberWithDouble:[longitudeField doubleValue]*(M_PI/180.0)]
            latitude:[NSNumber numberWithDouble:[latitudeField doubleValue]*(M_PI/180.0)]
            up:up
        ];
    } else {
        //NSLog(@"No lat/lon");
        [sim
            gotoSelection:[NSNumber numberWithFloat:5.0]
            distance:[NSNumber numberWithDouble:distance]
            up:up
            coordinateSystem:@"ObserverLocal"
        ];
    }
}

@end
