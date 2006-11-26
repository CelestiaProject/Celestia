//
//  BrowserWindowController.mm
//  celestia
//
//  Created by Hank Ramsey on Tue Dec 28 2004.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//  Modifications Copyright (c) 2004 Hank Ramsey. All rights reserved.
//

#import "BrowserWindowController.h"
#import "NSString_ObjCPlusPlus.h"
#import "CelestiaAppCore.h"

#include "celestiacore.h"
#include "celestia.h"
#include "selection.h"
#include "starbrowser.h"

#define BROWSER_MAX_DSO_COUNT   500
#define BROWSER_MAX_STAR_COUNT  100

@implementation BrowserWindowController

static CelestiaCore *appCore;

- (id) init
{
    self = [super initWithWindowNibName: @"BrowserWindow" ];
    appCore = (CelestiaCore*) [[CelestiaAppCore sharedAppCore] appCore];
    return self;
}

- (void) windowDidLoad
{
    if ([self window]==nil) NSLog(@"loaded browser window is nil");
}

//--------------------------------------------------------------

- (NSDictionary*) deepSkyDict
{
    int objCount;
    NSMutableDictionary* newDict;
    int i = 0;
    DSODatabase* catalog = appCore->getSimulation()->getUniverse()->getDSOCatalog();
    DeepSkyObject *obj;
    NSString *name;

    objCount = catalog->size();
    if (objCount > BROWSER_MAX_DSO_COUNT)
        objCount = BROWSER_MAX_DSO_COUNT;
    newDict = [NSMutableDictionary dictionaryWithCapacity: objCount];

    for (; i < objCount; ++i)
    {
        obj = catalog->getDSO(i);
        if (obj)
        {
            name = [NSString stringWithStdString:
                catalog->getDSOName(obj)];
            [newDict setObject: name
                        forKey: [NSString stringWithFormat: @"%@ (%s)", name, obj->getType()]];
        }
    }

    [newDict setObject: [[newDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
    [newDict setObject: @"" forKey: @"_path" ];
    return newDict;       
}

- (NSDictionary*) starDict: (int) kind
{
    std::vector<const Star*>* nearStars;
    int starCount = 0;
    StarBrowser* sb = new StarBrowser(appCore->getSimulation(),kind);
    nearStars = sb->listStars( BROWSER_MAX_STAR_COUNT );
    starCount = nearStars->size();            
//    if (nearStars == nil ) return 1;
    NSMutableDictionary* starDict = [NSMutableDictionary dictionaryWithCapacity: starCount+2];
    NSMutableArray* starKeys = [NSMutableArray arrayWithCapacity: starCount+2];
    const Star *aStar;
    int i;
    for (i=0;i<starCount;i++)
    {
        aStar = (*nearStars)[i];
        NSString* starName = [NSString  stringWithStdString: appCore->getSimulation()->getUniverse()->getStarCatalog()->getStarName(*aStar) ];
        [starDict setObject: starName forKey: starName];
        [starKeys addObject: starName];
    }
    [starDict setObject: starKeys forKey: @"_keys" ];
    [starDict setObject: @"" forKey: @"_path" ];
    delete sb;
    delete nearStars;
    return starDict;       
}

-(id) rootDict
{    // creates root dictionary for browser
    static NSDictionary* staticRootDict = NULL;

    if (staticRootDict!=NULL) return staticRootDict;
    int rootSize = 4;
    NSMutableDictionary* newDict;
    newDict = [NSMutableDictionary dictionaryWithCapacity: rootSize];
    
    [newDict setObject: @"Sol" forKey: NSLocalizedString(@"Home (Sol)",@"")];

    [newDict setObject: [self starDict: StarBrowser::NearestStars ] forKey: NSLocalizedString(@"Nearest Stars",@"")];
    [newDict setObject: [self starDict: StarBrowser::BrighterStars ] forKey: NSLocalizedString(@"Brightest Stars",@"")];
    [newDict setObject: [self starDict: StarBrowser::StarsWithPlanets ] forKey: NSLocalizedString(@"Stars With Planets",@"")];
    [newDict setObject: [self deepSkyDict ] forKey: NSLocalizedString(@"Deep Sky Objects",@"")];

    [newDict setObject: [[newDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
    [newDict setObject: @"" forKey: @"_path" ];
    staticRootDict = [newDict retain];
    return newDict;
}

-(id) newDictForPath: (NSString*) objPath
{   // creates browser dictionary for star or planet
    NSMutableDictionary* newDict = NULL;
    PlanetarySystem* sys = nil;
    Selection sel = appCore->getSimulation()->findObjectFromPath([objPath stdString], true);
    if ( sel.getType() == Selection::Type_Body )
    {
        sys = sel.body()->getSatellites();
    }
    else if ( sel.getType() == Selection::Type_Star )
    {
        sys = appCore->getSimulation()->getUniverse()->getSolarSystem(sel.star())->getPlanets();
    }
    
    if (sys)
    { 
    int sysSize = sys->getSystemSize();
    if (!newDict) newDict = [NSMutableDictionary dictionaryWithCapacity: sysSize+2];
    NSMutableDictionary* subDict;
    NSMutableDictionary* minorMoonDict = NULL;
    NSMutableDictionary* asteroidDict = NULL;
    NSMutableDictionary* cometDict = NULL;
    NSMutableDictionary* spacecraftDict = NULL;
    int i;
    for ( i=0; i<sysSize; i++)
    {
        Body* body = sys->getBody(i);
        NSString* bodName = [NSString stringWithStdString: body->getName(true)];
        NSString* bodPath = [[objPath stringByAppendingString: @"/"] stringByAppendingString: bodName];
        subDict = newDict;
        int bodyClass  = body->getClassification();
        if (bodyClass==Body::Asteroid && sel.getType() == Selection::Type_Body) bodyClass = Body::Moon;
        switch (bodyClass)
        {
            case Body::Planet:
                break;
            case Body::Moon:
                if (body->getRadius() < 100)
                {
                    if (!minorMoonDict)
                        minorMoonDict = [NSMutableDictionary dictionaryWithCapacity: 10];
                    subDict = minorMoonDict;
                }
                break;
            case Body::Asteroid:
                if (!asteroidDict)
                    asteroidDict = [NSMutableDictionary dictionaryWithCapacity: 10];
                subDict = asteroidDict;
                break;
            case Body::Comet:
                if (!cometDict)
                    cometDict = [NSMutableDictionary dictionaryWithCapacity: 10];
                subDict = cometDict;
                break;
            case Body::Spacecraft:
                if (!spacecraftDict)
                    spacecraftDict = [NSMutableDictionary dictionaryWithCapacity: 10];
                subDict = spacecraftDict;
                break;
        }
        [subDict setObject: bodPath forKey: bodName];
    }
    
    if (minorMoonDict)
    {
        if ([newDict count] == 0)
            newDict = minorMoonDict;
        else
        {
            [minorMoonDict setObject: [[minorMoonDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
            [minorMoonDict setObject: @"" forKey: @"_path" ];
            [newDict setObject: minorMoonDict forKey: NSLocalizedString(@"Minor Moons",@"")];
        }
    }
    if (asteroidDict)
    {
        [asteroidDict setObject: [[asteroidDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
        [asteroidDict setObject: @"" forKey: @"_path" ];
        [newDict setObject: asteroidDict forKey: NSLocalizedString(@"Asteroids",@"")];
    }
    if (cometDict)
    {
        [cometDict setObject: [[cometDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
        [cometDict setObject: @"" forKey: @"_path" ];
        [newDict setObject: cometDict forKey: NSLocalizedString(@"Comets",@"")];
    }
    if (spacecraftDict)
    {
        if ([newDict count] == 0)
            newDict = spacecraftDict;
        else
        {
            [spacecraftDict setObject: [[spacecraftDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
            [spacecraftDict setObject: @"" forKey: @"_path" ];
            [newDict setObject: spacecraftDict forKey: NSLocalizedString(@"Spacecrafts",@"")];
        }
    }
    }
    
    if ( sel.getType() == Selection::Type_Body )
    {
        std::vector<Location*>* locations = sel.body()->getLocations();
        if (locations != NULL)
        {
            NSMutableDictionary* locationDict = [NSMutableDictionary dictionaryWithCapacity: 100];
            if (!newDict) newDict = [NSMutableDictionary dictionaryWithCapacity: 1];
            for (vector<Location*>::const_iterator iter = locations->begin();
                iter != locations->end(); iter++)
            {
                NSString* locName = [NSString stringWithStdString: (*iter)->getName(true)];
                NSString* locPath = [[objPath stringByAppendingString: @"/"] stringByAppendingString: locName];
                [locationDict setObject: locPath forKey: locName];
            }
            if ([newDict count] == 0)
                newDict = locationDict;
            else
            {
                [locationDict setObject: [[locationDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
                [locationDict setObject: @"" forKey: @"_path" ];
                [newDict setObject: locationDict forKey: NSLocalizedString(@"Locations",@"")];
            }
        }
    }

    if (!newDict) return NULL;
    [newDict setObject: [[newDict allKeys]sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)] forKey: @"_keys" ];
    [newDict setObject: objPath forKey: @"_path" ];
    return newDict;
}

- (NSDictionary*) dictForPathArray: (NSArray*) pathNames
{	//returns dictionary for path array
    NSMutableDictionary* lastDict = [self rootDict];
    id nextDict = lastDict;
    NSString* lastKey = nil;
    unsigned i;
    for (i=1;i<[pathNames count];i++)
    {
	lastKey = [pathNames objectAtIndex: i];
        nextDict = [lastDict objectForKey: lastKey];
        if (!nextDict||![nextDict isKindOfClass: [NSDictionary class]]) break;
        lastDict = nextDict;
    }
    if ([nextDict isKindOfClass: [NSString class]])
    {
        NSDictionary* newDict = [self newDictForPath: (NSString*) nextDict ];
        if (newDict) [lastDict setObject: newDict forKey: lastKey ];
        nextDict = newDict;
    }
    if (![nextDict isKindOfClass: [NSDictionary class]]) nextDict = NULL;
    return nextDict;
}

- (id) infoForPathArray: (NSArray*) pathNames
{
    NSDictionary* nextDict = [self rootDict];
    unsigned i;
    for (i=1;i<[pathNames count];i++)
    {
        NSString* key = [pathNames objectAtIndex: i];
        nextDict = [nextDict objectForKey: key];
        if (!nextDict) break;
    }
    return nextDict;
}

- (Selection) selFromPathArray: (NSArray*) pathNames
{
    NSString* objPath = nil;    
    id obj = [self infoForPathArray: pathNames];

    if ([obj isKindOfClass: [NSDictionary class]])
    {
        objPath = (NSString*)[((NSDictionary*)obj) objectForKey: @"_path"];
    }
    else if ([obj isKindOfClass: [NSString class]])
    {
        objPath = (NSString*)obj;
    }
    
    Selection sel = appCore->getSimulation()->findObjectFromPath(objPath ? [objPath stdString] : std::string(), true);

//    if (sel == nil) NSLog([NSString stringWithFormat: @"obj %@ not found", objPath]);
    return sel;
}


- (int) browser: (NSBrowser*) sender numberOfRowsInColumn: (int) column
{
    if (browser==nil) 
    {
        browser = sender;        
        if ([browser respondsToSelector:@selector(setColumnResizingType:)])
            [browser setColumnResizingType: 2];
        [browser setMinColumnWidth: 80];
//        [browser setMaxVisibleColumns: 1];
        [browser setDoubleAction:@selector(doubleClick:)];
//        [browser setAction:@selector(click:)];
    }

    NSDictionary*  colDict = [self dictForPathArray: [[sender pathToColumn: column ] componentsSeparatedByString: [sender pathSeparator] ] ];
    NSArray* colKeys = [colDict objectForKey: @"_keys"];
    return [colKeys count];
}

- (BOOL) isLeaf: (id)  obj
{
    if ([obj isKindOfClass: [NSDictionary class]]) return NO;
    if ([obj isKindOfClass: [NSString class]])
    {
        Selection sel = appCore->getSimulation()->findObjectFromPath([obj stdString], true);
        if ( sel.getType() == Selection::Type_Body )
        {
            if ( sel.body()->getSatellites() || sel.body()->getLocations()) return NO;
        }
        else if ( sel.getType() == Selection::Type_Star )
        {
            if (appCore->getSimulation()->getUniverse()->getSolarSystem(sel.star())) return NO;
        }
    }
     return YES;
}

- (void) browser: (NSBrowser*) sender willDisplayCell: (id) cell atRow: (int) row column: (int) column
{
    NSDictionary* colDict = [self dictForPathArray: [[sender pathToColumn: column ] componentsSeparatedByString: [sender pathSeparator] ] ];
    NSArray* colKeys = [colDict objectForKey: @"_keys"];
    BOOL isLeaf = YES;
    NSString* itemName = [colKeys objectAtIndex: row];
    if (!itemName)
        itemName = @"????";
    else
        isLeaf = [self isLeaf: [colDict objectForKey: itemName ] ];

    NSRange rightParenRange = {NSNotFound, 0};
    NSRange leftParenRange  = {NSNotFound, 0};
    NSRange parenRange      = {NSNotFound, 0};

    if (isLeaf)
    {
        rightParenRange = [itemName rangeOfString:@")"
                                          options:NSBackwardsSearch];
        if (rightParenRange.length == 1)
        {
            leftParenRange = NSMakeRange(0, rightParenRange.location - 1);
            parenRange = [itemName rangeOfString:@"("
                                         options:NSBackwardsSearch
                                           range:leftParenRange];
            if (parenRange.length == 1)
            {
                parenRange.length = rightParenRange.location - parenRange.location + 1;
            }
        }
    }

    if (parenRange.location != NSNotFound)
    {
        NSDictionary *parenAttr = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSColor grayColor], NSForegroundColorAttributeName,
            nil];
        NSMutableAttributedString *attrStr = [[[NSMutableAttributedString alloc] initWithString: itemName] autorelease];
        [attrStr setAttributes:parenAttr range:parenRange];

        [cell setAttributedStringValue:attrStr];
    }
    else
    {
        [cell setTitle: itemName ];
    }

    [cell setLeaf: isLeaf];
}

- (IBAction) go: (id) sender
{
    Selection sel = [self selFromPathArray: [[browser path] componentsSeparatedByString: [browser pathSeparator] ]];
    appCore->getSimulation()->setSelection(sel);
    if ([sender tag]!=0) appCore->charEntered([sender tag]);
}

/*
static int firstVisibleColumn = 0;

-(void) adjustColumns: (id) sender
{
    // eliminate empty columns
    int lastColumn = [browser selectedColumn]+1;
    if ([[browser selectedCell] isLeaf]) lastColumn--;
    firstVisibleColumn = max(0,lastColumn + 1 - [browser numberOfVisibleColumns]);
    [browser setMaxVisibleColumns: lastColumn+1];
    [browser scrollColumnToVisible: firstVisibleColumn];
    [browser scrollColumnToVisible: lastColumn];
    firstVisibleColumn = [browser firstVisibleColumn];
}

- (IBAction) click: (id) sender
{
    if ([browser firstVisibleColumn]!=firstVisibleColumn &&
    ![[browser selectedCell] isLeaf] && ([browser selectedColumn]==([browser firstVisibleColumn]+[browser numberOfVisibleColumns]-2)))
    {
        // revert column autoscroll to allow double click
        [browser scrollColumnsRightBy: 1];
     }
     // delay column adjustment to allow double click
    [self performSelector: @selector(adjustColumns:) withObject: nil afterDelay: 0.3 ];
}
*/


- (IBAction) doubleClick: (id) sender
{
    Selection sel = [self selFromPathArray: [[browser path] componentsSeparatedByString: [browser pathSeparator] ]];
    appCore->getSimulation()->setSelection(sel);
    appCore->charEntered('g');
    // adjust columns immediately
//    [self adjustColumns: nil];
}


@end
