//
//  ScriptsController.mm
//  celestia
//
//  Created by Da Woon Jung on 2007-05-30.
//  Copyright 2007 Da Woon Jung. All rights reserved.
//

#import "ScriptsController.h"
#import "CelestiaController.h"
#import "NSString_ObjCPlusPlus.h"
#import "scriptmenu.h"


@interface ScriptsController(Private)
+ (NSDictionary *)scriptsForFolderRoot: (NSString *)root;
- (void)runScript: (id)aSender;
- (void)addItems: (NSDictionary *)aItems
          toMenu: (NSMenu *)aMenu;
@end

@implementation ScriptsController

+ (NSDictionary *)scriptsForFolderRoot: (NSString *)root
{
    NSString *scriptsFolder = nil;
    unsigned baseDirLevel = [[CEL_SCRIPTS_FOLDER pathComponents] count];
    BOOL isAbsoluteFolder = (root && [root length] > 0);
    if (isAbsoluteFolder)
        scriptsFolder = [root stringByAppendingPathComponent: CEL_SCRIPTS_FOLDER];
    else
        scriptsFolder = CEL_SCRIPTS_FOLDER;

    std::vector<ScriptMenuItem> *scripts = ScanScriptsDirectory([scriptsFolder stdString], true);
    if (NULL == scripts)
        return nil;

    NSMutableDictionary *itemDict = [NSMutableDictionary dictionary];
    NSString *title;
    id path;
    NSString *menuPath;
    size_t i;

    for (i = 0; i < scripts->size(); ++i)
    {
        title = [NSString stringWithStdString: (*scripts)[i].title];
        path  = [NSString stringWithStdString: (*scripts)[i].filename];
        menuPath = [NSString stringWithString: path];
        if (isAbsoluteFolder)
        {
            NSRange pathRange = [menuPath rangeOfString:scriptsFolder options:NSAnchoredSearch];
            if (NSNotFound != pathRange.location &&
                pathRange.location < [menuPath length])
            {
                pathRange.location += pathRange.length + 1;
                pathRange.length = [menuPath length] - pathRange.length - 1;
                menuPath = [menuPath substringWithRange:pathRange];
            }
        }
        // Build submenus for nested directories
        NSArray *subPaths = [[menuPath stringByDeletingLastPathComponent] pathComponents];
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
    delete scripts;
    return [itemDict count] > 0 ? itemDict : nil;
}

- (void)buildScriptMenu
{
    NSDictionary *itemDict = nil;
    NSArray *existingResourceDirsSetting = nil;
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    BOOL addSeparator = NO;

    int count = [scriptMenu numberOfItems];
    while (count-- > 0)
        [scriptMenu removeItemAtIndex: 0];

    if (itemDict = [ScriptsController scriptsForFolderRoot: @""])
    {
        [self addItems:itemDict toMenu:scriptMenu];
        addSeparator = YES;
    }

    if ((existingResourceDirsSetting = [prefs stringArrayForKey:@"existingResourceDirs"]))
    {
        NSEnumerator *iter = [existingResourceDirsSetting objectEnumerator];
        NSString *dir = nil;
        while ((dir = [iter nextObject]))
        {
            if (itemDict = [ScriptsController scriptsForFolderRoot: dir])
            {
                if (addSeparator)
                {
                    [scriptMenu addItem: [NSMenuItem separatorItem]];
                    addSeparator = NO;
                }
                [self addItems:[NSDictionary dictionaryWithObject:itemDict forKey:[dir stringByAbbreviatingWithTildeInPath]] toMenu:scriptMenu];
            }
        }
    }
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
        CelestiaController *controller = [CelestiaController shared];
        id filename = [aSender representedObject];
        if (filename)
        {
            [controller runScript: filename];
        }
    }
}
@end
