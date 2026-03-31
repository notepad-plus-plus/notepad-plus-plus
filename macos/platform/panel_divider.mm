#import "panel_divider.h"

@implementation PanelDividerView
{
	NSPoint _dragStart;
	BOOL _isDragging;
}

- (instancetype)initWithOrientation:(DividerOrientation)orientation
{
	self = [super initWithFrame:NSZeroRect];
	if (self)
	{
		_orientation = orientation;
		_isDragging = NO;
		self.wantsLayer = YES;
		self.layer.backgroundColor = [[NSColor separatorColor] CGColor];
	}
	return self;
}

- (void)resetCursorRects
{
	if (_orientation == DividerOrientation::Vertical)
	{
		[self addCursorRect:self.bounds cursor:[NSCursor resizeLeftRightCursor]];
	}
	else
	{
		[self addCursorRect:self.bounds cursor:[NSCursor resizeUpDownCursor]];
	}
}

- (void)mouseDown:(NSEvent*)event
{
	_dragStart = [event locationInWindow];
	_isDragging = YES;

	if (event.clickCount == 2 && self.onDoubleClick)
	{
		self.onDoubleClick();
		_isDragging = NO;
		return;
	}
}

- (void)mouseDragged:(NSEvent*)event
{
	if (!_isDragging) return;

	NSPoint current = [event locationInWindow];
	CGFloat delta;

	if (_orientation == DividerOrientation::Vertical)
	{
		delta = current.x - _dragStart.x;
	}
	else
	{
		delta = current.y - _dragStart.y;
	}

	_dragStart = current;

	if (self.onDrag)
	{
		self.onDrag(delta);
	}
}

- (void)mouseUp:(NSEvent*)event
{
	_isDragging = NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
	return YES;
}

@end

PanelDividerView* createPanelDivider(
	DividerOrientation orientation,
	NSView* parentView,
	void (^onDrag)(CGFloat delta),
	void (^onDoubleClick)(void))
{
	PanelDividerView* divider = [[PanelDividerView alloc] initWithOrientation:orientation];
	divider.onDrag = onDrag;
	divider.onDoubleClick = onDoubleClick;
	divider.autoresizingMask = (orientation == DividerOrientation::Vertical)
		? NSViewHeightSizable
		: NSViewWidthSizable;
	[parentView addSubview:divider];
	return divider;
}
