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
-(void)insertObject:(id)obj atIndex:(NSUInteger)idx;
-(void)removeLastObject;
-(void)removeObjectAtIndex:(NSUInteger)idx;
-(void)replaceObjectAtIndex:(NSUInteger)idx withObject:(id)obj;
-(NSUInteger)count;
-(id)objectAtIndex:(NSUInteger)idx;
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

- (void)insertChild:(MyTree*)child atIndex:(NSInteger)index;
- (void)insertChildren:(NSArray*)children atIndex:(NSInteger)index;
- (void)removeChild:(MyTree*)child;
- (void)removeFromParent;

- (NSUInteger)indexOfChild:(MyTree*)child;
- (NSUInteger)indexOfChildIdenticalTo:(MyTree*)child;

- (NSUInteger)numberOfChildren;
- (MyTree*)firstChild;
- (MyTree*)lastChild;
- (MyTree*)childAtIndex:(NSInteger)index;

@end
