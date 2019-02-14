//
//  EclipseFinderController.mm
//  celestia
//
//  Created by Da Woon Jung on 2007-05-07.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import "EclipseFinderController.h"
#import "CelestiaAppCore.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaBody_PrivateAPI.h"
#import "eclipsefinder.h"
#import <celmath/geomutil.h>

@interface EclipseFinderController(Private)
- (void)getEclipses: (id)aObject;
- (void)getEclipsesDone: (id)aObject;
@end


@implementation EclipseFinderController

static NSMutableArray *eclipses;
static NSCalendarDate *startDate;
static NSCalendarDate *midDate;
static NSCalendarDate *endDate;
static string receiverName;
static Eclipse::Type eclipseType;
static CelestiaBody *eclipseBody;

- (id)init
{
    self = [super initWithWindowNibName: @"EclipseFinder"];
    if (self)
    {
        eclipses = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)windowDidLoad
{
    [eclipseList setDoubleAction: @selector(go:)];
}

- (void)windowWillClose:(NSNotification *)aNotification
{
    if ([self window] == [aNotification object])
    {
        keepGoing = NO;
    }
}

- (void)dealloc
{
    [eclipses release];
    [super dealloc];
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
    NSDictionary *userInfo = [aNotification userInfo];
    if (userInfo && NSReturnTextMovement == [[userInfo objectForKey: @"NSTextMovement"] intValue])
    {
        [self find: self];
    }
}

- (IBAction)find: (id)sender
{
    CelestiaSimulation *sim;
    CelestiaSelection *sel;
    Body *body;
    id startDateObj       = [eclipseFindStart objectValue];
    id endDateObj         = [eclipseFindEnd   objectValue];
    NSString *receiverStr = [eclipseReceiver  stringValue];
    keepGoing = YES;

    if (0 == [receiverStr length])
    {
        NSRunAlertPanel(NSLocalizedString(@"No Object Name Entered",@""),
                        NSLocalizedString(@"Please enter an object name.",@""),
                        nil,nil,nil);
        return;
    }

    if (nil == startDateObj || nil == endDateObj)
    {
        NSRunAlertPanel(NSLocalizedString(@"No Starting or Ending Date Entered",@""),
                        NSLocalizedString(@"Please enter starting and ending dates.",@""),
                        nil,nil,nil);
        return;
    }

    sim = [[CelestiaAppCore sharedAppCore] simulation];
    sel = [sim findObjectFromPath: receiverStr];
    if (nil == sel || [sel isEmpty])
    {
        NSRunAlertPanel(NSLocalizedString(@"Object Not Found",@""),
                        NSLocalizedString(@"Please check that the object name is correct.",@""),
                        nil,nil,nil);
        return;
    }

    eclipseBody = [[sel body] retain];
    body = [eclipseBody body];
    if (nil == body || nil == body->getSystem())
    {
        NSRunAlertPanel(NSLocalizedString(@"Object Not Found",@""),
                        NSLocalizedString(@"Please check that the object name is correct.",@""),
                        nil,nil,nil);
        return;
    }

    if (body->getSystem()->getPrimaryBody())
    {
        // Eclipse receiver is a moon -> find lunar eclipses
        eclipseType = Eclipse::Moon;
        PlanetarySystem *system = body->getSystem();
        if (system)
        {
            Body *parent = system->getPrimaryBody();
            if (NULL == parent)
            {
                [eclipseBody release];
                return;
            }
            receiverName = parent->getName();
        }
    }
    else
    {
        eclipseType = Eclipse::Solar;
        receiverName = body->getName();
        // EclipseFinder class expects unlocalized names
    }
    

    startDate = (NSCalendarDate *)[startDateObj retain];
    endDate = (NSCalendarDate *)[endDateObj retain];
    midDate = [[startDate dateByAddingYears: 0
                                     months: 0
                                       days: 15
                                      hours: 0
                                    minutes: 0
                                    seconds: 0] retain];
    // Find eclipses in small timeslices, to give the user
    // a chance to abort with the stop button

    [findButton setEnabled: NO];
    [eclipses release];
    eclipses = [[NSMutableArray alloc] init];
    [eclipseProgress startAnimation: self];

    [NSThread detachNewThreadSelector: @selector(getEclipses:)
                             toTarget: self withObject: nil];
}

/* Go code borrowed from Windows version */
- (IBAction)go: (id)sender
{
    int rowIndex = [eclipseList selectedRow];
    if (rowIndex < 0) return;

    CelestiaCore *appCore = (CelestiaCore*) [[CelestiaAppCore sharedAppCore] appCore];
    NSDictionary *eclipse = [eclipses objectAtIndex: rowIndex];
    Body *body = [(CelestiaBody *)[eclipse objectForKey: @"body"] body];
    double startTime = [[eclipse objectForKey: @"start"] doubleValue];
    Simulation *sim = appCore->getSimulation();
    sim->setTime(startTime);
    Selection target(body);
    Selection ref(body->getSystem()->getStar());
    sim->setFrame(ObserverFrame::PhaseLock, target, ref);
    sim->update(0.0);

    double distance = target.radius() * 4.0;
    sim->setSelection(target);
    sim->gotoLocation(UniversalCoord::Zero().offsetKm(Eigen::Vector3d(distance, 0, 0)),
                      YRotation(-0.5*PI)*XRotation(-0.5*PI),
                      5.0);
}

- (IBAction)stopFind: (id)sender
{
    keepGoing = NO;
}


- (void)getEclipses: (id)aObject
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    CelestiaCore *appCore = (CelestiaCore*) [[CelestiaAppCore sharedAppCore] appCore];
    EclipseFinder *eclipseFinder;
    const Eclipse *eclipse;
    double jd;
    double hours, minutes;
    NSString *eclipseName;
    NSString *duration;
    NSCalendarDate *eclipseDate;
    vector<Eclipse>::const_iterator iter;

    while (keepGoing)
    {
        if (NSOrderedDescending == [midDate compare: endDate])
        {
            if (NSOrderedAscending == [startDate compare: endDate])
            {
                [midDate release];
                midDate = [endDate retain];
            }
            else
            {
                break;
            }
        }

        eclipseFinder =
            new EclipseFinder(appCore,
                              receiverName,
                              eclipseType,
                              (double)astro::Date(astro::secondsToJulianDate([startDate timeIntervalSince1970])) + (double)astro::Date(1970,1,1),
                              (double)astro::Date(astro::secondsToJulianDate([midDate timeIntervalSince1970])) + (double)astro::Date(1970,1,1)
                              );

        const vector<Eclipse> &freshEclipses = eclipseFinder->getEclipses();

        for (iter  = freshEclipses.begin();
             iter != freshEclipses.end() && keepGoing;
             ++iter)
        {
            eclipse = &(*iter);
            jd = astro::julianDateToSeconds((double)(*eclipse->date));
            if (0 == jd && eclipse->planete == "None")
                continue;
            if (eclipse->body != [(CelestiaBody *)eclipseBody body])
                continue;

            eclipseName = [NSString stringWithStdString: (eclipseType == Eclipse::Moon ? eclipse->planete : eclipse->sattelite)];
            eclipseDate = [NSCalendarDate dateWithTimeIntervalSince1970: (jd - astro::julianDateToSeconds((double)astro::Date(1970,1,1)))];
            hours = astro::julianDateToSeconds(eclipse->endTime - eclipse->startTime)/3600.0;
            minutes = 60.0*(hours - (int)hours);
            duration = [NSString stringWithFormat: @"%02d:%02d", (int)hours, (int)minutes];

            [eclipses addObject:
                [NSDictionary dictionaryWithObjectsAndKeys:
                    NSLocalizedStringFromTable(eclipseName,@"po",@""), @"caster",
                    eclipseDate, @"date",
                    [[[CelestiaBody alloc] initWithBody: eclipse->body] autorelease], @"body",
                    [NSNumber numberWithDouble: eclipse->startTime], @"start",
                    duration, @"duration",                
                    nil]
            ];
        }

        delete eclipseFinder;

        [startDate release];
        startDate = [midDate retain];
        [midDate release];
        midDate = [[startDate dateByAddingYears: 0
                                         months: 0
                                           days: 15
                                          hours: 0
                                        minutes: 0
                                        seconds: 0] retain];
    }

    [self performSelectorOnMainThread: @selector(getEclipsesDone:)
                           withObject: nil
                        waitUntilDone: NO];
    [pool release];
}

- (void)getEclipsesDone: (id)aObject
{
    [startDate release];   startDate = nil;
    [midDate release];     midDate = nil;
    [endDate release];     endDate = nil;
    [eclipseBody release]; eclipseBody = nil;
    [eclipseProgress stopAnimation: self];
    [findButton setEnabled: YES];
    [eclipseList reloadData];    
}


- (int)numberOfRowsInTableView: (NSTableView *)aTableView
{
    return [eclipses count];
}

- (id)tableView: (NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
    id result = nil;
    NSDictionary *eclipse = nil;

    if ([eclipses count] > 0)
    {
        eclipse = [eclipses objectAtIndex: rowIndex];
        if ([[aTableColumn identifier] isEqualToString: @"begin"])
        {
            result = [NSCalendarDate dateWithTimeIntervalSince1970: astro::julianDateToSeconds([[eclipse objectForKey: @"start"] doubleValue] - (double)astro::Date(1970,1,1))];
        }
        else
        {
            result = [eclipse objectForKey: [aTableColumn identifier]];
        }
    }

    return result;
}
@end
