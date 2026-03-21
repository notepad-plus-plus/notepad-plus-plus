// tab_bar_view.mm — Custom tab bar view with close buttons and drag-to-reorder
// Part of the Notepad++ macOS port.

#import "tab_bar_view.h"

// ============================================================
// Tab item model
// ============================================================

@interface NppTabItem : NSObject
@property (nonatomic, copy) NSString* title;
@property (nonatomic, assign) BOOL modified;
@end

@implementation NppTabItem
@end

// ============================================================
// Constants
// ============================================================

static const CGFloat kTabMinWidth = 80.0;
static const CGFloat kTabMaxWidth = 200.0;
static const CGFloat kTabPaddingH = 12.0;
static const CGFloat kTabSpacing = 1.0;
static const CGFloat kCloseButtonSize = 14.0;
static const CGFloat kCloseButtonPadding = 4.0;
static const CGFloat kModifiedDotSize = 6.0;
static const CGFloat kTabCornerRadius = 5.0;
static const CGFloat kTabTopMargin = 2.0;
static const CGFloat kDragThreshold = 5.0;

// ============================================================
// NppTabBarView private interface
// ============================================================

@interface NppTabBarView ()
{
	NSMutableArray<NppTabItem*>* _tabs;
	NSInteger _selectedIndex;
	NSInteger _hoveredIndex;
	NSInteger _hoveredCloseIndex;
	NSTrackingArea* _trackingArea;

	// Drag state
	BOOL _isDragging;
	NSInteger _dragSourceIndex;
	NSPoint _dragStartPoint;
	NSPoint _dragCurrentPoint;
	NSInteger _dragInsertionIndex;
}
@end

// ============================================================
// NppTabBarView implementation
// ============================================================

@implementation NppTabBarView

- (instancetype)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (self)
	{
		_tabs = [NSMutableArray array];
		_selectedIndex = -1;
		_hoveredIndex = -1;
		_hoveredCloseIndex = -1;
		_isDragging = NO;
		_dragSourceIndex = -1;
		_dragInsertionIndex = -1;
	}
	return self;
}

// ============================================================
// Properties
// ============================================================

- (NSInteger)numberOfTabs
{
	return static_cast<NSInteger>(_tabs.count);
}

- (NSInteger)selectedIndex
{
	return _selectedIndex;
}

// ============================================================
// Tab management
// ============================================================

- (void)insertTabWithTitle:(NSString*)title atIndex:(NSInteger)index
{
	NppTabItem* item = [[NppTabItem alloc] init];
	item.title = title ?: @"";
	item.modified = NO;

	if (index < 0 || index > static_cast<NSInteger>(_tabs.count))
		index = static_cast<NSInteger>(_tabs.count);

	[_tabs insertObject:item atIndex:static_cast<NSUInteger>(index)];

	// Adjust selected index if insertion is before it
	if (_selectedIndex >= index && _selectedIndex >= 0)
		++_selectedIndex;

	// Auto-select first tab
	if (_selectedIndex < 0 && _tabs.count > 0)
		_selectedIndex = 0;

	[self setNeedsDisplay:YES];
}

- (void)removeTabAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return;

	[_tabs removeObjectAtIndex:static_cast<NSUInteger>(index)];

	if (_tabs.count == 0)
	{
		_selectedIndex = -1;
	}
	else if (index < _selectedIndex)
	{
		--_selectedIndex;
	}
	else if (index == _selectedIndex)
	{
		if (_selectedIndex >= static_cast<NSInteger>(_tabs.count))
			_selectedIndex = static_cast<NSInteger>(_tabs.count) - 1;
	}

	_hoveredIndex = -1;
	_hoveredCloseIndex = -1;
	[self setNeedsDisplay:YES];
}

- (void)removeAllTabs
{
	[_tabs removeAllObjects];
	_selectedIndex = -1;
	_hoveredIndex = -1;
	_hoveredCloseIndex = -1;
	[self setNeedsDisplay:YES];
}

- (void)selectTabAtIndex:(NSInteger)index
{
	if (index >= 0 && index < static_cast<NSInteger>(_tabs.count))
		_selectedIndex = index;
	[self setNeedsDisplay:YES];
}

- (void)moveTabFrom:(NSInteger)fromIndex to:(NSInteger)toIndex
{
	if (fromIndex < 0 || fromIndex >= static_cast<NSInteger>(_tabs.count))
		return;
	if (toIndex < 0 || toIndex >= static_cast<NSInteger>(_tabs.count))
		return;
	if (fromIndex == toIndex)
		return;

	NppTabItem* item = _tabs[static_cast<NSUInteger>(fromIndex)];
	[_tabs removeObjectAtIndex:static_cast<NSUInteger>(fromIndex)];
	[_tabs insertObject:item atIndex:static_cast<NSUInteger>(toIndex)];

	// Adjust selected index to follow the moved tab
	if (_selectedIndex == fromIndex)
	{
		_selectedIndex = toIndex;
	}
	else
	{
		if (fromIndex < _selectedIndex && toIndex >= _selectedIndex)
			--_selectedIndex;
		else if (fromIndex > _selectedIndex && toIndex <= _selectedIndex)
			++_selectedIndex;
	}

	[self setNeedsDisplay:YES];
}

// ============================================================
// Tab properties
// ============================================================

- (void)setTitle:(NSString*)title forTabAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return;
	_tabs[static_cast<NSUInteger>(index)].title = title ?: @"";
	[self setNeedsDisplay:YES];
}

- (NSString*)titleForTabAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return @"";
	return _tabs[static_cast<NSUInteger>(index)].title;
}

- (void)setModified:(BOOL)modified forTabAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return;
	_tabs[static_cast<NSUInteger>(index)].modified = modified;
	[self setNeedsDisplay:YES];
}

- (BOOL)isModifiedAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return NO;
	return _tabs[static_cast<NSUInteger>(index)].modified;
}

// ============================================================
// Geometry helpers
// ============================================================

- (CGFloat)widthForTabAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return kTabMinWidth;

	NppTabItem* item = _tabs[static_cast<NSUInteger>(index)];
	NSString* displayTitle = item.title;

	NSDictionary* attrs = @{NSFontAttributeName: [NSFont systemFontOfSize:11.0]};
	CGFloat textWidth = [displayTitle sizeWithAttributes:attrs].width;

	// title + close button + padding + modified dot space
	CGFloat totalWidth = kTabPaddingH + textWidth + kCloseButtonPadding + kCloseButtonSize + kTabPaddingH;

	if (totalWidth < kTabMinWidth)
		totalWidth = kTabMinWidth;
	if (totalWidth > kTabMaxWidth)
		totalWidth = kTabMaxWidth;

	return totalWidth;
}

- (NSRect)rectForTabAtIndex:(NSInteger)index
{
	if (index < 0 || index >= static_cast<NSInteger>(_tabs.count))
		return NSZeroRect;

	CGFloat x = 0;
	for (NSInteger i = 0; i < index; ++i)
	{
		x += [self widthForTabAtIndex:i] + kTabSpacing;
	}

	CGFloat w = [self widthForTabAtIndex:index];
	return NSMakeRect(x, 0, w, self.bounds.size.height);
}

- (NSRect)closeButtonRectForTabRect:(NSRect)tabRect
{
	CGFloat closeX = NSMaxX(tabRect) - kCloseButtonPadding - kCloseButtonSize;
	CGFloat closeY = NSMidY(tabRect) - kCloseButtonSize / 2.0;
	return NSMakeRect(closeX, closeY, kCloseButtonSize, kCloseButtonSize);
}

- (NSInteger)tabIndexAtPoint:(NSPoint)point
{
	for (NSInteger i = 0; i < static_cast<NSInteger>(_tabs.count); ++i)
	{
		NSRect tabRect = [self rectForTabAtIndex:i];
		if (NSPointInRect(point, tabRect))
			return i;
	}
	return -1;
}

// ============================================================
// Dark/light mode detection
// ============================================================

- (BOOL)isDarkMode
{
	NSAppearanceName name = [self.effectiveAppearance
		bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
	return [name isEqualToString:NSAppearanceNameDarkAqua];
}

// ============================================================
// Drawing
// ============================================================

- (BOOL)isFlipped
{
	return NO;
}

- (void)drawRect:(NSRect)dirtyRect
{
	[super drawRect:dirtyRect];

	BOOL isDark = [self isDarkMode];

	// Background
	NSColor* bgColor = isDark
		? [NSColor colorWithCalibratedRed:0.15 green:0.15 blue:0.15 alpha:1.0]
		: [NSColor colorWithCalibratedRed:0.92 green:0.92 blue:0.92 alpha:1.0];
	[bgColor setFill];
	NSRectFill(self.bounds);

	// Bottom border
	NSColor* borderColor = isDark
		? [NSColor colorWithCalibratedRed:0.25 green:0.25 blue:0.25 alpha:1.0]
		: [NSColor colorWithCalibratedRed:0.78 green:0.78 blue:0.78 alpha:1.0];
	[borderColor setFill];
	NSRectFill(NSMakeRect(0, 0, self.bounds.size.width, 1.0));

	NSDictionary* normalAttrs = @{
		NSFontAttributeName: [NSFont systemFontOfSize:11.0],
		NSForegroundColorAttributeName: isDark
			? [NSColor colorWithCalibratedWhite:0.75 alpha:1.0]
			: [NSColor colorWithCalibratedWhite:0.3 alpha:1.0]
	};

	NSDictionary* selectedAttrs = @{
		NSFontAttributeName: [NSFont systemFontOfSize:11.0 weight:NSFontWeightMedium],
		NSForegroundColorAttributeName: isDark
			? [NSColor colorWithCalibratedWhite:0.95 alpha:1.0]
			: [NSColor colorWithCalibratedWhite:0.05 alpha:1.0]
	};

	for (NSInteger i = 0; i < static_cast<NSInteger>(_tabs.count); ++i)
	{
		// Skip the dragged tab (it will be drawn floating)
		if (_isDragging && i == _dragSourceIndex)
			continue;

		[self drawTabAtIndex:i isDark:isDark normalAttrs:normalAttrs selectedAttrs:selectedAttrs alpha:1.0 offsetX:0];
	}

	// Draw insertion indicator during drag
	if (_isDragging && _dragInsertionIndex >= 0)
	{
		NSColor* indicatorColor = isDark
			? [NSColor colorWithCalibratedRed:0.4 green:0.6 blue:1.0 alpha:0.8]
			: [NSColor colorWithCalibratedRed:0.2 green:0.4 blue:0.9 alpha:0.8];
		[indicatorColor setFill];

		CGFloat insertX = 0;
		for (NSInteger i = 0; i < _dragInsertionIndex; ++i)
		{
			if (i == _dragSourceIndex) continue;
			insertX += [self widthForTabAtIndex:i] + kTabSpacing;
		}
		NSRectFill(NSMakeRect(insertX - 1, kTabTopMargin, 2, self.bounds.size.height - kTabTopMargin * 2));
	}

	// Draw dragged tab floating at cursor
	if (_isDragging && _dragSourceIndex >= 0 && _dragSourceIndex < static_cast<NSInteger>(_tabs.count))
	{
		CGFloat offsetX = _dragCurrentPoint.x - _dragStartPoint.x;

		NSGraphicsContext* gc = [NSGraphicsContext currentContext];
		[gc saveGraphicsState];

		// Draw with slight transparency
		[self drawTabAtIndex:_dragSourceIndex isDark:isDark normalAttrs:normalAttrs selectedAttrs:selectedAttrs alpha:0.8 offsetX:offsetX];

		[gc restoreGraphicsState];
	}
}

- (void)drawTabAtIndex:(NSInteger)index isDark:(BOOL)isDark
           normalAttrs:(NSDictionary*)normalAttrs selectedAttrs:(NSDictionary*)selectedAttrs
                 alpha:(CGFloat)alpha offsetX:(CGFloat)offsetX
{
	NppTabItem* item = _tabs[static_cast<NSUInteger>(index)];
	BOOL isSelected = (index == _selectedIndex);
	BOOL isHovered = (index == _hoveredIndex);

	NSRect tabRect = [self rectForTabAtIndex:index];
	tabRect.origin.x += offsetX;

	// Tab body (inset for rounded rect)
	NSRect bodyRect = NSInsetRect(tabRect, 0, kTabTopMargin);

	NSColor* tabBg;
	if (isSelected)
	{
		tabBg = isDark
			? [NSColor colorWithCalibratedRed:0.22 green:0.22 blue:0.22 alpha:alpha]
			: [NSColor colorWithCalibratedRed:1.0 green:1.0 blue:1.0 alpha:alpha];
	}
	else if (isHovered)
	{
		tabBg = isDark
			? [NSColor colorWithCalibratedRed:0.19 green:0.19 blue:0.19 alpha:alpha]
			: [NSColor colorWithCalibratedRed:0.96 green:0.96 blue:0.96 alpha:alpha];
	}
	else
	{
		tabBg = [NSColor clearColor];
	}

	NSBezierPath* tabPath = [NSBezierPath bezierPathWithRoundedRect:bodyRect
	                                                        xRadius:kTabCornerRadius
	                                                        yRadius:kTabCornerRadius];
	[tabBg setFill];
	[tabPath fill];

	// Active tab bottom highlight
	if (isSelected)
	{
		NSColor* accentColor = isDark
			? [NSColor colorWithCalibratedRed:0.35 green:0.55 blue:0.95 alpha:alpha]
			: [NSColor colorWithCalibratedRed:0.2 green:0.4 blue:0.9 alpha:alpha];
		[accentColor setFill];
		NSRectFill(NSMakeRect(tabRect.origin.x + 2, 0, tabRect.size.width - 4, 2));
	}

	// Title text
	NSDictionary* attrs = isSelected ? selectedAttrs : normalAttrs;
	NSString* displayTitle = item.title;

	// Calculate text area (leave room for close button on right, modified dot on left)
	CGFloat textX = tabRect.origin.x + kTabPaddingH;
	CGFloat textMaxWidth = tabRect.size.width - kTabPaddingH * 2 - kCloseButtonSize - kCloseButtonPadding;

	if (item.modified)
	{
		// Draw modified dot before title
		NSColor* dotColor = isDark
			? [NSColor colorWithCalibratedRed:0.9 green:0.6 blue:0.2 alpha:alpha]
			: [NSColor colorWithCalibratedRed:0.8 green:0.4 blue:0.1 alpha:alpha];
		[dotColor setFill];
		CGFloat dotY = NSMidY(tabRect) - kModifiedDotSize / 2.0;
		NSBezierPath* dot = [NSBezierPath bezierPathWithOvalInRect:
			NSMakeRect(textX, dotY, kModifiedDotSize, kModifiedDotSize)];
		[dot fill];
		textX += kModifiedDotSize + 3.0;
		textMaxWidth -= kModifiedDotSize + 3.0;
	}

	if (textMaxWidth < 10)
		textMaxWidth = 10;

	NSSize textSize = [displayTitle sizeWithAttributes:attrs];
	CGFloat textY = NSMidY(tabRect) - textSize.height / 2.0;

	NSRect textRect = NSMakeRect(textX, textY, textMaxWidth, textSize.height);

	// Truncate with ellipsis
	NSMutableParagraphStyle* paraStyle = [[NSMutableParagraphStyle alloc] init];
	paraStyle.lineBreakMode = NSLineBreakByTruncatingTail;
	NSMutableDictionary* drawAttrs = [attrs mutableCopy];
	drawAttrs[NSParagraphStyleAttributeName] = paraStyle;

	[displayTitle drawInRect:textRect withAttributes:drawAttrs];

	// Close button (visible on active tab always, on hover for others)
	BOOL showClose = isSelected || isHovered;
	if (showClose)
	{
		NSRect closeRect = [self closeButtonRectForTabRect:tabRect];
		BOOL closeHovered = (index == _hoveredCloseIndex);

		if (closeHovered)
		{
			NSColor* closeBg = isDark
				? [NSColor colorWithCalibratedRed:0.35 green:0.35 blue:0.35 alpha:alpha]
				: [NSColor colorWithCalibratedRed:0.82 green:0.82 blue:0.82 alpha:alpha];
			[closeBg setFill];
			NSBezierPath* closeCircle = [NSBezierPath bezierPathWithOvalInRect:closeRect];
			[closeCircle fill];
		}

		// Draw X
		NSColor* xColor = isDark
			? [NSColor colorWithCalibratedWhite:0.7 alpha:alpha]
			: [NSColor colorWithCalibratedWhite:0.4 alpha:alpha];
		if (closeHovered)
		{
			xColor = isDark
				? [NSColor colorWithCalibratedWhite:0.95 alpha:alpha]
				: [NSColor colorWithCalibratedWhite:0.1 alpha:alpha];
		}

		[xColor setStroke];
		CGFloat inset = 3.5;
		NSBezierPath* xPath = [NSBezierPath bezierPath];
		xPath.lineWidth = 1.2;
		[xPath moveToPoint:NSMakePoint(closeRect.origin.x + inset, closeRect.origin.y + inset)];
		[xPath lineToPoint:NSMakePoint(NSMaxX(closeRect) - inset, NSMaxY(closeRect) - inset)];
		[xPath moveToPoint:NSMakePoint(NSMaxX(closeRect) - inset, closeRect.origin.y + inset)];
		[xPath lineToPoint:NSMakePoint(closeRect.origin.x + inset, NSMaxY(closeRect) - inset)];
		[xPath stroke];
	}
}

// ============================================================
// Tracking area (for hover detection)
// ============================================================

- (void)updateTrackingAreas
{
	[super updateTrackingAreas];

	if (_trackingArea)
		[self removeTrackingArea:_trackingArea];

	_trackingArea = [[NSTrackingArea alloc]
		initWithRect:self.bounds
		     options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveInKeyWindow)
		       owner:self
		    userInfo:nil];
	[self addTrackingArea:_trackingArea];
}

- (void)mouseMoved:(NSEvent*)event
{
	NSPoint localPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSInteger oldHovered = _hoveredIndex;
	NSInteger oldCloseHovered = _hoveredCloseIndex;

	_hoveredIndex = [self tabIndexAtPoint:localPoint];
	_hoveredCloseIndex = -1;

	if (_hoveredIndex >= 0)
	{
		NSRect tabRect = [self rectForTabAtIndex:_hoveredIndex];
		NSRect closeRect = [self closeButtonRectForTabRect:tabRect];
		if (NSPointInRect(localPoint, closeRect))
			_hoveredCloseIndex = _hoveredIndex;
	}

	if (_hoveredIndex != oldHovered || _hoveredCloseIndex != oldCloseHovered)
		[self setNeedsDisplay:YES];
}

- (void)mouseEntered:(NSEvent*)event
{
	[self mouseMoved:event];
}

- (void)mouseExited:(NSEvent*)event
{
	_hoveredIndex = -1;
	_hoveredCloseIndex = -1;
	[self setNeedsDisplay:YES];
}

// ============================================================
// Mouse events
// ============================================================

- (void)mouseDown:(NSEvent*)event
{
	NSPoint localPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSInteger clickedIndex = [self tabIndexAtPoint:localPoint];

	if (clickedIndex < 0)
		return;

	// Check if close button was clicked
	NSRect tabRect = [self rectForTabAtIndex:clickedIndex];
	NSRect closeRect = [self closeButtonRectForTabRect:tabRect];
	BOOL isSelected = (clickedIndex == _selectedIndex);
	BOOL isHovered = (clickedIndex == _hoveredIndex);
	BOOL showClose = isSelected || isHovered;

	if (showClose && NSPointInRect(localPoint, closeRect))
	{
		[_delegate tabBarView:self didCloseTabAtIndex:clickedIndex];
		return;
	}

	// Select the tab
	if (clickedIndex != _selectedIndex)
	{
		_selectedIndex = clickedIndex;
		[self setNeedsDisplay:YES];
		[_delegate tabBarView:self didSelectTabAtIndex:clickedIndex];
	}

	// Prepare for potential drag
	_dragStartPoint = localPoint;
	_dragSourceIndex = clickedIndex;
	_isDragging = NO;
}

- (void)mouseDragged:(NSEvent*)event
{
	if (_dragSourceIndex < 0)
		return;

	NSPoint localPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	_dragCurrentPoint = localPoint;

	if (!_isDragging)
	{
		CGFloat dx = localPoint.x - _dragStartPoint.x;
		CGFloat dy = localPoint.y - _dragStartPoint.y;
		if (sqrt(dx * dx + dy * dy) < kDragThreshold)
			return;
		_isDragging = YES;
	}

	// Determine insertion index
	_dragInsertionIndex = [self insertionIndexForPoint:localPoint excludingIndex:_dragSourceIndex];
	[self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent*)event
{
	if (_isDragging && _dragSourceIndex >= 0)
	{
		NSPoint localPoint = [self convertPoint:[event locationInWindow] fromView:nil];
		NSInteger dropIndex = [self insertionIndexForPoint:localPoint excludingIndex:_dragSourceIndex];

		if (dropIndex >= 0 && dropIndex != _dragSourceIndex && dropIndex != _dragSourceIndex + 1)
		{
			// Normalize: if dropping after the source, adjust for removal
			NSInteger toIndex = dropIndex;
			if (toIndex > _dragSourceIndex)
				--toIndex;

			[_delegate tabBarView:self didReorderTabFrom:_dragSourceIndex to:toIndex];
		}
	}

	_isDragging = NO;
	_dragSourceIndex = -1;
	_dragInsertionIndex = -1;
	[self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
	NSPoint localPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSInteger clickedIndex = [self tabIndexAtPoint:localPoint];

	if (clickedIndex >= 0)
	{
		// Select the right-clicked tab first
		if (clickedIndex != _selectedIndex)
		{
			_selectedIndex = clickedIndex;
			[self setNeedsDisplay:YES];
			[_delegate tabBarView:self didSelectTabAtIndex:clickedIndex];
		}
		[_delegate tabBarView:self didRightClickTabAtIndex:clickedIndex atPoint:[event locationInWindow]];
	}
}

- (void)otherMouseDown:(NSEvent*)event
{
	// Middle click (button 2)
	if (event.buttonNumber == 2)
	{
		NSPoint localPoint = [self convertPoint:[event locationInWindow] fromView:nil];
		NSInteger clickedIndex = [self tabIndexAtPoint:localPoint];
		if (clickedIndex >= 0)
			[_delegate tabBarView:self didMiddleClickTabAtIndex:clickedIndex];
	}
}

// ============================================================
// Drag helpers
// ============================================================

- (NSInteger)insertionIndexForPoint:(NSPoint)point excludingIndex:(NSInteger)excludeIndex
{
	CGFloat x = 0;
	NSInteger visualIndex = 0;

	for (NSInteger i = 0; i < static_cast<NSInteger>(_tabs.count); ++i)
	{
		if (i == excludeIndex)
			continue;

		CGFloat tabWidth = [self widthForTabAtIndex:i];
		CGFloat midX = x + tabWidth / 2.0;

		if (point.x < midX)
			return i;

		x += tabWidth + kTabSpacing;
		++visualIndex;
	}

	return static_cast<NSInteger>(_tabs.count);
}

// ============================================================
// Accessibility
// ============================================================

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
	return YES;
}

@end
