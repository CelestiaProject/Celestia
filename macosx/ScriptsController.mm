//
//  ScriptsController.mm
//  celestia
//
//  Created by Da Woon Jung on 2007-05-30.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import "ScriptsController.h"
#import "CelestiaAppCore.h"
#import "NSString_ObjCPlusPlus.h"
#import "celestiacore.h"
#import "scriptmenu.h"

#define CEL_SCRIPTS_FOLDER  @"scripts"


@interface ScriptsController(Private)
- (void)runScript: (id)aSender;
- (void)addItems: (NSDictionary *)aItems
          toMenu: (NSMenu *)aMenu;
@end

@implementation ScriptsController

- (void)buildScriptMenu
{
    std::vector<ScriptMenuItem> *scripts = ScanScriptsDirectory([CEL_SCRIPTS_FOLDER stdString], true);
    int count = [scriptMenu numberOfItems];
    while (count-- > 0)
        [scriptMenu removeItemAtIndex: 0];

    NSMutableDictionary *itemDict = [NSMutableDictionary dictionary];
    unsigned baseDirLevel = [[CEL_SCRIPTS_FOLDER pathComponents] count];
    NSString *title;
    id path;
    size_t i;

    for (i = 0; i < scripts->size(); ++i)
    {
        title = [NSString stringWithStdString: (*scripts)[i].title];
        path  = [NSString stringWithStdString: (*scripts)[i].filename];
        // Build submenus for nested directories
        NSArray *subPaths = [[path stringByDeletingLastPathComponent] pathComponents];
        if (subPaths && [subPaths count] > baseDirLevel)
        {
            id parentDict = itemDict;
            id childDict = nil;
            NSString *subDir;
            for (unsigned j = baseDirLevel; j < [subPaths count]; ++j)
            {
                subDir = [subPaths objectAtIndex: j];
                childDict = [parentDict objectForKey: subDir];
                if (nil == childDict)
                    childDict = [NSMutableDictionary dictionary];
                [parentDict setObject: childDict forKey: subDir];
                parentDict = childDict;
            }
            if (childDict) [childDict setObject: path forKey: title];
            continue;
        }
        [itemDict setObject: path forKey: title];
    }

    [self addItems: itemDict toMenu: scriptMenu];

    delete scripts;
}

- (void)addItems: (NSDictionary *)aItems
          toMenu: (NSMenu *)aMenu
{
    NSMenuItem *mi;
    id item;
    NSString *title;
    NSArray *titles = [[aItems allKeys] sortedArrayUsingSelector: @selector(localizedCaseInsensitiveCompare:)];

    for (unsigned i = 0; i < [titles count]; ++i)
    {
        title = [titles objectAtIndex: i];
        mi = [aMenu addItemWithTitle: title
                              action: @selector(runScript:)
                       keyEquivalent: @""];
        if (mi)
        {
            item = [aItems objectForKey: title];
            if ([item respondsToSelector: @selector(objectForKey:)])
            {
                [mi setAction: nil];
                NSMenu *subMenu = [[[NSMenu alloc] initWithTitle: title] autorelease];
                [self addItems: item toMenu: subMenu];
                [mi setSubmenu: subMenu];
            }
            else
            {
                [mi setTarget: self];
                [mi setRepresentedObject: item];
            }
        }
    }
}

- (void)runScript: (id)aSender
{
    if (aSender && [aSender respondsToSelector: @selector(representedObject)])
    {
        CelestiaCore *appCore = (CelestiaCore*) [[CelestiaAppCore sharedAppCore] appCore];
        id filename = [aSender representedObject];
        if (filename)
        {
            appCore->runScript([(NSString *)filename stdString]);
        }
    }
}
@end
