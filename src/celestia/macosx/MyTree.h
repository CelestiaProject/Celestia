//
//  MyTree.h
//  celestia
//
//  Created by Bob Ippolito on Thu Jun 20 2002.
//  Copyright (c) 2002 Chris Laurel. All rights reserved.
//

#import "NSArray_Extensions.h"

@interface MyVector : NSMutableArray /*<NSCoding>*/ {
    NSMutableArray *_array;
    Class _myClass;
}
-(void)encodeWithCoder:(NSCoder*)coder;
-(id)initWithCoder:(NSCoder*)coder;
-(id)initWithClass:(Class)myClass;
-(void)addObject:(id)obj;
-(void)insertObject:(id)obj atIndex:(unsigned)idx;
-(void)removeLastObject;
-(void)removeObjectAtIndex:(unsigned)idx;
-(void)replaceObjectAtIndex:(unsigned)idx withObject:(id)obj;
-(unsigned)count;
-(id)objectAtIndex:(unsigned)idx;
@end

@interface MyTree : NSObject <NSCoding> {
    id <NSCoding> _nodeValue;
    MyVector* _children;
    MyTree* _parent;
}
-(void)encodeWithCoder:(NSCoder*)coder;
-(id)initWithCoder:(NSCoder*)coder;
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

-(id)initWithDictionary:(NSDictionary*)dict parent:(MyTree*)parent;
-(NSDictionary*)dictionary;
-(NSDictionary*)recursiveDictionary;

- (void)insertChild:(MyTree*)child atIndex:(int)index;
- (void)insertChildren:(NSArray*)children atIndex:(int)index;
- (void)removeChild:(MyTree*)child;
- (void)removeFromParent;

- (int)indexOfChild:(MyTree*)child;
- (int)indexOfChildIdenticalTo:(MyTree*)child;

- (int)numberOfChildren;
- (MyTree*)firstChild;
- (MyTree*)lastChild;
- (MyTree*)childAtIndex:(int)index;

@end
