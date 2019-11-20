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
#include <celutil/util.h>

@implementation NSMenu (CelestiaMenu)
- (NSInteger) indexOfItemWithLocalizableTitle: (NSString *) aTitle
{
    NSInteger index = [self indexOfItemWithTitle: NSLocalizedString(aTitle,@"")];
    if (index < 0)
        index = [self indexOfItemWithTitle: aTitle];
    return index;
}

- (void) removeRefMarkItems
{
    NSInteger index;
    index = [self indexOfItemWithTitle:[NSString stringWithUTF8String:_("Reference Vectors")]];
    if (index >= 0) [self removeItemAtIndex: index];
}

- (void) removePlanetarySystemItem
{
    NSInteger satMenuIndex = [self indexOfItemWithTitle:
       [NSString stringWithUTF8String:_("Satellites")]];
    if (satMenuIndex < 0)
        satMenuIndex = [self indexOfItemWithTitle:
           [NSString stringWithUTF8String:_("Orbiting Bodies")]];
    if (satMenuIndex < 0)
        satMenuIndex = [self indexOfItemWithTitle:[NSString stringWithUTF8String:_("Planets")]];
    if (satMenuIndex >= 0)
        [self removeItemAtIndex: satMenuIndex];
}

- (void) removeAltSurfaceItem
{
    NSInteger surfMenuIndex = [self indexOfItemWithLocalizableTitle:
        @"Show Alternate Surface" ];
    if (surfMenuIndex >= 0)
        [self removeItemAtIndex: surfMenuIndex];
}

- (BOOL) insertRefMarkItemsForSelection: (CelestiaSelection *) aSelection
                                atIndex: (NSInteger) aIndex
{
    BOOL result    = NO;
    NSMenuItem *mi = nil;
    id target      = nil;
    CelestiaSettings *settings = [CelestiaSettings shared];

    if ([aSelection body])
    {
        target = [aSelection body];
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Reference Vectors")] action: nil keyEquivalent: @""];
        NSMenu *refMarksMenu = [[NSMenu alloc] initWithTitle: @"Reference Vectors" ];
        [mi setSubmenu: refMarksMenu];
        if (mi)
        {
            [self insertItem: mi atIndex: aIndex];
        }                
        
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Show Body Axes")] action: nil keyEquivalent: @""];
        if (mi)
        {
            [mi setTag: 1000];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Show Frame Axes")] action: nil keyEquivalent: @""];
        if (mi)
        {
            [mi setTag: 1001];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Show Sun Direction")] action: nil keyEquivalent: @""];
        if (mi)
        {
            [mi setTag: 1002];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Show Velocity Vector")] action: nil keyEquivalent: @""];
        if (mi)
        {
            [mi setTag: 1003];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Show Planetographic Grid")] action: nil keyEquivalent: @""];
        if (mi)
        {
            [mi setTag: 1004];
            [refMarksMenu addItem: mi];
            [settings scanForKeys: mi];
        }        
        mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Show Terminator")] action: nil keyEquivalent: @""];
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
                                       atIndex: (NSInteger) aIndex
{
    BOOL result = NO;
    NSMenuItem *mi = nil;
    id browseItem;
    if ([aSelection body])
    {
        browseItem = [[BrowserItem alloc] initWithCelestiaBody: [aSelection body]];
        [BrowserItem addChildrenToBody: browseItem];
        NSArray *children = [browseItem allChildNames];
        if (children && [children count] > 0)
        {
            mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_("Satellites")] action: nil keyEquivalent: @""];
            [mi addPlanetarySystemMenuForItem: browseItem
                                       target: aTarget
                                       action: @selector(selectSatellite:)];
        }
    }
    else if ([aSelection star])
    {
        browseItem = [[BrowserItem alloc] initWithCelestiaStar: [aSelection star]];
        [BrowserItem addChildrenToStar: browseItem];
        NSArray *children = [browseItem allChildNames];
        if (children && [children count] > 0)
        {
            NSString *satMenuItemName = [[browseItem name] isEqualToString: @"Sol"] ?
            @"Orbiting Bodies" : @"Planets";
            mi = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_([satMenuItemName UTF8String])] action: nil keyEquivalent: @""];
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
                                  atIndex: (NSInteger) aIndex
{
    BOOL result = NO;
    NSMenuItem *mi = nil;
    if ([aSelection body])
    {
        NSArray *surfaces = [[aSelection body] alternateSurfaceNames];
        if (surfaces && [surfaces count] > 0)
        {
            mi = [[NSMenuItem alloc] initWithTitle:
  NSLocalizedString(@"Show Alternate Surface",@"") action: nil
                                      keyEquivalent: @""];
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
    NSArray *childChildren;
    unsigned childCount = 0;
    BOOL loneChild = NO;
    NSString *locationsName =[NSString stringWithUTF8String:_("Locations")];
    NSArray *children = [browseItem allChildNames];
    NSEnumerator *childEnum = [children objectEnumerator];

    while ((childName = [childEnum nextObject]))
        if (![childName isEqualToString: locationsName]) ++childCount;
    loneChild = (childCount == 1);

    satMenu = [[NSMenu alloc] initWithTitle: @"Satellites" ];
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
                satMenuItem = [[NSMenuItem alloc] initWithTitle: childName action: nil keyEquivalent: @""];
                [satMenuItem setRepresentedObject: [child body] ];
                [satMenuItem setTarget: target];
                [satMenu addItem: satMenuItem];
            }

            if (childChildren && [childChildren count] > 0)
            {
                NSMenu *subMenu = [[NSMenu alloc] initWithTitle: @"children" ];
                NSMenuItem *subMenuItem;
                id subChildName;
                id subChild;
                NSEnumerator *subEnum = [childChildren objectEnumerator];
                while ((subChildName = [subEnum nextObject]))
                {
                    subChild = [child childNamed: subChildName];
                    if (subChild)
                    {
                        subMenuItem = [[NSMenuItem alloc] initWithTitle: subChildName action: action keyEquivalent: @""];
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
        newItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:_([[surfaces objectAtIndex: i] UTF8String])] action: action keyEquivalent: @""];
        [newItem setTag:     601+i ];
        [newItem setTarget:  target ];
        [ surfaceMenu addItem: newItem ];
    }
    [ self setSubmenu: surfaceMenu ];
    [ surfaceMenu update ];
}
@end
