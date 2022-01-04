
/**
 * Implementation of the native Cocoa View that serves as container for the scintilla parts.
 * @file ScintillaView.mm
 *
 * Created by Mike Lischke.
 *
 * Copyright 2011, 2013, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2009, 2011 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#include <cmath>

#include <string_view>
#include <vector>
#include <optional>

#import "ScintillaTypes.h"
#import "ScintillaMessages.h"
#import "ScintillaStructures.h"

#import "Debugging.h"
#import "Geometry.h"
#import "Platform.h"
#import "ScintillaView.h"
#import "ScintillaCocoa.h"

#if !__has_feature(objc_arc)
#error ARC must be enabled
#endif

using namespace Scintilla;
using namespace Scintilla::Internal;

// Add backend property to ScintillaView as a private category.
// Specified here as backend accessed by SCIMarginView and SCIContentView.
@interface ScintillaView()
@property(nonatomic, readonly) Scintilla::Internal::ScintillaCocoa *backend;
@end

// Two additional cursors we need, which aren't provided by Cocoa.
static NSCursor *reverseArrowCursor;
static NSCursor *waitCursor;

NSString *const SCIUpdateUINotification = @"SCIUpdateUI";

/**
 * Provide an NSCursor object that matches the Window::Cursor enumeration.
 */
static NSCursor *cursorFromEnum(Window::Cursor cursor) {
	switch (cursor) {
	case Window::Cursor::text:
		return [NSCursor IBeamCursor];
	case Window::Cursor::arrow:
		return [NSCursor arrowCursor];
	case Window::Cursor::wait:
		return waitCursor;
	case Window::Cursor::horizontal:
		return [NSCursor resizeLeftRightCursor];
	case Window::Cursor::vertical:
		return [NSCursor resizeUpDownCursor];
	case Window::Cursor::reverseArrow:
		return reverseArrowCursor;
	case Window::Cursor::up:
	default:
		return [NSCursor arrowCursor];
	}
}

@implementation SCIScrollView
- (void) tile {
	[super tile];

#if defined(MAC_OS_X_VERSION_10_14)
	if (std::floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_13) {
		NSRect frame = self.contentView.frame;
		frame.origin.x = self.verticalRulerView.requiredThickness;
		frame.size.width -= frame.origin.x;
		self.contentView.frame = frame;
	}
#endif
}
@end

// Add marginWidth and owner properties as a private category.
@interface SCIMarginView()
@property(assign) int marginWidth;
@property(nonatomic, weak) ScintillaView *owner;
@end

@implementation SCIMarginView {
	int marginWidth;
	ScintillaView *__weak owner;
	NSMutableArray *currentCursors;
}

@synthesize marginWidth, owner;

- (instancetype) initWithScrollView: (NSScrollView *) aScrollView {
	self = [super initWithScrollView: aScrollView orientation: NSVerticalRuler];
	if (self != nil) {
		owner = nil;
		marginWidth = 20;
		currentCursors = [NSMutableArray arrayWithCapacity: 0];
		for (size_t i=0; i<=SC_MAX_MARGIN; i++) {
			[currentCursors addObject: reverseArrowCursor];
		}
		self.clientView = aScrollView.documentView;
		if ([self respondsToSelector: @selector(setAccessibilityLabel:)])
			self.accessibilityLabel = @"Scintilla Margin";
	}
	return self;
}


- (void) setFrame: (NSRect) frame {
	super.frame = frame;

	[self.window invalidateCursorRectsForView: self];
}

- (CGFloat) requiredThickness {
	return marginWidth;
}

- (void) drawHashMarksAndLabelsInRect: (NSRect) aRect {
	if (owner) {
		NSRect contentRect = self.scrollView.contentView.bounds;
		NSRect marginRect = self.bounds;
		// Ensure paint to bottom of view to avoid glitches
		if (marginRect.size.height > contentRect.size.height) {
			// Legacy scroll bar mode leaves a poorly painted corner
			aRect = marginRect;
		}
		owner.backend->PaintMargin(aRect);
	}
}

/**
 * Called by the framework if it wants to show a context menu for the margin.
 */
- (NSMenu *) menuForEvent: (NSEvent *) theEvent {
	NSMenu *menu = [owner menuForEvent: theEvent];
	if (menu) {
		return menu;
	} else if (owner.backend->ShouldDisplayPopupOnMargin()) {
		return owner.backend->CreateContextMenu(theEvent);
	} else {
		return nil;
	}
}

- (void) mouseDown: (NSEvent *) theEvent {
	NSClipView *textView = self.scrollView.contentView;
	[textView.window makeFirstResponder: textView];
	owner.backend->MouseDown(theEvent);
}

- (void) rightMouseDown: (NSEvent *) theEvent {
	[NSMenu popUpContextMenu: [self menuForEvent: theEvent] withEvent: theEvent forView: self];

	owner.backend->RightMouseDown(theEvent);
}

- (void) mouseDragged: (NSEvent *) theEvent {
	owner.backend->MouseMove(theEvent);
}

- (void) mouseMoved: (NSEvent *) theEvent {
	owner.backend->MouseMove(theEvent);
}

- (void) mouseUp: (NSEvent *) theEvent {
	owner.backend->MouseUp(theEvent);
}

// Not a simple button so return failure
- (BOOL) accessibilityPerformPress {
	return NO;
}

/**
 * This method is called to give us the opportunity to define our mouse sensitive rectangle.
 */
- (void) resetCursorRects {
	[super resetCursorRects];

	int x = 0;
	NSRect marginRect = self.bounds;
	size_t co = currentCursors.count;
	for (size_t i=0; i<co; i++) {
		long cursType = owner.backend->WndProc(Message::GetMarginCursorN, i, 0);
		long width =owner.backend->WndProc(Message::GetMarginWidthN, i, 0);
		NSCursor *cc = cursorFromEnum(static_cast<Window::Cursor>(cursType));
		currentCursors[i] = cc;
		marginRect.origin.x = x;
		marginRect.size.width = width;
		[self addCursorRect: marginRect cursor: cc];
		[cc setOnMouseEntered: YES];
		x += width;
	}
}

- (void) drawRect: (NSRect) rect {
	[super drawRect:rect];
}

@end

// Add owner property as a private category.
@interface SCIContentView()
@property(nonatomic, weak) ScintillaView *owner;
@end

@implementation SCIContentView {
	ScintillaView *__weak mOwner;
	NSCursor *mCurrentCursor;
	NSTrackingArea *trackingArea;

	// Set when we are in composition mode and partial input is displayed.
	NSRange mMarkedTextRange;
}

@synthesize owner = mOwner;

//--------------------------------------------------------------------------------------------------

- (NSView *) initWithFrame: (NSRect) frame {
	self = [super initWithFrame: frame];

	if (self != nil) {
		// Some initialization for our view.
		mCurrentCursor = [NSCursor arrowCursor];
		trackingArea = nil;
		mMarkedTextRange = NSMakeRange(NSNotFound, 0);

		[self registerForDraggedTypes: @[NSStringPboardType, ScintillaRecPboardType, NSFilenamesPboardType]];

		// Set up accessibility in the text role
		if ([self respondsToSelector: @selector(setAccessibilityElement:)]) {
			self.accessibilityElement = TRUE;
			self.accessibilityEnabled = TRUE;
			self.accessibilityLabel = NSLocalizedString(@"Scintilla", nil);	// No real localization
			self.accessibilityRoleDescription = @"source code editor";
			self.accessibilityRole = NSAccessibilityTextAreaRole;
			self.accessibilityIdentifier = @"Scintilla";
		}
	}

	return self;
}

//--------------------------------------------------------------------------------------------------

/**
 * When the view is resized or scrolled we need to update our tracking area.
 */
- (void) updateTrackingAreas {
	if (trackingArea) {
		[self removeTrackingArea: trackingArea];
	}

	int opts = (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
	trackingArea = [[NSTrackingArea alloc] initWithRect: self.bounds
						    options: opts
						      owner: self
						   userInfo: nil];
	[self addTrackingArea: trackingArea];
	[super updateTrackingAreas];
}

//--------------------------------------------------------------------------------------------------

/**
 * When the view is resized we need to let the backend know.
 */
- (void) setFrame: (NSRect) frame {
	super.frame = frame;

	mOwner.backend->Resize();

	[super prepareContentInRect: [self visibleRect]];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the backend if a new cursor must be set for the view.
 */
- (void) setCursor: (int) cursor {
	Window::Cursor eCursor = (Window::Cursor)cursor;
	mCurrentCursor = cursorFromEnum(eCursor);

	// Trigger recreation of the cursor rectangle(s).
	[self.window invalidateCursorRectsForView: self];
	[mOwner updateMarginCursors];
}

//--------------------------------------------------------------------------------------------------

/**
 * This method is called to give us the opportunity to define our mouse sensitive rectangle.
 */
- (void) resetCursorRects {
	[super resetCursorRects];

	// We only have one cursor rect: our bounds.
	const NSRect visibleBounds = mOwner.backend->GetBounds();
	[self addCursorRect: visibleBounds cursor: mCurrentCursor];
	[mCurrentCursor setOnMouseEntered: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called before repainting.
 */
- (void) viewWillDraw {
	if (!mOwner) {
		[super viewWillDraw];
		return;
	}

	const NSRect *rects;
	NSInteger nRects = 0;
	[self getRectsBeingDrawn: &rects count: &nRects];
	if (nRects > 0) {
		NSRect rectUnion = rects[0];
		for (int i=0; i<nRects; i++) {
			rectUnion = NSUnionRect(rectUnion, rects[i]);
		}
		mOwner.backend->WillDraw(rectUnion);
	}
	[super viewWillDraw];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called before responsive scrolling overdraw.
 */
- (void) prepareContentInRect: (NSRect) rect {
	if (mOwner)
		mOwner.backend->WillDraw(rect);
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080
	[super prepareContentInRect: rect];
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 * Gets called by the runtime when the effective appearance changes.
 */
- (void) viewDidChangeEffectiveAppearance {
	if (mOwner.backend) {
		mOwner.backend->UpdateBaseElements();
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Gets called by the runtime when the view needs repainting.
 */
- (void) drawRect: (NSRect) rect {
	CGContextRef context = (CGContextRef) [NSGraphicsContext currentContext].graphicsPort;

	if (!mOwner.backend->Draw(rect, context)) {
		dispatch_async(dispatch_get_main_queue(), ^ {
			[self setNeedsDisplay: YES];
		});
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Windows uses a client coordinate system where the upper left corner is the origin in a window
 * (and so does Scintilla). We have to adjust for that. However by returning YES here, we are
 * already done with that.
 * Note that because of returning YES here most coordinates we use now (e.g. for painting,
 * invalidating rectangles etc.) are given with +Y pointing down!
 */
- (BOOL) isFlipped {
	return YES;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) isOpaque {
	return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Implement the "click through" behavior by telling the caller we accept the first mouse event too.
 */
- (BOOL) acceptsFirstMouse: (NSEvent *) theEvent {
#pragma unused(theEvent)
	return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Make this view accepting events as first responder.
 */
- (BOOL) acceptsFirstResponder {
	return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the framework if it wants to show a context menu for the editor.
 */
- (NSMenu *) menuForEvent: (NSEvent *) theEvent {
	NSMenu *menu = [mOwner menuForEvent: theEvent];
	if (menu) {
		return menu;
	} else if (mOwner.backend->ShouldDisplayPopupOnText()) {
		return mOwner.backend->CreateContextMenu(theEvent);
	} else {
		return nil;
	}
}

//--------------------------------------------------------------------------------------------------

// Adoption of NSTextInputClient protocol.

- (NSAttributedString *) attributedSubstringForProposedRange: (NSRange) aRange actualRange: (NSRangePointer) actualRange {
	const NSInteger lengthCharacters = self.accessibilityNumberOfCharacters;
	if (aRange.location > lengthCharacters) {
		return nil;
	}
	const NSRange posRange = mOwner.backend->PositionsFromCharacters(aRange);
	// The backend validated aRange and may have removed characters beyond the end of the document.
	const NSRange charRange = mOwner.backend->CharactersFromPositions(posRange);
	if (!NSEqualRanges(aRange, charRange)) {
		*actualRange = charRange;
	}

	[mOwner message: SCI_SETTARGETRANGE wParam: posRange.location lParam: NSMaxRange(posRange)];
	std::string text([mOwner message: SCI_TARGETASUTF8], 0);
	[mOwner message: SCI_TARGETASUTF8 wParam: 0 lParam: reinterpret_cast<sptr_t>(&text[0])];
	text = FixInvalidUTF8(text);
	NSString *result = @(text.c_str());
	NSMutableAttributedString *asResult = [[NSMutableAttributedString alloc] initWithString: result];

	const NSRange rangeAS = NSMakeRange(0, asResult.length);
	// SCI_GETSTYLEAT reports a signed byte but want an unsigned to index into styles
	const char styleByte = static_cast<char>([mOwner message: SCI_GETSTYLEAT wParam: posRange.location]);
	const long style = static_cast<unsigned char>(styleByte);
	std::string fontName([mOwner message: SCI_STYLEGETFONT wParam: style lParam: 0], 0);
	[mOwner message: SCI_STYLEGETFONT wParam: style lParam: (sptr_t)&fontName[0]];
	const CGFloat fontSize = [mOwner message: SCI_STYLEGETSIZEFRACTIONAL wParam: style] / 100.0f;
	NSString *sFontName = @(fontName.c_str());
	NSFont *font = [NSFont fontWithName: sFontName size: fontSize];
	if (font) {
		[asResult addAttribute: NSFontAttributeName value: font range: rangeAS];
	}

	return asResult;
}

//--------------------------------------------------------------------------------------------------

- (NSUInteger) characterIndexForPoint: (NSPoint) point {
	const NSRect rectPoint = {point, NSZeroSize};
	const NSRect rectInWindow = [self.window convertRectFromScreen: rectPoint];
	const NSRect rectLocal = [self.superview.superview convertRect: rectInWindow fromView: nil];

	const long position = [mOwner message: SCI_CHARPOSITIONFROMPOINT
				       wParam: rectLocal.origin.x
				       lParam: rectLocal.origin.y];
	if (position == Sci::invalidPosition) {
		return NSNotFound;
	} else {
		const NSRange index = mOwner.backend->CharactersFromPositions(NSMakeRange(position, 0));
		return index.location;
	}
}

//--------------------------------------------------------------------------------------------------

- (void) doCommandBySelector: (SEL) selector {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
	if ([self respondsToSelector: selector])
		[self performSelector: selector withObject: nil];
#pragma clang diagnostic pop
}

//--------------------------------------------------------------------------------------------------

- (NSRect) firstRectForCharacterRange: (NSRange) aRange actualRange: (NSRangePointer) actualRange {
#pragma unused(actualRange)
	const NSRange posRange = mOwner.backend->PositionsFromCharacters(aRange);

	NSRect rect;
	rect.origin.x = [mOwner message: SCI_POINTXFROMPOSITION wParam: 0 lParam: posRange.location];
	rect.origin.y = [mOwner message: SCI_POINTYFROMPOSITION wParam: 0 lParam: posRange.location];
	const NSUInteger rangeEnd = NSMaxRange(posRange);
	rect.size.width = [mOwner message: SCI_POINTXFROMPOSITION wParam: 0 lParam: rangeEnd] - rect.origin.x;
	rect.size.height = [mOwner message: SCI_POINTYFROMPOSITION wParam: 0 lParam: rangeEnd] - rect.origin.y;
	rect.size.height += [mOwner message: SCI_TEXTHEIGHT wParam: 0 lParam: 0];
	const NSRect rectInWindow = [self.superview.superview convertRect: rect toView: nil];
	const NSRect rectScreen = [self.window convertRectToScreen: rectInWindow];

	return rectScreen;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) hasMarkedText {
	return mMarkedTextRange.length > 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * General text input. Used to insert new text at the current input position, replacing the current
 * selection if there is any.
 * First removes the replacementRange.
 */
- (void) insertText: (id) aString replacementRange: (NSRange) replacementRange {
	if ((mMarkedTextRange.location != NSNotFound) && (replacementRange.location != NSNotFound)) {
		NSLog(@"Trying to insertText when there is both a marked range and a replacement range");
	}

	// Remove any previously marked text first.
	mOwner.backend->CompositionUndo();
	if (mMarkedTextRange.location != NSNotFound) {
		const NSRange posRangeMark = mOwner.backend->PositionsFromCharacters(mMarkedTextRange);
		[mOwner message: SCI_SETEMPTYSELECTION wParam: posRangeMark.location];
	}
	mMarkedTextRange = NSMakeRange(NSNotFound, 0);

	if (replacementRange.location == (NSNotFound-1))
		// This occurs when the accent popup is visible and menu selected.
		// Its replacing a non-existent position so do nothing.
		return;

	if (replacementRange.location != NSNotFound) {
		const NSRange posRangeReplacement = mOwner.backend->PositionsFromCharacters(replacementRange);
		[mOwner message: SCI_DELETERANGE
			 wParam: posRangeReplacement.location
			 lParam: posRangeReplacement.length];
		[mOwner message: SCI_SETEMPTYSELECTION wParam: posRangeReplacement.location];
	}

	NSString *newText = @"";
	if ([aString isKindOfClass: [NSString class]])
		newText = (NSString *) aString;
	else if ([aString isKindOfClass: [NSAttributedString class]])
		newText = (NSString *) [aString string];

	mOwner.backend->InsertText(newText, CharacterSource::DirectInput);
}

//--------------------------------------------------------------------------------------------------

- (NSRange) markedRange {
	return mMarkedTextRange;
}

//--------------------------------------------------------------------------------------------------

- (NSRange) selectedRange {
	const NSRange posRangeSel = [mOwner selectedRangePositions];
	if (posRangeSel.length == 0) {
		NSTextInputContext *tic = [NSTextInputContext currentInputContext];
		// Chinese input causes malloc crash when empty selection returned with actual
		// position so return NSNotFound.
		// If this is applied to European input, it stops the accented character
		// chooser from appearing.
		// May need to add more input source names.
		if ([tic.selectedKeyboardInputSource
				isEqualToString: @"com.apple.inputmethod.TCIM.Cangjie"]) {
			return NSMakeRange(NSNotFound, 0);
		}
	}
	return mOwner.backend->CharactersFromPositions(posRangeSel);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the input manager to set text which might be combined with further input to form
 * the final text (e.g. composition of ^ and a to â).
 *
 * @param aString The text to insert, either what has been marked already or what is selected already
 *                or simply added at the current insertion point. Depending on what is available.
 * @param range The range of the new text to select (given relative to the insertion point of the new text).
 * @param replacementRange The range to remove before insertion.
 */
- (void) setMarkedText: (id) aString selectedRange: (NSRange) range replacementRange: (NSRange) replacementRange {
	NSString *newText = @"";
	if ([aString isKindOfClass: [NSString class]])
		newText = (NSString *) aString;
	else if ([aString isKindOfClass: [NSAttributedString class]])
		newText = (NSString *) [aString string];

	// Replace marked text if there is one.
	if (mMarkedTextRange.length > 0) {
		mOwner.backend->CompositionUndo();
		if (replacementRange.location != NSNotFound) {
			// This situation makes no sense and has not occurred in practice.
			NSLog(@"Can not handle a replacement range when there is also a marked range");
		} else {
			replacementRange = mMarkedTextRange;
			const NSRange posRangeMark = mOwner.backend->PositionsFromCharacters(mMarkedTextRange);
			[mOwner message: SCI_SETEMPTYSELECTION wParam: posRangeMark.location];
		}
	} else {
		// Must perform deletion before entering composition mode or else
		// both document and undo history will not contain the deleted text
		// leading to an inaccurate and unusable undo history.

		// Convert selection virtual space into real space
		mOwner.backend->ConvertSelectionVirtualSpace();

		if (replacementRange.location != NSNotFound) {
			const NSRange posRangeReplacement = mOwner.backend->PositionsFromCharacters(replacementRange);
			[mOwner message: SCI_DELETERANGE
				 wParam: posRangeReplacement.location
				 lParam: posRangeReplacement.length];
		} else { // No marked or replacement range, so replace selection
			if (!mOwner.backend->ScintillaCocoa::ClearAllSelections()) {
				// Some of the selection is protected so can not perform composition here
				return;
			}
			// Ensure only a single selection.
			mOwner.backend->SelectOnlyMainSelection();
			const NSRange posRangeSel = [mOwner selectedRangePositions];
			replacementRange = mOwner.backend->CharactersFromPositions(posRangeSel);
		}
	}

	// To support IME input to multiple selections, the following code would
	// need to insert newText at each selection, mark each piece of new text and then
	// select range relative to each insertion.

	if (newText.length) {
		// Switching into composition.
		mOwner.backend->CompositionStart();

		NSRange posRangeCurrent = mOwner.backend->PositionsFromCharacters(NSMakeRange(replacementRange.location, 0));
		// Note: Scintilla internally works almost always with bytes instead chars, so we need to take
		//       this into account when determining selection ranges and such.
		ptrdiff_t lengthInserted = mOwner.backend->InsertText(newText, CharacterSource::TentativeInput);
		posRangeCurrent.length = lengthInserted;
		mMarkedTextRange = mOwner.backend->CharactersFromPositions(posRangeCurrent);
		// Mark the just inserted text. Keep the marked range for later reset.
		[mOwner setGeneralProperty: SCI_SETINDICATORCURRENT value: INDICATOR_IME];
		[mOwner setGeneralProperty: SCI_INDICATORFILLRANGE
				 parameter: posRangeCurrent.location
				     value: posRangeCurrent.length];
	} else {
		mMarkedTextRange = NSMakeRange(NSNotFound, 0);
		// Re-enable undo action collection if composition ended (indicated by an empty mark string).
		mOwner.backend->CompositionCommit();
	}

	// Select the part which is indicated in the given range. It does not scroll the caret into view.
	if (range.length > 0) {
		// range is in characters so convert to bytes for selection.
		range.location += replacementRange.location;
		NSRange posRangeSelect = mOwner.backend->PositionsFromCharacters(range);
		[mOwner setGeneralProperty: SCI_SETSELECTION parameter: NSMaxRange(posRangeSelect) value: posRangeSelect.location];
	}
}

//--------------------------------------------------------------------------------------------------

- (void) unmarkText {
	if (mMarkedTextRange.length > 0) {
		mOwner.backend->CompositionCommit();
		mMarkedTextRange = NSMakeRange(NSNotFound, 0);
	}
}

//--------------------------------------------------------------------------------------------------

- (NSArray *) validAttributesForMarkedText {
	return @[];
}

// End of the NSTextInputClient protocol adoption.

//--------------------------------------------------------------------------------------------------

/**
 * Generic input method. It is used to pass on keyboard input to Scintilla. The control itself only
 * handles shortcuts. The input is then forwarded to the Cocoa text input system, which in turn does
 * its own input handling (character composition via NSTextInputClient protocol):
 */
- (void) keyDown: (NSEvent *) theEvent {
	bool handled = false;
	if (mMarkedTextRange.length == 0)
		handled = mOwner.backend->KeyboardInput(theEvent);
	if (!handled) {
		NSArray *events = @[theEvent];
		[self interpretKeyEvents: events];
	}
}

//--------------------------------------------------------------------------------------------------

- (void) mouseDown: (NSEvent *) theEvent {
	mOwner.backend->MouseDown(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseDragged: (NSEvent *) theEvent {
	mOwner.backend->MouseMove(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseUp: (NSEvent *) theEvent {
	mOwner.backend->MouseUp(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseMoved: (NSEvent *) theEvent {
	mOwner.backend->MouseMove(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseEntered: (NSEvent *) theEvent {
	mOwner.backend->MouseEntered(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseExited: (NSEvent *) theEvent {
	mOwner.backend->MouseExited(theEvent);
}

//--------------------------------------------------------------------------------------------------

/**
 * Implementing scrollWheel makes scrolling work better even if just
 * calling super.
 * Mouse wheel with command key may magnify text if enabled.
 * Pinch gestures and key commands can also be used for magnification.
 */
- (void) scrollWheel: (NSEvent *) theEvent {
#ifdef SCROLL_WHEEL_MAGNIFICATION
	if (([theEvent modifierFlags] & NSEventModifierFlagCommand) != 0) {
		mOwner.backend->MouseWheel(theEvent);
		return;
	}
#endif
	[super scrollWheel: theEvent];
}

//--------------------------------------------------------------------------------------------------

/**
 * Ensure scrolling is aligned to whole lines instead of starting part-way through a line
 */
- (NSRect) adjustScroll: (NSRect) proposedVisibleRect {
	if (!mOwner)
		return proposedVisibleRect;
	NSRect rc = proposedVisibleRect;
	// Snap to lines
	NSRect contentRect = self.bounds;
	if ((rc.origin.y > 0) && (NSMaxY(rc) < contentRect.size.height)) {
		// Only snap for positions inside the document - allow outside
		// for overshoot.
		long lineHeight = mOwner.backend->WndProc(Message::TextHeight, 0, 0);
		rc.origin.y = std::round(static_cast<XYPOSITION>(rc.origin.y) / lineHeight) * lineHeight;
	}
	// Snap to whole points - on retina displays this avoids visual debris
	// when scrolling horizontally.
	if ((rc.origin.x > 0) && (NSMaxX(rc) < contentRect.size.width)) {
		// Only snap for positions inside the document - allow outside
		// for overshoot.
		rc.origin.x = std::round(static_cast<XYPOSITION>(rc.origin.x));
	}
	return rc;
}

//--------------------------------------------------------------------------------------------------

/**
 * The editor is getting the foreground control (the one getting the input focus).
 */
- (BOOL) becomeFirstResponder {
	mOwner.backend->SetFirstResponder(true);
	return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * The editor is losing the input focus.
 */
- (BOOL) resignFirstResponder {
	mOwner.backend->SetFirstResponder(false);
	return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Implement NSDraggingSource.
 */

- (NSDragOperation) draggingSession: (NSDraggingSession *) session
	sourceOperationMaskForDraggingContext: (NSDraggingContext) context {
#pragma unused(session)
	switch (context) {
	case NSDraggingContextOutsideApplication:
		return NSDragOperationCopy | NSDragOperationMove | NSDragOperationDelete;

	case NSDraggingContextWithinApplication:
	default:
		return NSDragOperationCopy | NSDragOperationMove | NSDragOperationDelete;
	}
}

- (void) draggingSession: (NSDraggingSession *) session
	    movedToPoint: (NSPoint) screenPoint {
#pragma unused(session, screenPoint)
}

- (void) draggingSession: (NSDraggingSession *) session
	    endedAtPoint: (NSPoint) screenPoint
	       operation: (NSDragOperation) operation {
#pragma unused(session, screenPoint)
	if (operation == NSDragOperationDelete) {
		mOwner.backend->WndProc(Message::Clear, 0, 0);
	}
}

/**
 * Implement NSDraggingDestination.
 */

//--------------------------------------------------------------------------------------------------

/**
 * Called when an external drag operation enters the view.
 */
- (NSDragOperation) draggingEntered: (id <NSDraggingInfo>) sender {
	return mOwner.backend->DraggingEntered(sender);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called frequently during an external drag operation if we are the target.
 */
- (NSDragOperation) draggingUpdated: (id <NSDraggingInfo>) sender {
	return mOwner.backend->DraggingUpdated(sender);
}

//--------------------------------------------------------------------------------------------------

/**
 * Drag image left the view. Clean up if necessary.
 */
- (void) draggingExited: (id <NSDraggingInfo>) sender {
	mOwner.backend->DraggingExited(sender);
}

//--------------------------------------------------------------------------------------------------

- (BOOL) prepareForDragOperation: (id <NSDraggingInfo>) sender {
#pragma unused(sender)
	return YES;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) performDragOperation: (id <NSDraggingInfo>) sender {
	return mOwner.backend->PerformDragOperation(sender);
}

//--------------------------------------------------------------------------------------------------

/**
 * Drag operation is done. Notify editor.
 */
- (void) concludeDragOperation: (id <NSDraggingInfo>) sender {
	// Clean up is the same as if we are no longer the drag target.
	mOwner.backend->DraggingExited(sender);
}

//--------------------------------------------------------------------------------------------------

// NSResponder actions.

- (void) selectAll: (id) sender {
#pragma unused(sender)
	mOwner.backend->SelectAll();
}

- (void) deleteBackward: (id) sender {
#pragma unused(sender)
	mOwner.backend->DeleteBackward();
}

- (void) cut: (id) sender {
#pragma unused(sender)
	mOwner.backend->Cut();
}

- (void) copy: (id) sender {
#pragma unused(sender)
	mOwner.backend->Copy();
}

- (void) paste: (id) sender {
#pragma unused(sender)
	if (mMarkedTextRange.location != NSNotFound) {
		[[NSTextInputContext currentInputContext] discardMarkedText];
		mOwner.backend->CompositionCommit();
		mMarkedTextRange = NSMakeRange(NSNotFound, 0);
	}
	mOwner.backend->Paste();
}

- (void) undo: (id) sender {
#pragma unused(sender)
	if (mMarkedTextRange.location != NSNotFound) {
		[[NSTextInputContext currentInputContext] discardMarkedText];
		mOwner.backend->CompositionCommit();
		mMarkedTextRange = NSMakeRange(NSNotFound, 0);
	}
	mOwner.backend->Undo();
}

- (void) redo: (id) sender {
#pragma unused(sender)
	mOwner.backend->Redo();
}

- (BOOL) canUndo {
	return mOwner.backend->CanUndo() && (mMarkedTextRange.location == NSNotFound);
}

- (BOOL) canRedo {
	return mOwner.backend->CanRedo();
}

- (BOOL) validateUserInterfaceItem: (id <NSValidatedUserInterfaceItem>) anItem {
	SEL action = anItem.action;
	if (action==@selector(undo:)) {
		return [self canUndo];
	} else if (action==@selector(redo:)) {
		return [self canRedo];
	} else if (action==@selector(cut:) || action==@selector(copy:) || action==@selector(clear:)) {
		return mOwner.backend->HasSelection();
	} else if (action==@selector(paste:)) {
		return mOwner.backend->CanPaste();
	}
	return YES;
}

- (void) clear: (id) sender {
	[self deleteBackward: sender];
}

- (BOOL) isEditable {
	return mOwner.backend->WndProc(Message::GetReadOnly, 0, 0) == 0;
}

#pragma mark - NSAccessibility

//--------------------------------------------------------------------------------------------------

// Adoption of NSAccessibility protocol.
// NSAccessibility wants to pass ranges in UTF-16 code units, not bytes (like Scintilla)
// or characters.
// Needs more testing with non-ASCII and non-BMP text.
// Needs to take account of folding and wraping.

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Text of the whole document as a string.
 */
- (id) accessibilityValue {
	const sptr_t length = [mOwner message: SCI_GETLENGTH];
	return mOwner.backend->RangeTextAsString(NSMakeRange(0, length));
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Line of the caret.
 */
- (NSInteger) accessibilityInsertionPointLineNumber {
	const Sci::Position caret = [mOwner message: SCI_GETCURRENTPOS];
	const NSRange rangeCharactersCaret = mOwner.backend->CharactersFromPositions(NSMakeRange(caret, 0));
	return mOwner.backend->VisibleLineForIndex(rangeCharactersCaret.location);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Not implemented and not called by VoiceOver.
 */
- (NSRange) accessibilityRangeForPosition: (NSPoint) point {
	return NSMakeRange(0, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Number of characters in the whole document.
 */
- (NSInteger) accessibilityNumberOfCharacters {
	sptr_t length = [mOwner message: SCI_GETLENGTH];
	const NSRange posRange = mOwner.backend->CharactersFromPositions(NSMakeRange(length, 0));
	return posRange.location;
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : The selection text as a string.
 */
- (NSString *) accessibilitySelectedText {
	const sptr_t positionBegin = [mOwner message: SCI_GETSELECTIONSTART];
	const sptr_t positionEnd = [mOwner message: SCI_GETSELECTIONEND];
	const NSRange posRangeSel = NSMakeRange(positionBegin, positionEnd-positionBegin);
	return mOwner.backend->RangeTextAsString(posRangeSel);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : The character range of the main selection.
 */
- (NSRange) accessibilitySelectedTextRange {
	const sptr_t positionBegin = [mOwner message: SCI_GETSELECTIONSTART];
	const sptr_t positionEnd = [mOwner message: SCI_GETSELECTIONEND];
	const NSRange posRangeSel = NSMakeRange(positionBegin, positionEnd-positionBegin);
	return mOwner.backend->CharactersFromPositions(posRangeSel);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : The setter for accessibilitySelectedTextRange.
 * This method is the only setter required for reasonable VoiceOver behaviour.
 */
- (void) setAccessibilitySelectedTextRange: (NSRange) range {
	NSRange rangePositions = mOwner.backend->PositionsFromCharacters(range);
	[mOwner message: SCI_SETSELECTION wParam: rangePositions.location lParam: NSMaxRange(rangePositions)];
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Range of the glyph at a character index.
 * Currently doesn't try to handle composite characters.
 */
- (NSRange) accessibilityRangeForIndex: (NSInteger) index {
	sptr_t length = [mOwner message: SCI_GETLENGTH];
	const NSRange rangeLength = mOwner.backend->CharactersFromPositions(NSMakeRange(length, 0));
	NSRange rangePositions = NSMakeRange(length, 0);
	if (index < rangeLength.location) {
		rangePositions = mOwner.backend->PositionsFromCharacters(NSMakeRange(index, 1));
	}
	return mOwner.backend->CharactersFromPositions(rangePositions);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : All the text ranges.
 * Currently only returns the main selection.
 */
- (NSArray<NSValue *> *) accessibilitySelectedTextRanges {
	const sptr_t positionBegin = [mOwner message: SCI_GETSELECTIONSTART];
	const sptr_t positionEnd = [mOwner message: SCI_GETSELECTIONEND];
	const NSRange posRangeSel = NSMakeRange(positionBegin, positionEnd-positionBegin);
	NSRange rangeCharacters = mOwner.backend->CharactersFromPositions(posRangeSel);
	NSValue *valueRange = [NSValue valueWithRange: (NSRange)rangeCharacters];
	return @[valueRange];
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Character range currently visible.
 */
- (NSRange) accessibilityVisibleCharacterRange {
	const sptr_t lineTopVisible = [mOwner message: SCI_GETFIRSTVISIBLELINE];
	const sptr_t lineTop = [mOwner message: SCI_DOCLINEFROMVISIBLE wParam: lineTopVisible];
	const sptr_t lineEndVisible = lineTopVisible + [mOwner message: SCI_LINESONSCREEN] - 1;
	const sptr_t lineEnd = [mOwner message: SCI_DOCLINEFROMVISIBLE wParam: lineEndVisible];
	const sptr_t posStartView = [mOwner message: SCI_POSITIONFROMLINE wParam: lineTop];
	const sptr_t posEndView = [mOwner message: SCI_GETLINEENDPOSITION wParam: lineEnd];
	const NSRange posRangeSel = NSMakeRange(posStartView, posEndView-posStartView);
	return mOwner.backend->CharactersFromPositions(posRangeSel);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Character range of a line.
 */
- (NSRange) accessibilityRangeForLine: (NSInteger) line {
	return mOwner.backend->RangeForVisibleLine(line);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Line number of a text position in characters.
 */
- (NSInteger) accessibilityLineForIndex: (NSInteger) index {
	return mOwner.backend->VisibleLineForIndex(index);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : A rectangle that covers a range which will be shown as the
 * VoiceOver cursor.
 * Producing a nice rectangle is a little tricky particularly when including new
 * lines. Needs to improve the case where parts of two lines are included.
 */
- (NSRect) accessibilityFrameForRange: (NSRange) range {
	const NSRect rectInView = mOwner.backend->FrameForRange(range);
	const NSRect rectInWindow = [self.superview.superview convertRect: rectInView toView: nil];
	return [self.window convertRectToScreen: rectInWindow];
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : A range of text as a string.
 */
- (NSString *) accessibilityStringForRange: (NSRange) range {
	const NSRange posRange = mOwner.backend->PositionsFromCharacters(range);
	return mOwner.backend->RangeTextAsString(posRange);
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : A range of text as an attributed string.
 * Currently no attributes are set.
 */
- (NSAttributedString *) accessibilityAttributedStringForRange: (NSRange) range {
	const NSRange posRange = mOwner.backend->PositionsFromCharacters(range);
	NSString *result = mOwner.backend->RangeTextAsString(posRange);
	return [[NSMutableAttributedString alloc] initWithString: result];
}

//--------------------------------------------------------------------------------------------------

/**
 * NSAccessibility : Show the context menu at the caret.
 */
- (BOOL) accessibilityPerformShowMenu {
	const sptr_t caret = [mOwner message: SCI_GETCURRENTPOS];
	NSRect rect;
	rect.origin.x = [mOwner message: SCI_POINTXFROMPOSITION wParam: 0 lParam: caret];
	rect.origin.y = [mOwner message: SCI_POINTYFROMPOSITION wParam: 0 lParam: caret];
	rect.origin.y += [mOwner message: SCI_TEXTHEIGHT wParam: 0 lParam: 0];
	rect.size.width = 1.0;
	rect.size.height = 1.0;
	NSRect rectInWindow = [self.superview.superview convertRect: rect toView: nil];
	NSPoint pt = rectInWindow.origin;
	NSEvent *event = [NSEvent mouseEventWithType: NSEventTypeRightMouseDown
					    location: pt
				       modifierFlags: 0
					   timestamp: 0
					windowNumber: self.window.windowNumber
					     context: nil
					 eventNumber: 0
					  clickCount: 1
					    pressure: 0.0];
	NSMenu *menu = mOwner.backend->CreateContextMenu(event);
	[NSMenu popUpContextMenu: menu withEvent: event forView: self];
	return YES;
}

//--------------------------------------------------------------------------------------------------


@end

//--------------------------------------------------------------------------------------------------

@implementation ScintillaView {
	// The back end is kind of a controller and model in one.
	// It uses the content view for display.
	Scintilla::Internal::ScintillaCocoa *mBackend;

	// This is the actual content to which the backend renders itself.
	SCIContentView *mContent;

	NSScrollView *scrollView;
	SCIMarginView *marginView;

	CGFloat zoomDelta;

	// Area to display additional controls (e.g. zoom info, caret position, status info).
	NSView <InfoBarCommunicator> *mInfoBar;
	BOOL mInfoBarAtTop;

	id<ScintillaNotificationProtocol> __unsafe_unretained mDelegate;
}

@synthesize backend = mBackend;
@synthesize delegate = mDelegate;
@synthesize scrollView;

/**
 * ScintillaView is a composite control made from an NSView and an embedded NSView that is
 * used as canvas for the output (by the backend, using its CGContext), plus other elements
 * (scrollers, info bar).
 */

//--------------------------------------------------------------------------------------------------

/**
 * Initialize custom cursor.
 */
+ (void) initialize {
	if (self == [ScintillaView class]) {
		NSBundle *bundle = [NSBundle bundleForClass: [ScintillaView class]];

		NSString *path = [bundle pathForResource: @"mac_cursor_busy" ofType: @"tiff" inDirectory: nil];
		NSImage *image = [[NSImage alloc] initWithContentsOfFile: path];
		if (image) {
			waitCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint(2, 2)];
		} else {
			NSLog(@"Wait cursor is invalid.");
			waitCursor = [NSCursor arrowCursor];
		}

		path = [bundle pathForResource: @"mac_cursor_flipped" ofType: @"tiff" inDirectory: nil];
		image = [[NSImage alloc] initWithContentsOfFile: path];
		if (image) {
			reverseArrowCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint(15, 2)];
		} else {
			NSLog(@"Reverse arrow cursor is invalid.");
			reverseArrowCursor = [NSCursor arrowCursor];
		}
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Specify the SCIContentView class. Can be overridden in a subclass to provide an SCIContentView subclass.
 */

+ (Class) contentViewClass {
	return [SCIContentView class];
}

//--------------------------------------------------------------------------------------------------

/**
 * Receives zoom messages, for example when a "pinch zoom" is performed on the trackpad.
 */
- (void) magnifyWithEvent: (NSEvent *) event {
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
	zoomDelta += event.magnification * 10.0;

	if (std::abs(zoomDelta)>=1.0) {
		long zoomFactor = static_cast<long>([self getGeneralProperty: SCI_GETZOOM] + zoomDelta);
		[self setGeneralProperty: SCI_SETZOOM parameter: zoomFactor value: 0];
		zoomDelta = 0.0;
	}
#endif
}

- (void) beginGestureWithEvent: (NSEvent *) event {
// Scintilla is only interested in this event as the starft of a zoom
#pragma unused(event)
	zoomDelta = 0.0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Sends a new notification of the given type to the default notification center.
 */
- (void) sendNotification: (NSString *) notificationName {
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center postNotificationName: notificationName object: self];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by a connected component (usually the info bar) if something changed there.
 *
 * @param type The type of the notification.
 * @param message Carries the new status message if the type is a status message change.
 * @param location Carries the new location (e.g. caret) if the type is a caret change or similar type.
 * @param value Carries the new zoom value if the type is a zoom change.
 */
- (void) notify: (NotificationType) type message: (NSString *) message location: (NSPoint) location
	  value: (float) value {
// These parameters are just to conform to the protocol
#pragma unused(message)
#pragma unused(location)
	switch (type) {
	case IBNZoomChanged: {
			// Compute point increase/decrease based on default font size.
			long fontSize = [self getGeneralProperty: SCI_STYLEGETSIZE parameter: STYLE_DEFAULT];
			int zoom = (int)(fontSize * (value - 1));
			[self setGeneralProperty: SCI_SETZOOM value: zoom];
			break;
		}
	default:
		break;
	};
}

//--------------------------------------------------------------------------------------------------

- (void) setCallback: (id <InfoBarCommunicator>) callback {
// Not used. Only here to satisfy protocol.
#pragma unused(callback)
}

//--------------------------------------------------------------------------------------------------

/**
 * Prevents drawing of the inner view to avoid flickering when doing many visual updates
 * (like clearing all marks and setting new ones etc.).
 */
- (void) suspendDrawing: (BOOL) suspend {
	if (suspend)
		[self.window disableFlushWindow];
	else
		[self.window enableFlushWindow];
}

//--------------------------------------------------------------------------------------------------

/**
 * Method receives notifications from Scintilla (e.g. for handling clicks on the
 * folder margin or changes in the editor).
 * A delegate can be set to receive all notifications. If set no handling takes place here, except
 * for action pertaining to internal stuff (like the info bar).
 */
- (void) notification: (SCNotification *) scn {
	// Parent notification. Details are passed as SCNotification structure.

	if (mDelegate != nil) {
		[mDelegate notification: scn];
		if (scn->nmhdr.code != static_cast<unsigned int>(Notification::Zoom) &&
		    scn->nmhdr.code != static_cast<unsigned int>(Notification::UpdateUI))
			return;
	}

	switch (static_cast<Notification>(scn->nmhdr.code)) {
	case Notification::MarginClick: {
			if (scn->margin == 2) {
				// Click on the folder margin. Toggle the current line if possible.
				long line = [self getGeneralProperty: SCI_LINEFROMPOSITION parameter: scn->position];
				[self setGeneralProperty: SCI_TOGGLEFOLD value: line];
			}
			break;
		};
	case Notification::Modified: {
			// Decide depending on the modification type what to do.
			// There can be more than one modification carried by one notification.
			if (scn->modificationType &
				static_cast<int>((ModificationFlags::InsertText | ModificationFlags::DeleteText)))
				[self sendNotification: NSTextDidChangeNotification];
			break;
		}
	case Notification::Zoom: {
			// A zoom change happened. Notify info bar if there is one.
			float zoom = [self getGeneralProperty: SCI_GETZOOM parameter: 0];
			long fontSize = [self getGeneralProperty: SCI_STYLEGETSIZE parameter: STYLE_DEFAULT];
			float factor = (zoom / fontSize) + 1;
			[mInfoBar notify: IBNZoomChanged message: nil location: NSZeroPoint value: factor];
			break;
		}
	case Notification::UpdateUI: {
			// Triggered whenever changes in the UI state need to be reflected.
			// These can be: caret changes, selection changes etc.
			NSPoint caretPosition = mBackend->GetCaretPosition();
			[mInfoBar notify: IBNCaretChanged message: nil location: caretPosition value: 0];
			[self sendNotification: SCIUpdateUINotification];
			if (scn->updated & static_cast<int>((Update::Selection | Update::Content))) {
				[self sendNotification: NSTextViewDidChangeSelectionNotification];
			}
			break;
		}
	case Notification::FocusOut:
		[self sendNotification: NSTextDidEndEditingNotification];
		break;
	case Notification::FocusIn: // Nothing to do for now.
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Setup a special indicator used in the editor to provide visual feedback for
 * input composition, depending on language, keyboard etc.
 */
- (void) updateIndicatorIME {
	[self setColorProperty: SCI_INDICSETFORE parameter: INDICATOR_IME fromHTML: @"#FF0000"];
	const bool drawInBackground = [self message: SCI_GETPHASESDRAW] != 0;
	[self setGeneralProperty: SCI_INDICSETUNDER parameter: INDICATOR_IME value: drawInBackground];
	[self setGeneralProperty: SCI_INDICSETSTYLE parameter: INDICATOR_IME value: INDIC_PLAIN];
	[self setGeneralProperty: SCI_INDICSETALPHA parameter: INDICATOR_IME value: 100];
}

//--------------------------------------------------------------------------------------------------

/**
 * Initialization of the view. Used to setup a few other things we need.
 */
- (instancetype) initWithFrame: (NSRect) frame {
	self = [super initWithFrame: frame];
	if (self) {
		mContent = [[[[self class] contentViewClass] alloc] initWithFrame: NSZeroRect];
		mContent.owner = self;

		// Initialize the scrollers but don't show them yet.
		// Pick an arbitrary size, just to make NSScroller selecting the proper scroller direction
		// (horizontal or vertical).
		NSRect scrollerRect = NSMakeRect(0, 0, 100, 10);
		scrollView = (NSScrollView *)[[SCIScrollView alloc] initWithFrame: scrollerRect];
#if defined(MAC_OS_X_VERSION_10_14)
		// Let SCIScrollView account for other subviews such as vertical ruler by turning off
		// automaticallyAdjustsContentInsets.
		if (std::floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_13) {
			scrollView.contentView.automaticallyAdjustsContentInsets = NO;
			scrollView.contentView.contentInsets = NSEdgeInsetsMake(0., 0., 0., 0.);
		}
#endif
		scrollView.documentView = mContent;
		[scrollView setHasVerticalScroller: YES];
		[scrollView setHasHorizontalScroller: YES];
		scrollView.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;
		//[scrollView setScrollerStyle:NSScrollerStyleLegacy];
		//[scrollView setScrollerKnobStyle:NSScrollerKnobStyleDark];
		//[scrollView setHorizontalScrollElasticity:NSScrollElasticityNone];
		[self addSubview: scrollView];

		marginView = [[SCIMarginView alloc] initWithScrollView: scrollView];
		marginView.owner = self;
		marginView.ruleThickness = marginView.requiredThickness;
		scrollView.verticalRulerView = marginView;
		[scrollView setHasHorizontalRuler: NO];
		[scrollView setHasVerticalRuler: YES];
		[scrollView setRulersVisible: YES];

		mBackend = new ScintillaCocoa(self, mContent, marginView);

		// Establish a connection from the back end to this container so we can handle situations
		// which require our attention.
		mBackend->SetDelegate(self);

		[self updateIndicatorIME];

		NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
		[center addObserver: self
			   selector: @selector(applicationDidResignActive:)
			       name: NSApplicationDidResignActiveNotification
			     object: nil];

		[center addObserver: self
			   selector: @selector(applicationDidBecomeActive:)
			       name: NSApplicationDidBecomeActiveNotification
			     object: nil];

		[center addObserver: self
			   selector: @selector(windowWillMove:)
			       name: NSWindowWillMoveNotification
			     object: self.window];

		[center addObserver: self
			   selector: @selector(defaultsDidChange:)
			       name: NSSystemColorsDidChangeNotification
			     object: self.window];

		[scrollView.contentView setPostsBoundsChangedNotifications: YES];
		[center addObserver: self
			   selector: @selector(scrollerAction:)
			       name: NSViewBoundsDidChangeNotification
			     object: scrollView.contentView];

		mBackend->UpdateBaseElements();
	}
	return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	mBackend->Finalise();
	delete mBackend;
	mBackend = NULL;
	mContent.owner = nil;
	[marginView setClientView: nil];
	[scrollView removeFromSuperview];
}

//--------------------------------------------------------------------------------------------------

- (void) applicationDidResignActive: (NSNotification *) note {
#pragma unused(note)
	mBackend->ActiveStateChanged(false);
}

//--------------------------------------------------------------------------------------------------

- (void) applicationDidBecomeActive: (NSNotification *) note {
#pragma unused(note)
	mBackend->ActiveStateChanged(true);
}

//--------------------------------------------------------------------------------------------------

- (void) windowWillMove: (NSNotification *) note {
#pragma unused(note)
	mBackend->WindowWillMove();
}

//--------------------------------------------------------------------------------------------------

- (void) defaultsDidChange: (NSNotification *) note {
#pragma unused(note)
	mBackend->UpdateBaseElements();
}

//--------------------------------------------------------------------------------------------------

- (void) viewDidMoveToWindow {
	[super viewDidMoveToWindow];

	[self positionSubViews];

	// Enable also mouse move events for our window (and so this view).
	[self.window setAcceptsMouseMovedEvents: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to position and size the parts of the editor (content, scrollers, info bar).
 */
- (void) positionSubViews {
	CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize: NSControlSizeRegular
							  scrollerStyle: NSScrollerStyleLegacy];

	NSSize size = self.frame.size;
	NSRect barFrame = {{0, size.height - scrollerWidth}, {size.width, scrollerWidth}};
	BOOL infoBarVisible = mInfoBar != nil && !mInfoBar.hidden;

	// Horizontal offset of the content. Almost always 0 unless the vertical scroller
	// is on the left side.
	CGFloat contentX = 0;
	NSRect scrollRect = {{contentX, 0}, {size.width, size.height}};

	// Info bar frame.
	if (infoBarVisible) {
		scrollRect.size.height -= scrollerWidth;
		// Initial value already is as if the bar is at top.
		if (!mInfoBarAtTop) {
			scrollRect.origin.y += scrollerWidth;
			barFrame.origin.y = 0;
		}
	}

	if (!NSEqualRects(scrollView.frame, scrollRect)) {
		scrollView.frame = scrollRect;
	}

	if (infoBarVisible)
		mInfoBar.frame = barFrame;
}

//--------------------------------------------------------------------------------------------------

/**
 * Set the width of the margin.
 */
- (void) setMarginWidth: (int) width {
	if (marginView.ruleThickness != width) {
		marginView.marginWidth = width;
		marginView.ruleThickness = marginView.requiredThickness;
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered by one of the scrollers when it gets manipulated by the user. Notify the backend
 * about the change.
 */
- (void) scrollerAction: (id) sender {
#pragma unused(sender)
	mBackend->UpdateForScroll();
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to reposition our content depending on the size of the view.
 */
- (void) setFrame: (NSRect) newFrame {
	NSRect previousFrame = self.frame;
	super.frame = newFrame;
	[self positionSubViews];
	if (!NSEqualRects(previousFrame, newFrame)) {
		mBackend->Resize();
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Getter for the currently selected text in raw form (no formatting information included).
 * If there is no text available an empty string is returned.
 */
- (NSString *) selectedString {
	NSString *result = @"";

	const long length = mBackend->WndProc(Message::GetSelText, 0, 0);
	if (length > 0) {
		std::string buffer(length + 1, '\0');
		try {
			mBackend->WndProc(Message::GetSelText, length + 1, (sptr_t) &buffer[0]);

			result = @(buffer.c_str());
		} catch (...) {
		}
	}

	return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Delete a range from the document.
 */
- (void) deleteRange: (NSRange) aRange {
	if (aRange.length > 0) {
		NSRange posRange = mBackend->PositionsFromCharacters(aRange);
		[self message: SCI_DELETERANGE wParam: posRange.location lParam: posRange.length];
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Getter for the current text in raw form (no formatting information included).
 * If there is no text available an empty string is returned.
 */
- (NSString *) string {
	NSString *result = @"";

	const long length = mBackend->WndProc(Message::GetLength, 0, 0);
	if (length > 0) {
		std::string buffer(length + 1, '\0');
		try {
			mBackend->WndProc(Message::GetText, length + 1, (sptr_t) &buffer[0]);

			result = @(buffer.c_str());
		} catch (...) {
		}
	}

	return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Setter for the current text (no formatting included).
 */
- (void) setString: (NSString *) aString {
	const char *text = aString.UTF8String;
	mBackend->WndProc(Message::SetText, 0, (long) text);
}

//--------------------------------------------------------------------------------------------------

- (void) insertString: (NSString *) aString atOffset: (int) offset {
	const char *text = aString.UTF8String;
	mBackend->WndProc(Message::AddText, offset, (long) text);
}

//--------------------------------------------------------------------------------------------------

- (void) setEditable: (BOOL) editable {
	mBackend->WndProc(Message::SetReadOnly, editable ? 0 : 1, 0);
}

//--------------------------------------------------------------------------------------------------

- (BOOL) isEditable {
	return mBackend->WndProc(Message::GetReadOnly, 0, 0) == 0;
}

//--------------------------------------------------------------------------------------------------

- (SCIContentView *) content {
	return mContent;
}

//--------------------------------------------------------------------------------------------------

- (void) updateMarginCursors {
	[self.window invalidateCursorRectsForView: marginView];
}

//--------------------------------------------------------------------------------------------------

/**
 * Direct call into the backend to allow uninterpreted access to it. The values to be passed in and
 * the result heavily depend on the message that is used for the call. Refer to the Scintilla
 * documentation to learn what can be used here.
 */
+ (sptr_t) directCall: (ScintillaView *) sender message: (unsigned int) message wParam: (uptr_t) wParam
	       lParam: (sptr_t) lParam {
	return ScintillaCocoa::DirectFunction(
		       reinterpret_cast<sptr_t>(sender->mBackend), message, wParam, lParam);
}

- (sptr_t) message: (unsigned int) message wParam: (uptr_t) wParam lParam: (sptr_t) lParam {
	return mBackend->WndProc(static_cast<Message>(message), wParam, lParam);
}

- (sptr_t) message: (unsigned int) message wParam: (uptr_t) wParam {
	return mBackend->WndProc(static_cast<Message>(message), wParam, 0);
}

- (sptr_t) message: (unsigned int) message {
	return mBackend->WndProc(static_cast<Message>(message), 0, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * This is a helper method to set properties in the backend, with native parameters.
 *
 * @param property Main property like SCI_STYLESETFORE for which a value is to be set.
 * @param parameter Additional info for this property like a parameter or index.
 * @param value The actual value. It depends on the property what this parameter means.
 */
- (void) setGeneralProperty: (int) property parameter: (long) parameter value: (long) value {
	mBackend->WndProc(static_cast<Message>(property), parameter, value);
}

//--------------------------------------------------------------------------------------------------

/**
 * A simplified version for setting properties which only require one parameter.
 *
 * @param property Main property like SCI_STYLESETFORE for which a value is to be set.
 * @param value The actual value. It depends on the property what this parameter means.
 */
- (void) setGeneralProperty: (int) property value: (long) value {
	mBackend->WndProc(static_cast<Message>(property), value, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * This is a helper method to get a property in the backend, with native parameters.
 *
 * @param property Main property like SCI_STYLESETFORE for which a value is to get.
 * @param parameter Additional info for this property like a parameter or index.
 * @param extra Yet another parameter if needed.
 * @result A generic value which must be interpreted depending on the property queried.
 */
- (long) getGeneralProperty: (int) property parameter: (long) parameter extra: (long) extra {
	return mBackend->WndProc(static_cast<Message>(property), parameter, extra);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convenience function to avoid unneeded extra parameter.
 */
- (long) getGeneralProperty: (int) property parameter: (long) parameter {
	return mBackend->WndProc(static_cast<Message>(property), parameter, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convenience function to avoid unneeded parameters.
 */
- (long) getGeneralProperty: (int) property {
	return mBackend->WndProc(static_cast<Message>(property), 0, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Use this variant if you have to pass in a reference to something (e.g. a text range).
 */
- (long) getGeneralProperty: (int) property ref: (const void *) ref {
	return mBackend->WndProc(static_cast<Message>(property), 0, (sptr_t) ref);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for colors.
 */
- (void) setColorProperty: (int) property parameter: (long) parameter value: (NSColor *) value {
	if (value.colorSpaceName != NSDeviceRGBColorSpace)
		value = [value colorUsingColorSpaceName: NSDeviceRGBColorSpace];
	long red = static_cast<long>(value.redComponent * 255);
	long green = static_cast<long>(value.greenComponent * 255);
	long blue = static_cast<long>(value.blueComponent * 255);

	long color = (blue << 16) + (green << 8) + red;
	mBackend->WndProc(static_cast<Message>(property), parameter, color);
}

//--------------------------------------------------------------------------------------------------

/**
 * Another color property setting, which allows to specify the color as string like in HTML
 * documents (i.e. with leading # and either 3 hex digits or 6).
 */
- (void) setColorProperty: (int) property parameter: (long) parameter fromHTML: (NSString *) fromHTML {
	if (fromHTML.length > 3 && [fromHTML characterAtIndex: 0] == '#') {
		bool longVersion = fromHTML.length > 6;
		int index = 1;

		char value[3] = {0, 0, 0};
		value[0] = static_cast<char>([fromHTML characterAtIndex: index++]);
		if (longVersion)
			value[1] = static_cast<char>([fromHTML characterAtIndex: index++]);
		else
			value[1] = value[0];

		unsigned rawRed;
		[[NSScanner scannerWithString: @(value)] scanHexInt: &rawRed];

		value[0] = static_cast<char>([fromHTML characterAtIndex: index++]);
		if (longVersion)
			value[1] = static_cast<char>([fromHTML characterAtIndex: index++]);
		else
			value[1] = value[0];

		unsigned rawGreen;
		[[NSScanner scannerWithString: @(value)] scanHexInt: &rawGreen];

		value[0] = static_cast<char>([fromHTML characterAtIndex: index++]);
		if (longVersion)
			value[1] = static_cast<char>([fromHTML characterAtIndex: index++]);
		else
			value[1] = value[0];

		unsigned rawBlue;
		[[NSScanner scannerWithString: @(value)] scanHexInt: &rawBlue];

		long color = (rawBlue << 16) + (rawGreen << 8) + rawRed;
		mBackend->WndProc(static_cast<Message>(property), parameter, color);
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for colors.
 */
- (NSColor *) getColorProperty: (int) property parameter: (long) parameter {
	long color = mBackend->WndProc(static_cast<Message>(property), parameter, 0);
	CGFloat red = (color & 0xFF) / 255.0;
	CGFloat green = ((color >> 8) & 0xFF) / 255.0;
	CGFloat blue = ((color >> 16) & 0xFF) / 255.0;
	NSColor *result = [NSColor colorWithDeviceRed: red green: green blue: blue alpha: 1];
	return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for references (pointers, addresses).
 */
- (void) setReferenceProperty: (int) property parameter: (long) parameter value: (const void *) value {
	mBackend->WndProc(static_cast<Message>(property), parameter, (sptr_t) value);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for references (pointers, addresses).
 */
- (const void *) getReferenceProperty: (int) property parameter: (long) parameter {
	return (const void *) mBackend->WndProc(static_cast<Message>(property), parameter, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for string values.
 */
- (void) setStringProperty: (int) property parameter: (long) parameter value: (NSString *) value {
	const char *rawValue = value.UTF8String;
	mBackend->WndProc(static_cast<Message>(property), parameter, (sptr_t) rawValue);
}


//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for string values.
 */
- (NSString *) getStringProperty: (int) property parameter: (long) parameter {
	const char *rawValue = (const char *) mBackend->WndProc(static_cast<Message>(property), parameter, 0);
	return @(rawValue);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for lexer properties, which are commonly passed as strings.
 */
- (void) setLexerProperty: (NSString *) name value: (NSString *) value {
	const char *rawName = name.UTF8String;
	const char *rawValue = value.UTF8String;
	mBackend->WndProc(Message::SetProperty, (sptr_t) rawName, (sptr_t) rawValue);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for references (pointers, addresses).
 */
- (NSString *) getLexerProperty: (NSString *) name {
	const char *rawName = name.UTF8String;
	const char *result = (const char *) mBackend->WndProc(Message::SetProperty, (sptr_t) rawName, 0);
	return @(result);
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the notification callback
 */
- (void) registerNotifyCallback: (intptr_t) windowid value: (SciNotifyFunc) callback {
	mBackend->RegisterNotifyCallback(windowid, callback);
}


//--------------------------------------------------------------------------------------------------

/**
 * Sets the new control which is displayed as info bar at the top or bottom of the editor.
 * Set newBar to nil if you want to hide the bar again.
 * The info bar's height is set to the height of the scrollbar.
 */
- (void) setInfoBar: (NSView <InfoBarCommunicator> *) newBar top: (BOOL) top {
	if (mInfoBar != newBar) {
		[mInfoBar removeFromSuperview];

		mInfoBar = newBar;
		mInfoBarAtTop = top;
		if (mInfoBar != nil) {
			[self addSubview: mInfoBar];
			[mInfoBar setCallback: self];
		}

		[self positionSubViews];
	}
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the edit's info bar status message. This call only has an effect if there is an info bar.
 */
- (void) setStatusText: (NSString *) text {
	if (mInfoBar != nil)
		[mInfoBar notify: IBNStatusChanged message: text location: NSZeroPoint value: 0];
}

//--------------------------------------------------------------------------------------------------

- (NSRange) selectedRange {
	return [mContent selectedRange];
}

//--------------------------------------------------------------------------------------------------

/**
 * Return the main selection as an NSRange of positions (not characters).
 * Unlike selectedRange, this can return empty ranges inside the document.
 */

- (NSRange) selectedRangePositions {
	const sptr_t positionBegin = [self message: SCI_GETSELECTIONSTART];
	const sptr_t positionEnd = [self message: SCI_GETSELECTIONEND];
	return NSMakeRange(positionBegin, positionEnd-positionBegin);
}


//--------------------------------------------------------------------------------------------------

- (void) insertText: (id) aString {
	if ([aString isKindOfClass: [NSString class]])
		mBackend->InsertText(aString, CharacterSource::DirectInput);
	else if ([aString isKindOfClass: [NSAttributedString class]])
		mBackend->InsertText([aString string], CharacterSource::DirectInput);
}

//--------------------------------------------------------------------------------------------------

/**
 * For backwards compatibility.
 */
- (BOOL) findAndHighlightText: (NSString *) searchText
		    matchCase: (BOOL) matchCase
		    wholeWord: (BOOL) wholeWord
		     scrollTo: (BOOL) scrollTo
			 wrap: (BOOL) wrap {
	return [self findAndHighlightText: searchText
				matchCase: matchCase
				wholeWord: wholeWord
				 scrollTo: scrollTo
				     wrap: wrap
				backwards: NO];
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches and marks the first occurrence of the given text and optionally scrolls it into view.
 *
 * @result YES if something was found, NO otherwise.
 */
- (BOOL) findAndHighlightText: (NSString *) searchText
		    matchCase: (BOOL) matchCase
		    wholeWord: (BOOL) wholeWord
		     scrollTo: (BOOL) scrollTo
			 wrap: (BOOL) wrap
		    backwards: (BOOL) backwards {
	FindOption searchFlags = FindOption::None;
	if (matchCase)
		searchFlags = searchFlags | FindOption::MatchCase;
	if (wholeWord)
		searchFlags = searchFlags | FindOption::WholeWord;

	long selectionStart = [self getGeneralProperty: SCI_GETSELECTIONSTART parameter: 0];
	long selectionEnd = [self getGeneralProperty: SCI_GETSELECTIONEND parameter: 0];

	// Sets the start point for the coming search to the beginning of the current selection.
	// For forward searches we have therefore to set the selection start to the current selection end
	// for proper incremental search. This does not harm as we either get a new selection if something
	// is found or the previous selection is restored.
	if (!backwards)
		[self getGeneralProperty: SCI_SETSELECTIONSTART parameter: selectionEnd];
	[self setGeneralProperty: SCI_SEARCHANCHOR value: 0];
	sptr_t result;
	const char *textToSearch = searchText.UTF8String;

	// The following call will also set the selection if something was found.
	if (backwards) {
		result = [ScintillaView directCall: self
					   message: SCI_SEARCHPREV
					    wParam: (uptr_t) searchFlags
					    lParam: (sptr_t) textToSearch];
		if (result < 0 && wrap) {
			// Try again from the end of the document if nothing could be found so far and
			// wrapped search is set.
			[self getGeneralProperty: SCI_SETSELECTIONSTART parameter: [self getGeneralProperty: SCI_GETTEXTLENGTH parameter: 0]];
			[self setGeneralProperty: SCI_SEARCHANCHOR value: 0];
			result = [ScintillaView directCall: self
						   message: SCI_SEARCHNEXT
						    wParam: (uptr_t) searchFlags
						    lParam: (sptr_t) textToSearch];
		}
	} else {
		result = [ScintillaView directCall: self
					   message: SCI_SEARCHNEXT
					    wParam: (uptr_t) searchFlags
					    lParam: (sptr_t) textToSearch];
		if (result < 0 && wrap) {
			// Try again from the start of the document if nothing could be found so far and
			// wrapped search is set.
			[self getGeneralProperty: SCI_SETSELECTIONSTART parameter: 0];
			[self setGeneralProperty: SCI_SEARCHANCHOR value: 0];
			result = [ScintillaView directCall: self
						   message: SCI_SEARCHNEXT
						    wParam: (uptr_t) searchFlags
						    lParam: (sptr_t) textToSearch];
		}
	}

	if (result >= 0) {
		if (scrollTo)
			[self setGeneralProperty: SCI_SCROLLCARET value: 0];
	} else {
		// Restore the former selection if we did not find anything.
		[self setGeneralProperty: SCI_SETSELECTIONSTART value: selectionStart];
		[self setGeneralProperty: SCI_SETSELECTIONEND value: selectionEnd];
	}
	return (result >= 0) ? YES : NO;
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches the given text and replaces
 *
 * @result Number of entries replaced, 0 if none.
 */
- (int) findAndReplaceText: (NSString *) searchText
		    byText: (NSString *) newText
		 matchCase: (BOOL) matchCase
		 wholeWord: (BOOL) wholeWord
		     doAll: (BOOL) doAll {
	// The current position is where we start searching for single occurrences. Otherwise we start at
	// the beginning of the document.
	long startPosition;
	if (doAll)
		startPosition = 0; // Start at the beginning of the text if we replace all occurrences.
	else
		// For a single replacement we start at the current caret position.
		startPosition = [self getGeneralProperty: SCI_GETCURRENTPOS];
	long endPosition = [self getGeneralProperty: SCI_GETTEXTLENGTH];

	FindOption searchFlags = FindOption::None;
	if (matchCase)
		searchFlags = searchFlags | FindOption::MatchCase;
	if (wholeWord)
		searchFlags = searchFlags | FindOption::WholeWord;
	[self setGeneralProperty: SCI_SETSEARCHFLAGS value: (long)searchFlags];
	[self setGeneralProperty: SCI_SETTARGETSTART value: startPosition];
	[self setGeneralProperty: SCI_SETTARGETEND value: endPosition];

	const char *textToSearch = searchText.UTF8String;
	long sourceLength = strlen(textToSearch); // Length in bytes.
	const char *replacement = newText.UTF8String;
	long targetLength = strlen(replacement);  // Length in bytes.
	sptr_t result;

	int replaceCount = 0;
	if (doAll) {
		while (true) {
			result = [ScintillaView directCall: self
						   message: SCI_SEARCHINTARGET
						    wParam: sourceLength
						    lParam: (sptr_t) textToSearch];
			if (result < 0)
				break;

			replaceCount++;
			[ScintillaView directCall: self
					  message: SCI_REPLACETARGET
					   wParam: targetLength
					   lParam: (sptr_t) replacement];

			// The replacement changes the target range to the replaced text. Continue after that till the end.
			// The text length might be changed by the replacement so make sure the target end is the actual
			// text end.
			[self setGeneralProperty: SCI_SETTARGETSTART value: [self getGeneralProperty: SCI_GETTARGETEND]];
			[self setGeneralProperty: SCI_SETTARGETEND value: [self getGeneralProperty: SCI_GETTEXTLENGTH]];
		}
	} else {
		result = [ScintillaView directCall: self
					   message: SCI_SEARCHINTARGET
					    wParam: sourceLength
					    lParam: (sptr_t) textToSearch];
		replaceCount = (result < 0) ? 0 : 1;

		if (replaceCount > 0) {
			[ScintillaView directCall: self
					  message: SCI_REPLACETARGET
					   wParam: targetLength
					   lParam: (sptr_t) replacement];

			// For a single replace we set the new selection to the replaced text.
			[self setGeneralProperty: SCI_SETSELECTIONSTART value: [self getGeneralProperty: SCI_GETTARGETSTART]];
			[self setGeneralProperty: SCI_SETSELECTIONEND value: [self getGeneralProperty: SCI_GETTARGETEND]];
		}
	}

	return replaceCount;
}

//--------------------------------------------------------------------------------------------------

- (void) setFontName: (NSString *) font
		size: (int) size
		bold: (BOOL) bold
	      italic: (BOOL) italic {
	for (int i = 0; i < 128; i++) {
		[self setGeneralProperty: SCI_STYLESETFONT
			       parameter: i
				   value: (sptr_t)font.UTF8String];
		[self setGeneralProperty: SCI_STYLESETSIZE
			       parameter: i
				   value: size];
		[self setGeneralProperty: SCI_STYLESETBOLD
			       parameter: i
				   value: bold];
		[self setGeneralProperty: SCI_STYLESETITALIC
			       parameter: i
				   value: italic];
	}
}

//--------------------------------------------------------------------------------------------------

@end

