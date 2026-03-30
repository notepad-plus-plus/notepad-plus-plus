#pragma once
#import <Cocoa/Cocoa.h>
#include <functional>

enum class DividerOrientation
{
	Vertical,    // between left/right zones (drag changes width)
	Horizontal   // between stacked panels (drag changes height)
};

@interface PanelDividerView : NSView

@property (nonatomic, assign) DividerOrientation orientation;
@property (nonatomic, copy) void (^onDrag)(CGFloat delta);
@property (nonatomic, copy) void (^onDoubleClick)(void);

- (instancetype)initWithOrientation:(DividerOrientation)orientation;

@end

// C++ helper to create and add a divider between two views
PanelDividerView* createPanelDivider(
	DividerOrientation orientation,
	NSView* parentView,
	void (^onDrag)(CGFloat delta),
	void (^onDoubleClick)(void)
);
