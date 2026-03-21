// tab_bar_view.h — Custom tab bar view with close buttons and drag-to-reorder
// Part of the Notepad++ macOS port.

#pragma once

#import <Cocoa/Cocoa.h>

@class NppTabBarView;

@protocol NppTabBarViewDelegate <NSObject>
- (void)tabBarView:(NppTabBarView*)tabBar didSelectTabAtIndex:(NSInteger)index;
- (void)tabBarView:(NppTabBarView*)tabBar didCloseTabAtIndex:(NSInteger)index;
- (void)tabBarView:(NppTabBarView*)tabBar didReorderTabFrom:(NSInteger)fromIndex to:(NSInteger)toIndex;
- (void)tabBarView:(NppTabBarView*)tabBar didRightClickTabAtIndex:(NSInteger)index atPoint:(NSPoint)point;
- (void)tabBarView:(NppTabBarView*)tabBar didMiddleClickTabAtIndex:(NSInteger)index;
@end

@interface NppTabBarView : NSView

@property (nonatomic, weak) id<NppTabBarViewDelegate> delegate;
@property (nonatomic, readonly) NSInteger numberOfTabs;
@property (nonatomic, readonly) NSInteger selectedIndex;

// Tab management
- (void)insertTabWithTitle:(NSString*)title atIndex:(NSInteger)index;
- (void)removeTabAtIndex:(NSInteger)index;
- (void)removeAllTabs;
- (void)selectTabAtIndex:(NSInteger)index;

// Tab properties
- (void)setTitle:(NSString*)title forTabAtIndex:(NSInteger)index;
- (NSString*)titleForTabAtIndex:(NSInteger)index;
- (void)setModified:(BOOL)modified forTabAtIndex:(NSInteger)index;
- (BOOL)isModifiedAtIndex:(NSInteger)index;

// Reorder (moves tab internally without notifying delegate)
- (void)moveTabFrom:(NSInteger)fromIndex to:(NSInteger)toIndex;

// Geometry
- (NSRect)rectForTabAtIndex:(NSInteger)index;
- (NSInteger)tabIndexAtPoint:(NSPoint)point;

@end
