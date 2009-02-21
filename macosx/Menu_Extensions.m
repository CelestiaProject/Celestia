//
//  Menu_Extensions.m
//  celestia
//
//  Created by Da Woon Jung on 12/9/07.
//  Copyright (C) 2007, Celestia Development Team
//

#import "Menu_Extensions.h"
#import "CelestiaSelection.h"
#import "BrowserItem.h"
#import "CelestiaSettings.h"


@implementation NSMenu (CelestiaMenu)
- (int) indexOfItemWithLocalizableTitle: (NSString *) aTitle
{
    int index = [self indexOfItemWithTitle: NSLocalizedString(aTitle,@"")];
    if (index < 0)
        index = [self indexOfItemWithTitle: aTitle];
    return index;
}

- (void) removeRefMarkItems
{
    int index;
    index = [self indexOfItemWithTitle: NSLocalizedStringFromTable(@"Reference Vectors",@"po",@"")];
    if (index >= 0) [self removeItemAtIndex: index];
}

- (void) removePlanetarySystemItem
{
    int satMenuIndex = [self indexOfItemWithTitle:
        NSLocalizedStringFromTable(@"Satellites",@"po",@"")];
    if (satMenuIndex < 0)
        satMenuIndex = [self indexOfItemWithTitle:
            NSLocalizedStringFromTable(@"Orbiting Bodies",@"po",@"")];
    if (satMenuIndex < 0)
        satMenuIndex = [self indexOfItemWithTitle: NSLocalizedStringFromTable(@"Planets",@"po",@"")];
    if (satMenuIndex >= 0)
        [self removeItemAtIndex: satMenuIndex];
}

- (void) removeAltSurfaceItem
{
    int surfMenuIndex = [self indexOfItemWithLocalizableTitle:
        @"Show Alternate Surface" ];
    if (surfMenuIndex >= 0)
        [self removeItemAtIndex: surfMenuIndex];
}

- (BOOL) insertRefMarkItemsForSelection: (CelestiaSelection *) aSelection
                                atIndex: (int) aIndex
{
    BOOL result    = NO;
    NSMenuItem *mi = nil;
    id target      = nil;
    CelestiaSettings *settings = [CelestiaSettings shared];

    if ([aSelection body])
    {
        target = [aSelection body];
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Reference Vectors",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        NSMenu *refMarksMenu = [[[NSMenu alloc ] initWithTitle: @"Reference Vectors" ] autorelease];
        [mi setSubmenu: refMarksMenu];
        if (mi)
        {
            [self insertItem: mi atIndex: aIndex];
        }                
        
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Show Body Axes",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        if (mi)
        {
            [mi setTag: 1000];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Show Frame Axes",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        if (mi)
        {
            [mi setTag: 1001];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Show Sun Direction",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        if (mi)
        {
            [mi setTag: 1002];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Show Velocity Vector",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        if (mi)
        {
            [mi setTag: 1003];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Show Planetographic Grid",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        if (mi)
        {
            [mi setTag: 1004];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }        
        mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Show Terminator",@"po",@"") action: nil keyEquivalent: @""] autorelease];
        if (mi)
        {
            [mi setTag: 1005];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }                
        result = YES;
    }
    return result;
}

- (BOOL) insertPlanetarySystemItemForSelection: (CelestiaSelection *) aSelection
                                        target: (id) aTarget
                                       atIndex: (int) aIndex
{
    BOOL result = NO;
    NSMenuItem *mi = nil;
    id browseItem;
    if ([aSelection body])
    {
        browseItem = [[[BrowserItem alloc] initWithCelestiaBody: [aSelection body]] autorelease];
        [BrowserItem addChildrenToBody: browseItem];
        NSArray *children = [browseItem allChildNames];
        if (children && [children count] > 0)
        {
            mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(@"Satellites",@"po",@"") action: nil keyEquivalent: @""] autorelease];
            [mi addPlanetarySystemMenuForItem: browseItem
                                       target: aTarget
                                       action: @selector(selectSatellite:)];
        }
    }
    else if ([aSelection star])
    {
        browseItem = [[[BrowserItem alloc] initWithCelestiaStar: [aSelection star]] autorelease];
        [BrowserItem addChildrenToStar: browseItem];
        NSArray *children = [browseItem allChildNames];
        if (children && [children count] > 0)
        {
            NSString *satMenuItemName = [[browseItem name] isEqualToString: @"Sol"] ?
            @"Orbiting Bodies" : @"Planets";
            mi = [[[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable(satMenuItemName,@"po",@"") action: nil keyEquivalent: @""] autorelease];
            [mi addPlanetarySystemMenuForItem: browseItem
                                       target: aTarget
                                       action: @selector(selectSatellite:)];
        }
    }
    if (mi && [[mi submenu] numberOfItems] > 0)
    {
        [self insertItem: mi atIndex: aIndex];
        result = YES;
    }
    return result;
}

- (BOOL) insertAltSurfaceItemForSelection: (CelestiaSelection *) aSelection
                                   target: (id) aTarget
                                  atIndex: (int) aIndex
{
    BOOL result = NO;
    NSMenuItem *mi = nil;
    if ([aSelection body])
    {
        NSArray *surfaces = [[aSelection body] alternateSurfaceNames];
        if (surfaces && [surfaces count] > 0)
        {
            mi = [[[NSMenuItem alloc] initWithTitle:
  NSLocalizedString(@"Show Alternate Surface",@"") action: nil
                                      keyEquivalent: @""] autorelease];
            [mi addAltSurfaceMenuWithNames: surfaces
                                    target: aTarget
                                    action: @selector(activateMenuItem:)];
            if (mi && [[mi submenu] numberOfItems] > 0)
            {
                [self insertItem: mi atIndex: aIndex];
                result = YES;
            }
        }
    }
    return result;
}
@end


@implementation NSMenuItem (CelestiaMenu)
- (void) addPlanetarySystemMenuForItem: (BrowserItem *) browseItem
                                target: (id)  target
                                action: (SEL) action
{
    NSMenu *satMenu;
    id childName;
    id child;
    id childChildren;
    unsigned childCount = 0;
    BOOL loneChild = NO;
    NSString *locationsName = NSLocalizedStringFromTable(@"Locations",@"po",@"");
    NSArray *children = [browseItem allChildNames];
    NSEnumerator *childEnum = [children objectEnumerator];

    while ((childName = [childEnum nextObject]))
        if (![childName isEqualToString: locationsName]) ++childCount;
    loneChild = (childCount == 1);

    satMenu = [[[NSMenu alloc ] initWithTitle: @"Satellites" ] autorelease];
    [self setSubmenu: satMenu];

    childEnum = [children objectEnumerator];
    while ((childName = [childEnum nextObject]))
    {
        if ([childName isEqualToString: locationsName])
            continue;
        NSMenuItem *satMenuItem;
        child = [browseItem childNamed: childName];
        if (child)
        {
            childChildren = [child allChildNames];
            // Don't create a submenu for a single item
            if (loneChild)
            {
                satMenuItem = self;
            }
            else
            {
                satMenuItem = [[[NSMenuItem alloc] initWithTitle: childName action: nil keyEquivalent: @""] autorelease];
                [satMenuItem setRepresentedObject: [child body] ];
                [satMenuItem setTarget: target];
                [satMenu addItem: satMenuItem];
            }

            if (childChildren && [childChildren count] > 0)
            {
                NSMenu *subMenu = [[[NSMenu alloc ] initWithTitle: @"children" ] autorelease];
                NSMenuItem *subMenuItem;
                id subChildName;
                id subChild;
                NSEnumerator *subEnum = [childChildren objectEnumerator];
                while ((subChildName = [subEnum nextObject]))
                {
                    subChild = [child childNamed: subChildName];
                    if (subChild)
                    {
                        subMenuItem = [[[NSMenuItem alloc] initWithTitle: subChildName action: action keyEquivalent: @""] autorelease];
                        [subMenuItem setRepresentedObject: [subChild body] ];
                        [subMenuItem setTarget: target];
                        [subMenu addItem: subMenuItem];
                    }
                }
                [satMenuItem setSubmenu: subMenu];
            }
            else
            {
                [satMenuItem setAction: action];
            }
        }
    }
}

- (void) addAltSurfaceMenuWithNames: (NSArray *) surfaces
                             target: (id)  target
                             action: (SEL) action
{
    NSMenu *surfaceMenu = [[NSMenu alloc ] initWithTitle: @"altsurf" ];
    [ self setEnabled: YES ];
    NSMenuItem *newItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedString(@"default",@"") action: action keyEquivalent: @""];
    [newItem setTag:    600 ];
    [newItem setTarget: target ];
    [ surfaceMenu addItem: newItem ];
    unsigned i;
    for (i = 0; i < [surfaces count]; ++i)
    {
        newItem = [[NSMenuItem alloc] initWithTitle: NSLocalizedStringFromTable([surfaces objectAtIndex: i],@"po",@"") action: action keyEquivalent: @""];
        [newItem setTag:     601+i ];
        [newItem setTarget:  target ];
        [ surfaceMenu addItem: newItem ];
    }
    [ self setSubmenu: surfaceMenu ];
    [ surfaceMenu update ];
}
@end
