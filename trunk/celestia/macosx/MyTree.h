//
//  MyTree.h
//  celestia
//
//  Created by Bob Ippolito on Thu Jun 20 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "NSArray_Extensions.h"

@interface MyVector : NSMutableArray {
    NSMutableArray *_array;
    Class _myClass;
}
-(id)initWithClass:(Class)myClass;
-(void)addObject:(id)obj;
-(void)insertObject:(id)obj atIndex:(unsigned)idx;
-(void)removeLastObject;
-(void)removeObjectAtIndex:(unsigned)idx;
-(void)replaceObjectAtIndex:(unsigned)idx withObject:(id)obj;
-(unsigned)count;
-(id)objectAtIndex:(unsigned)idx;
@end

@interface MyTree : NSObject {
    id _nodeValue;
    MyVector* _children;
    MyTree* _parent;
}
// for initializing a tree root node
-(id)init;
// initializing a leaf node
-(id)initWithNode:(id)obj parent:(MyTree*)parent;
// initializing a branch node
-(id)initWithNode:(id)obj parent:(MyTree*)parent children:(NSArray*)children;
-(MyVector*)children;
-(void)setNode:(id)obj;
-(void)setChildren:(NSArray*)children;
-(void)setParent:(MyTree*)parent;
-(MyTree*)parent;
-(id)nodeValue;
-(BOOL)isLeaf;
-(BOOL)isDescendantOfNode:(MyTree*)node;
-(BOOL)isDescendantOfNodeInArray:(NSArray*)array;
+(NSArray*)minimumNodeCoverFromNodesInArray:(NSArray*)allNodes;

- (void)insertChild:(MyTree*)child atIndex:(int)index;
- (void)insertChildren:(NSArray*)children atIndex:(int)index;
- (void)removeChild:(MyTree*)child;
- (void)removeFromParent;

- (int)indexOfChild:(MyTree*)child;
- (int)indexOfChildIdenticalTo:(MyTree*)child;

- (int)numberOfChildren;
- (NSArray*)children;
- (MyTree*)firstChild;
- (MyTree*)lastChild;
- (MyTree*)childAtIndex:(int)index;

@end
