//
//  MyTree.m
//  celestia
//
//  Created by Bob Ippolito on Thu Jun 20 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#import "MyTree.h"
// HACK
#import "CelestiaFavorite.h"
#import <objc/objc-class.h>
@implementation MyVector
-(void)encodeWithCoder:(NSCoder*)coder
{
//    NSLog(@"[MyVector encodeWithCoder:%@]",coder);
    //[super encodeWithCoder:coder];
    [coder encodeObject:_array];
    //[coder encodeValueOfObjCType:@encode(Class) at:&_myClass];
}
-(id)initWithCoder:(NSCoder*)coder
{
//    NSLog(@"[MyVector initWithCoder:%@]",coder);
    //self = [super initWithCoder:coder];
    self = [self init];
    _array = [[coder decodeObject] retain];
    //[coder decodeValueOfObjCType:@encode(Class) at:&_myClass];
    return self;
}
-(id)init
{
    self = [super init];
    _array = [[NSMutableArray arrayWithCapacity:0] retain];
    _myClass = [NSObject class];
    return self;
}
-(id)initWithClass:(Class)myClass
{
    self = [self init];
    _myClass = myClass;
    return self;
}
-(void)dealloc
{
    [_array release];
    [super dealloc];
}
-(void)addObject:(id)obj
{
    if (![obj isKindOfClass:_myClass])
        [NSException raise:@"TypeError" format:@"%s invalid, only %s allowed",NAMEOF(obj),_myClass->name];
    [_array addObject:obj];
}
-(void)insertObject:(id)obj atIndex:(unsigned)idx
{
    if (![obj isKindOfClass:_myClass])
        [NSException raise:@"TypeError" format:@"%s invalid, only %s allowed",NAMEOF(obj),_myClass->name];
    [_array insertObject:obj atIndex:idx];
}
-(void)removeLastObject
{
    [_array removeLastObject];
}
-(void)removeObjectAtIndex:(unsigned)idx
{
    [_array removeObjectAtIndex:idx];
}
-(void)replaceObjectAtIndex:(unsigned)idx withObject:(id)obj
{
    if (![obj isKindOfClass:_myClass])
        [NSException raise:@"TypeError" format:@"%s invalid, only %s allowed",NAMEOF(obj),_myClass->name];
    [_array replaceObjectAtIndex:idx withObject:obj];
}
-(unsigned)count
{
    return [_array count];
}
-(id)objectAtIndex:(unsigned)idx
{
    return [_array objectAtIndex:idx];
}
@end

@implementation MyTree
-(void)encodeWithCoder:(NSCoder*)coder
{
//    NSLog(@"[MyTree encodeWithCoder:%@]",coder);
    //[super encodeWithCoder:coder];
    [coder encodeObject:_nodeValue];
    [coder encodeObject:_children];
}
-(id)initWithCoder:(NSCoder*)coder
{
//    NSLog(@"[MyTree initWithCoder:%@]",coder);
    //self = [super initWithCoder:coder];
    self = [self init];
    _parent = nil;
    _nodeValue = [[coder decodeObject] retain];
    _children = [[coder decodeObject] retain];
    [_children makeObjectsPerformSelector:@selector(setParent:) withObject:self];
    return self;
}
-(id)init
{
    self = [super init];
    _nodeValue = nil;
    _children = nil;
    _parent = nil;
    return self;
}
-(void)dealloc
{
//    NSLog(@"[MyTree dealloc]");
//    NSLog(@"%@",self);
    if ([self nodeValue] != nil)
        [[self nodeValue] autorelease];
    if ([self children] != nil)
        [[self children] autorelease];
    _nodeValue = nil;
    _children = nil;
    _parent = nil;
    [super dealloc];
}
-(id)initWithNode:(id)obj parent:(MyTree*)parent
{
    self = [self init];
    _nodeValue = [obj retain];
    _parent = parent;
    _children = nil;
    return self;
}
-(id)initWithNode:(id)obj parent:(MyTree*)parent children:(NSArray*)children
{
    NSEnumerator* enumerator;
    self = [self initWithNode:obj parent:parent];
    _children = [[MyVector alloc] initWithClass:[MyTree class]];
    enumerator = [children objectEnumerator];
    while ((obj = [enumerator nextObject]) != nil)
        [_children addObject:obj];
    return self;
}
-(id)initWithDictionary:(NSDictionary*)dict parent:(MyTree*)parent
{
    NSMutableArray* children = nil;
    NSArray* origArray = nil;
    NSEnumerator* enumerator = nil;
    NSDictionary* childDict = nil;
    id <NSCoding> nodeValue = nil;
    // NSLog(@"[MyTree initWithDictionary:%@ parent:%@]",dict,parent);
    // this part could use some work
    nodeValue = [[[CelestiaFavorite alloc] initWithDictionary:[dict objectForKey:@"nodeValue"]] autorelease];
    // Leaf
    if ((origArray = [dict objectForKey:@"children"]) == nil)
        return [self initWithNode:nodeValue parent:parent];
    children = [[[MyVector alloc] initWithClass:[MyTree class]] autorelease];
    enumerator = [origArray objectEnumerator];
    while ((childDict = [enumerator nextObject]) != nil)
        [children addObject:[[[MyTree alloc] initWithDictionary:childDict parent:self] autorelease]];
    return [self initWithNode:nodeValue parent:parent children:children];
}
-(MyTree*)parent
{
    return _parent;
}
-(MyVector*)children
{
    return _children;
}
-(id)nodeValue
{
    return _nodeValue;
}
-(BOOL)isLeaf
{
    return (([self children] == nil) ? YES : NO);
}
-(void)setNode:(id)obj
{
    if ([self nodeValue] != nil)
        [[self nodeValue] autorelease];
    _nodeValue = [obj retain];
}
-(void)setParent:(MyTree*)obj
{
    _parent = obj;
}
-(void)setChildren:(NSArray*)children
{
    NSEnumerator *enumerator = nil;
    id obj = nil;
    if ([self children] == nil)
        [[self children] autorelease];
    if (children == nil) {
        _children = nil;
        return;
    }
    enumerator = [children objectEnumerator];
    _children = [[MyVector alloc] initWithClass:[MyTree class]];
    while ((obj = [enumerator nextObject]) != nil)
        [_children addObject:obj];
}
-(NSDictionary*)dictionary
{
    return [NSDictionary dictionaryWithObjectsAndKeys:[[self nodeValue] dictionary],@"nodeValue",[NSNumber numberWithBool:[self isLeaf]],@"isLeaf",[self children],@"children",nil,nil];
}
-(NSDictionary*)recursiveDictionary
{
    NSMutableArray* array;
    NSEnumerator* enumerator;
    MyTree* obj;
    if ([self isLeaf])
        return [self dictionary];
    enumerator = [[self children] objectEnumerator];
    array = [NSMutableArray arrayWithCapacity:[[self children] count]];
    while ((obj = [enumerator nextObject]) != nil)
        [array addObject:[obj recursiveDictionary]];
    return [NSDictionary dictionaryWithObjectsAndKeys:[[self nodeValue] dictionary],@"nodeValue",[NSArray arrayWithArray:array],@"children",nil,nil];
}
-(NSString*)description
{
    return [[self dictionary] description];
}
-(BOOL)isDescendantOfNode:(MyTree*)node
{
    MyTree* parent = self;
    if ([self isEqualTo:node])
        return YES;
    while ((parent = [parent parent]) != nil)
        if ([node isEqualTo:parent])
            return YES;
    return NO;
}
-(BOOL)isDescendantOfNodeInArray:(NSArray*)array
{
    NSEnumerator* enumerator = [array objectEnumerator];
    MyTree* node = nil;
    while ((node = [enumerator nextObject]) != nil)
        if ([self isDescendantOfNode:node] == YES)
            return YES;
    return NO;
}

- (void)insertChild:(MyTree*)child atIndex:(int)index {
    [[self children] insertObject:child atIndex:index];
    [child setParent: self];
}

- (void)insertChildren:(NSArray*)children atIndex:(int)index {
    [[self children] insertObjectsFromArray: children atIndex: index];
    [children makeObjectsPerformSelector:@selector(setParent:) withObject:self];
}

- (void)_removeChildrenIdenticalTo:(NSArray*)children {
    MyTree *child;
    NSEnumerator *childEnumerator = [children objectEnumerator];
    [children makeObjectsPerformSelector:@selector(setParent:) withObject:nil];
    while ((child = [childEnumerator nextObject]) != nil) {
        [[self children] removeObjectIdenticalTo:child];
    }
}

- (void)removeChild:(MyTree*)child {
    [[self children] removeObject:child];
/*
    int index = [self indexOfChild: child];
    if (index != NSNotFound) {
        [self _removeChildrenIdenticalTo: [NSArray arrayWithObject: [self childAtIndex:index]]];
    }
*/
}

- (void)removeFromParent {
    [[self parent] removeChild:self];
}

- (int)indexOfChild:(MyTree*)child {
    return [[self children] indexOfObject:child];
}

- (int)indexOfChildIdenticalTo:(MyTree*)child {
    return [[self children] indexOfObjectIdenticalTo:child];
}

- (int)numberOfChildren {
    return [[self children] count];
}

- (MyTree*)firstChild {
    return [[self children] objectAtIndex:0];
}

- (MyTree*)lastChild {
    return [[self children] lastObject];
}

- (MyTree*)childAtIndex:(int)index {
    return [[self children] objectAtIndex:index];
}

// Returns the minimum nodes from 'allNodes' required to cover the nodes in 'allNodes'.
// This methods returns an array containing nodes from 'allNodes' such that no node in
// the returned array has an ancestor in the returned array.

// There are better ways to compute this, but this implementation should be efficient for our app.
+ (NSArray *) minimumNodeCoverFromNodesInArray: (NSArray *)allNodes {
    NSMutableArray *minimumCover = [NSMutableArray array];
    NSMutableArray *nodeQueue = [NSMutableArray arrayWithArray:allNodes];
    MyTree *node = nil;
    while ([nodeQueue count]) {
        node = [nodeQueue objectAtIndex:0];
        [nodeQueue removeObjectAtIndex:0];
        while ( [node parent] && [nodeQueue containsObjectIdenticalTo:[node parent]] ) {
            [nodeQueue removeObjectIdenticalTo: node];
            node = [node parent];
        }
        if (![node isDescendantOfNodeInArray: minimumCover]) [minimumCover addObject: node];
        [nodeQueue removeObjectIdenticalTo: node];
    }
    return minimumCover;
}

@end