
/**
 * Implementation of the native Cocoa View that serves as container for the scintilla parts.
 *
 * Created by Mike Lischke.
 *
 * Copyright 2011, 2013, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2009, 2011 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import "Platform.h"
#import "ScintillaView.h"
#import "ScintillaCocoa.h"

using namespace Scintilla;

// Two additional cursors we need, which aren't provided by Cocoa.
static NSCursor* reverseArrowCursor;
static NSCursor* waitCursor;

NSString *const SCIUpdateUINotification = @"SCIUpdateUI";

/**
 * Provide an NSCursor object that matches the Window::Cursor enumeration.
 */
static NSCursor *cursorFromEnum(Window::Cursor cursor)
{
  switch (cursor)
  {
    case Window::cursorText:
      return [NSCursor IBeamCursor];
    case Window::cursorArrow:
      return [NSCursor arrowCursor];
    case Window::cursorWait:
      return waitCursor;
    case Window::cursorHoriz:
      return [NSCursor resizeLeftRightCursor];
    case Window::cursorVert:
      return [NSCursor resizeUpDownCursor];
    case Window::cursorReverseArrow:
      return reverseArrowCursor;
    case Window::cursorUp:
    default:
      return [NSCursor arrowCursor];
  }
}


@implementation SCIMarginView

@synthesize marginWidth, owner;

- (id)initWithScrollView:(NSScrollView *)aScrollView
{
  self = [super initWithScrollView:aScrollView orientation:NSVerticalRuler];
  if (self != nil)
  {
    owner = nil;
    marginWidth = 20;
    currentCursors = [[NSMutableArray arrayWithCapacity:0] retain];
    for (size_t i=0; i<=SC_MAX_MARGIN; i++)
    {
      [currentCursors addObject: [reverseArrowCursor retain]];
    }
    [self setClientView:[aScrollView documentView]];
  }
  return self;
}

- (void) dealloc
{
  [currentCursors release];
  [super dealloc];
}

- (void) setFrame: (NSRect) frame
{
  [super setFrame: frame];

  [[self window] invalidateCursorRectsForView: self];
}

- (CGFloat)requiredThickness
{
  return marginWidth;
}

- (void)drawHashMarksAndLabelsInRect:(NSRect)aRect
{
  if (owner) {
    NSRect contentRect = [[[self scrollView] contentView] bounds];
    NSRect marginRect = [self bounds];
    // Ensure paint to bottom of view to avoid glitches
    if (marginRect.size.height > contentRect.size.height) {
      // Legacy scroll bar mode leaves a poorly painted corner
      aRect = marginRect;
    }
    owner.backend->PaintMargin(aRect);
  }
}

- (void) mouseDown: (NSEvent *) theEvent
{
  NSClipView *textView = [[self scrollView] contentView];
  [[textView window] makeFirstResponder:textView];
  owner.backend->MouseDown(theEvent);
}

- (void) mouseDragged: (NSEvent *) theEvent
{
  owner.backend->MouseMove(theEvent);
}

- (void) mouseMoved: (NSEvent *) theEvent
{
  owner.backend->MouseMove(theEvent);
}

- (void) mouseUp: (NSEvent *) theEvent
{
  owner.backend->MouseUp(theEvent);
}

/**
 * This method is called to give us the opportunity to define our mouse sensitive rectangle.
 */
- (void) resetCursorRects
{
  [super resetCursorRects];

  int x = 0;
  NSRect marginRect = [self bounds];
  size_t co = [currentCursors count];
  for (size_t i=0; i<co; i++)
  {
    long cursType = owner.backend->WndProc(SCI_GETMARGINCURSORN, i, 0);
    long width =owner.backend->WndProc(SCI_GETMARGINWIDTHN, i, 0);
    NSCursor *cc = cursorFromEnum(static_cast<Window::Cursor>(cursType));
    [currentCursors replaceObjectAtIndex:i withObject: cc];
    marginRect.origin.x = x;
    marginRect.size.width = width;
    [self addCursorRect: marginRect cursor: cc];
    [cc setOnMouseEntered: YES];
    x += width;
  }
}

@end

@implementation SCIContentView

@synthesize owner = mOwner;

//--------------------------------------------------------------------------------------------------

- (NSView*) initWithFrame: (NSRect) frame
{
  self = [super initWithFrame: frame];

  if (self != nil)
  {
    // Some initialization for our view.
    mCurrentCursor = [[NSCursor arrowCursor] retain];
    trackingArea = nil;
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);

    [self registerForDraggedTypes: [NSArray arrayWithObjects:
                                   NSStringPboardType, ScintillaRecPboardType, NSFilenamesPboardType, nil]];
  }

  return self;
}

//--------------------------------------------------------------------------------------------------

/**
 * When the view is resized or scrolled we need to update our tracking area.
 */
- (void) updateTrackingAreas
{
  if (trackingArea)
    [self removeTrackingArea:trackingArea];
  
  int opts = (NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);
  trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                              options:opts
                                                owner:self
                                             userInfo:nil];
  [self addTrackingArea: trackingArea];
  [super updateTrackingAreas];
}

//--------------------------------------------------------------------------------------------------

/**
 * When the view is resized we need to let the backend know.
 */
- (void) setFrame: (NSRect) frame
{
  [super setFrame: frame];
	
  mOwner.backend->Resize();
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the backend if a new cursor must be set for the view.
 */
- (void) setCursor: (int) cursor
{
  Window::Cursor eCursor = (Window::Cursor)cursor;
  [mCurrentCursor autorelease];
  mCurrentCursor = cursorFromEnum(eCursor);
  [mCurrentCursor retain];

  // Trigger recreation of the cursor rectangle(s).
  [[self window] invalidateCursorRectsForView: self];
  [mOwner updateMarginCursors];
}

//--------------------------------------------------------------------------------------------------

/**
 * This method is called to give us the opportunity to define our mouse sensitive rectangle.
 */
- (void) resetCursorRects
{
  [super resetCursorRects];

  // We only have one cursor rect: our bounds.
  [self addCursorRect: [self bounds] cursor: mCurrentCursor];
  [mCurrentCursor setOnMouseEntered: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called before repainting.
 */
- (void) viewWillDraw
{
  const NSRect *rects;
  NSInteger nRects = 0;
  [self getRectsBeingDrawn:&rects count:&nRects];
  if (nRects > 0) {
    NSRect rectUnion = rects[0];
    for (int i=0;i<nRects;i++) {
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
- (void) prepareContentInRect: (NSRect) rect
{
  mOwner.backend->WillDraw(rect);
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1080
  [super prepareContentInRect: rect];
#endif
}

//--------------------------------------------------------------------------------------------------

/**
 * Gets called by the runtime when the view needs repainting.
 */
- (void) drawRect: (NSRect) rect
{
  CGContextRef context = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];

  if (!mOwner.backend->Draw(rect, context)) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self setNeedsDisplay:YES];
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
- (BOOL) isFlipped
{
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) isOpaque
{
  return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Implement the "click through" behavior by telling the caller we accept the first mouse event too.
 */
- (BOOL) acceptsFirstMouse: (NSEvent *) theEvent
{
#pragma unused(theEvent)
  return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Make this view accepting events as first responder.
 */
- (BOOL) acceptsFirstResponder
{
  return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the framework if it wants to show a context menu for the editor.
 */
- (NSMenu*) menuForEvent: (NSEvent*) theEvent
{
  if (![mOwner respondsToSelector: @selector(menuForEvent:)])
    return mOwner.backend->CreateContextMenu(theEvent);
  else
    return [mOwner menuForEvent: theEvent];
}

//--------------------------------------------------------------------------------------------------

// Adoption of NSTextInputClient protocol.

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
  const NSRange posRange = mOwner.backend->PositionsFromCharacters(aRange);
  // The backend validated aRange and may have removed characters beyond the end of the document.
  const NSRange charRange = mOwner.backend->CharactersFromPositions(posRange);
  if (!NSEqualRanges(aRange, charRange))
  {
    *actualRange = charRange;
  }

  [mOwner message: SCI_SETTARGETRANGE wParam: posRange.location lParam: NSMaxRange(posRange)];
  std::string text([mOwner message: SCI_TARGETASUTF8] + 1, 0);
  [mOwner message: SCI_TARGETASUTF8 wParam: 0 lParam: reinterpret_cast<sptr_t>(&text[0])];
  NSString *result = [NSString stringWithUTF8String: text.c_str()];
  NSMutableAttributedString *asResult = [[[NSMutableAttributedString alloc] initWithString:result] autorelease];

  const NSRange rangeAS = NSMakeRange(0, [asResult length]);
  const long style = [mOwner message: SCI_GETSTYLEAT wParam:posRange.location];
  std::string fontName([mOwner message: SCI_STYLEGETFONT wParam:style lParam:0] + 1, 0);
  [mOwner message: SCI_STYLEGETFONT wParam:style lParam:(sptr_t)&fontName[0]];
  const CGFloat fontSize = [mOwner message: SCI_STYLEGETSIZEFRACTIONAL wParam:style] / 100.0f;
  NSString *sFontName = [NSString stringWithUTF8String: fontName.c_str()];
  NSFont *font = [NSFont fontWithName:sFontName size:fontSize];
  [asResult addAttribute:NSFontAttributeName value:font range:rangeAS];

  return asResult;
}

//--------------------------------------------------------------------------------------------------

- (NSUInteger) characterIndexForPoint: (NSPoint) point
{
  const NSRect rectPoint = {point, NSZeroSize};
  const NSRect rectInWindow = [self.window convertRectFromScreen:rectPoint];
  const NSRect rectLocal = [[[self superview] superview] convertRect:rectInWindow fromView:nil];

  const long position = [mOwner message: SCI_CHARPOSITIONFROMPOINT
                                 wParam: rectLocal.origin.x
                                 lParam: rectLocal.origin.y];
  if (position == INVALID_POSITION)
  {
    return NSNotFound;
  }
  else
  {
    const NSRange index = mOwner.backend->CharactersFromPositions(NSMakeRange(position, 0));
    return index.location;
  }
}

//--------------------------------------------------------------------------------------------------

- (void) doCommandBySelector: (SEL) selector
{
  if ([self respondsToSelector: @selector(selector)])
    [self performSelector: selector withObject: nil];
}

//--------------------------------------------------------------------------------------------------

- (NSRect) firstRectForCharacterRange: (NSRange) aRange actualRange: (NSRangePointer) actualRange
{
  const NSRange posRange = mOwner.backend->PositionsFromCharacters(aRange);

  NSRect rect;
  rect.origin.x = [mOwner message: SCI_POINTXFROMPOSITION wParam: 0 lParam: posRange.location];
  rect.origin.y = [mOwner message: SCI_POINTYFROMPOSITION wParam: 0 lParam: posRange.location];
  const NSUInteger rangeEnd = NSMaxRange(posRange);
  rect.size.width = [mOwner message: SCI_POINTXFROMPOSITION wParam: 0 lParam: rangeEnd] - rect.origin.x;
  rect.size.height = [mOwner message: SCI_POINTYFROMPOSITION wParam: 0 lParam: rangeEnd] - rect.origin.y;
  rect.size.height += [mOwner message: SCI_TEXTHEIGHT wParam: 0 lParam: 0];
  const NSRect rectInWindow = [[[self superview] superview] convertRect:rect toView:nil];
  const NSRect rectScreen = [self.window convertRectToScreen:rectInWindow];

  return rectScreen;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) hasMarkedText
{
  return mMarkedTextRange.length > 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * General text input. Used to insert new text at the current input position, replacing the current
 * selection if there is any.
 * First removes the replacementRange.
 */
- (void) insertText: (id) aString replacementRange: (NSRange) replacementRange
{
	if ((mMarkedTextRange.location != NSNotFound) && (replacementRange.location != NSNotFound))
	{
		NSLog(@"Trying to insertText when there is both a marked range and a replacement range");
	}

	// Remove any previously marked text first.
	mOwner.backend->CompositionUndo();
	if (mMarkedTextRange.location != NSNotFound)
	{
		const NSRange posRangeMark = mOwner.backend->PositionsFromCharacters(mMarkedTextRange);
		[mOwner message: SCI_SETEMPTYSELECTION wParam: posRangeMark.location];
	}
	mMarkedTextRange = NSMakeRange(NSNotFound, 0);

	if (replacementRange.location == (NSNotFound-1))
		// This occurs when the accent popup is visible and menu selected.
		// Its replacing a non-existent position so do nothing.
		return;

	if (replacementRange.location != NSNotFound)
	{
		const NSRange posRangeReplacement = mOwner.backend->PositionsFromCharacters(replacementRange);
		[mOwner message: SCI_DELETERANGE
			 wParam: posRangeReplacement.location
			 lParam: posRangeReplacement.length];
		[mOwner message: SCI_SETEMPTYSELECTION wParam: posRangeReplacement.location];
	}

	NSString* newText = @"";
	if ([aString isKindOfClass:[NSString class]])
		newText = (NSString*) aString;
	else if ([aString isKindOfClass:[NSAttributedString class]])
		newText = (NSString*) [aString string];

	mOwner.backend->InsertText(newText);
}

//--------------------------------------------------------------------------------------------------

- (NSRange) markedRange
{
  return mMarkedTextRange;
}

//--------------------------------------------------------------------------------------------------

- (NSRange) selectedRange
{
  const long positionBegin = [mOwner message: SCI_GETSELECTIONSTART];
  const long positionEnd = [mOwner message: SCI_GETSELECTIONEND];
  NSRange posRangeSel = NSMakeRange(positionBegin, positionEnd-positionBegin);
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
- (void) setMarkedText: (id) aString selectedRange: (NSRange)range replacementRange: (NSRange)replacementRange
{
  NSString* newText = @"";
  if ([aString isKindOfClass:[NSString class]])
    newText = (NSString*) aString;
  else
    if ([aString isKindOfClass:[NSAttributedString class]])
      newText = (NSString*) [aString string];

  // Replace marked text if there is one.
  if (mMarkedTextRange.length > 0)
  {
    mOwner.backend->CompositionUndo();
    if (replacementRange.location != NSNotFound)
    {
      // This situation makes no sense and has not occurred in practice.
      NSLog(@"Can not handle a replacement range when there is also a marked range");
    }
    else
    {
      replacementRange = mMarkedTextRange;
      const NSRange posRangeMark = mOwner.backend->PositionsFromCharacters(mMarkedTextRange);
      [mOwner message: SCI_SETEMPTYSELECTION wParam: posRangeMark.location];
    }
  }
  else
  {
    // Must perform deletion before entering composition mode or else
    // both document and undo history will not contain the deleted text
    // leading to an inaccurate and unusable undo history.
    
    // Convert selection virtual space into real space
    mOwner.backend->ConvertSelectionVirtualSpace();

    if (replacementRange.location != NSNotFound)
    {
      const NSRange posRangeReplacement = mOwner.backend->PositionsFromCharacters(replacementRange);
      [mOwner message: SCI_DELETERANGE
               wParam: posRangeReplacement.location
               lParam: posRangeReplacement.length];
    }
    else  // No marked or replacement range, so replace selection
    {
      if (!mOwner.backend->ScintillaCocoa::ClearAllSelections()) {
        // Some of the selection is protected so can not perform composition here
        return;
      }
      // Ensure only a single selection.
      mOwner.backend->SelectOnlyMainSelection();
      replacementRange = [self selectedRange];
    }
  }

  // To support IME input to multiple selections, the following code would
  // need to insert newText at each selection, mark each piece of new text and then
  // select range relative to each insertion.

  if ([newText length])
  {
    // Switching into composition.
    mOwner.backend->CompositionStart();
    
    NSRange posRangeCurrent = mOwner.backend->PositionsFromCharacters(NSMakeRange(replacementRange.location, 0));
    // Note: Scintilla internally works almost always with bytes instead chars, so we need to take
    //       this into account when determining selection ranges and such.
    int lengthInserted = mOwner.backend->InsertText(newText);
    posRangeCurrent.length = lengthInserted;
    mMarkedTextRange = mOwner.backend->CharactersFromPositions(posRangeCurrent);
    // Mark the just inserted text. Keep the marked range for later reset.
    [mOwner setGeneralProperty: SCI_SETINDICATORCURRENT value: INDIC_IME];
    [mOwner setGeneralProperty: SCI_INDICATORFILLRANGE
                     parameter: posRangeCurrent.location
                         value: posRangeCurrent.length];
  }
  else
  {
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
    // Re-enable undo action collection if composition ended (indicated by an empty mark string).
    mOwner.backend->CompositionCommit();
  }

  // Select the part which is indicated in the given range. It does not scroll the caret into view.
  if (range.length > 0)
  {
    // range is in characters so convert to bytes for selection.
    range.location += replacementRange.location;
    NSRange posRangeSelect = mOwner.backend->PositionsFromCharacters(range);
    [mOwner setGeneralProperty: SCI_SETSELECTION parameter: NSMaxRange(posRangeSelect) value: posRangeSelect.location];
  }
}

//--------------------------------------------------------------------------------------------------

- (void) unmarkText
{
  if (mMarkedTextRange.length > 0)
  {
    mOwner.backend->CompositionCommit();
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
  }
}

//--------------------------------------------------------------------------------------------------

- (NSArray*) validAttributesForMarkedText
{
  return nil;
}

// End of the NSTextInputClient protocol adoption.

//--------------------------------------------------------------------------------------------------

/**
 * Generic input method. It is used to pass on keyboard input to Scintilla. The control itself only
 * handles shortcuts. The input is then forwarded to the Cocoa text input system, which in turn does
 * its own input handling (character composition via NSTextInputClient protocol):
 */
- (void) keyDown: (NSEvent *) theEvent
{
  if (mMarkedTextRange.length == 0)
	mOwner.backend->KeyboardInput(theEvent);
  NSArray* events = [NSArray arrayWithObject: theEvent];
  [self interpretKeyEvents: events];
}

//--------------------------------------------------------------------------------------------------

- (void) mouseDown: (NSEvent *) theEvent
{
  mOwner.backend->MouseDown(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseDragged: (NSEvent *) theEvent
{
  mOwner.backend->MouseMove(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseUp: (NSEvent *) theEvent
{
  mOwner.backend->MouseUp(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseMoved: (NSEvent *) theEvent
{
  mOwner.backend->MouseMove(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseEntered: (NSEvent *) theEvent
{
  mOwner.backend->MouseEntered(theEvent);
}

//--------------------------------------------------------------------------------------------------

- (void) mouseExited: (NSEvent *) theEvent
{
  mOwner.backend->MouseExited(theEvent);
}

//--------------------------------------------------------------------------------------------------

/**
 * Mouse wheel with command key magnifies text.
 * Enabling this code causes visual garbage to appear when scrolling
 * horizontally on OS X 10.9 with a retina display.
 * Pinch gestures and key commands can be used for magnification.
 */
#ifdef SCROLL_WHEEL_MAGNIFICATION
- (void) scrollWheel: (NSEvent *) theEvent
{
  if (([theEvent modifierFlags] & NSCommandKeyMask) != 0) {
    mOwner.backend->MouseWheel(theEvent);
  } else {
    [super scrollWheel:theEvent];
  }
}
#endif

//--------------------------------------------------------------------------------------------------

/**
 * Ensure scrolling is aligned to whole lines instead of starting part-way through a line
 */
- (NSRect)adjustScroll:(NSRect)proposedVisibleRect
{
  NSRect rc = proposedVisibleRect;
  // Snap to lines
  NSRect contentRect = [self bounds];
  if ((rc.origin.y > 0) && (NSMaxY(rc) < contentRect.size.height)) {
    // Only snap for positions inside the document - allow outside
    // for overshoot.
    long lineHeight = mOwner.backend->WndProc(SCI_TEXTHEIGHT, 0, 0);
    rc.origin.y = roundf(static_cast<XYPOSITION>(rc.origin.y) / lineHeight) * lineHeight;
  }
  return rc;
}

//--------------------------------------------------------------------------------------------------

/**
 * The editor is getting the foreground control (the one getting the input focus).
 */
- (BOOL) becomeFirstResponder
{
  mOwner.backend->WndProc(SCI_SETFOCUS, 1, 0);
  return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * The editor is losing the input focus.
 */
- (BOOL) resignFirstResponder
{
  mOwner.backend->WndProc(SCI_SETFOCUS, 0, 0);
  return YES;
}

//--------------------------------------------------------------------------------------------------

/**
 * Implement NSDraggingSource.
 */

- (NSDragOperation)draggingSession: (NSDraggingSession *) session
sourceOperationMaskForDraggingContext: (NSDraggingContext) context
{
  switch(context)
  {
    case NSDraggingContextOutsideApplication:
      return NSDragOperationCopy | NSDragOperationMove | NSDragOperationDelete;
      
    case NSDraggingContextWithinApplication:
    default:
      return NSDragOperationCopy | NSDragOperationMove | NSDragOperationDelete;
  }
}

- (void)draggingSession:(NSDraggingSession *)session
           movedToPoint:(NSPoint)screenPoint
{
}

- (void)draggingSession:(NSDraggingSession *)session
           endedAtPoint:(NSPoint)screenPoint
              operation:(NSDragOperation)operation
{
  if (operation == NSDragOperationDelete)
  {
    mOwner.backend->WndProc(SCI_CLEAR, 0, 0);
  }
}

/**
 * Implement NSDraggingDestination.
 */

//--------------------------------------------------------------------------------------------------

/**
 * Called when an external drag operation enters the view.
 */
- (NSDragOperation) draggingEntered: (id <NSDraggingInfo>) sender
{
  return mOwner.backend->DraggingEntered(sender);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called frequently during an external drag operation if we are the target.
 */
- (NSDragOperation) draggingUpdated: (id <NSDraggingInfo>) sender
{
  return mOwner.backend->DraggingUpdated(sender);
}

//--------------------------------------------------------------------------------------------------

/**
 * Drag image left the view. Clean up if necessary.
 */
- (void) draggingExited: (id <NSDraggingInfo>) sender
{
  mOwner.backend->DraggingExited(sender);
}

//--------------------------------------------------------------------------------------------------

- (BOOL) prepareForDragOperation: (id <NSDraggingInfo>) sender
{
#pragma unused(sender)
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) performDragOperation: (id <NSDraggingInfo>) sender
{
  return mOwner.backend->PerformDragOperation(sender);
}

//--------------------------------------------------------------------------------------------------

/**
 * Drag operation is done. Notify editor.
 */
- (void) concludeDragOperation: (id <NSDraggingInfo>) sender
{
  // Clean up is the same as if we are no longer the drag target.
  mOwner.backend->DraggingExited(sender);
}

//--------------------------------------------------------------------------------------------------

// NSResponder actions.

- (void) selectAll: (id) sender
{
#pragma unused(sender)
  mOwner.backend->SelectAll();
}

- (void) deleteBackward: (id) sender
{
#pragma unused(sender)
  mOwner.backend->DeleteBackward();
}

- (void) cut: (id) sender
{
#pragma unused(sender)
  mOwner.backend->Cut();
}

- (void) copy: (id) sender
{
#pragma unused(sender)
  mOwner.backend->Copy();
}

- (void) paste: (id) sender
{
#pragma unused(sender)
  if (mMarkedTextRange.location != NSNotFound)
  {
    [[NSTextInputContext currentInputContext] discardMarkedText];
    mOwner.backend->CompositionCommit();
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
  }
  mOwner.backend->Paste();
}

- (void) undo: (id) sender
{
#pragma unused(sender)
  if (mMarkedTextRange.location != NSNotFound)
  {
    [[NSTextInputContext currentInputContext] discardMarkedText];
    mOwner.backend->CompositionCommit();
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
  }
  mOwner.backend->Undo();
}

- (void) redo: (id) sender
{
#pragma unused(sender)
  mOwner.backend->Redo();
}

- (BOOL) canUndo
{
  return mOwner.backend->CanUndo() && (mMarkedTextRange.location == NSNotFound);
}

- (BOOL) canRedo
{
  return mOwner.backend->CanRedo();
}

- (BOOL) validateUserInterfaceItem: (id <NSValidatedUserInterfaceItem>) anItem
{
  SEL action = [anItem action];
  if (action==@selector(undo:)) {
    return [self canUndo];
  }
  else if (action==@selector(redo:)) {
    return [self canRedo];
  }
  else if (action==@selector(cut:) || action==@selector(copy:) || action==@selector(clear:)) {
    return mOwner.backend->HasSelection();
  }
  else if (action==@selector(paste:)) {
    return mOwner.backend->CanPaste();
  }
  return YES;
}

- (void) clear: (id) sender
{
  [self deleteBackward:sender];
}

- (BOOL) isEditable
{
  return mOwner.backend->WndProc(SCI_GETREADONLY, 0, 0) == 0;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  [mCurrentCursor release];
  [super dealloc];
}

@end

//--------------------------------------------------------------------------------------------------

@implementation ScintillaView

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
+ (void) initialize
{
  if (self == [ScintillaView class])
  {
    NSBundle* bundle = [NSBundle bundleForClass: [ScintillaView class]];

    NSString* path = [bundle pathForResource: @"mac_cursor_busy" ofType: @"tiff" inDirectory: nil];
    NSImage* image = [[[NSImage alloc] initWithContentsOfFile: path] autorelease];
    waitCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint(2, 2)];

    path = [bundle pathForResource: @"mac_cursor_flipped" ofType: @"tiff" inDirectory: nil];
    image = [[[NSImage alloc] initWithContentsOfFile: path] autorelease];
    reverseArrowCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint(12, 2)];
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Specify the SCIContentView class. Can be overridden in a subclass to provide an SCIContentView subclass.
 */

+ (Class) contentViewClass
{
  return [SCIContentView class];
}

//--------------------------------------------------------------------------------------------------

/**
 * Receives zoom messages, for example when a "pinch zoom" is performed on the trackpad.
 */
- (void) magnifyWithEvent: (NSEvent *) event
{
#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_5
  zoomDelta += event.magnification * 10.0;

  if (fabs(zoomDelta)>=1.0) {
    long zoomFactor = static_cast<long>([self getGeneralProperty: SCI_GETZOOM] + zoomDelta);
    [self setGeneralProperty: SCI_SETZOOM parameter: zoomFactor value:0];
    zoomDelta = 0.0;
  }
#endif
}

- (void) beginGestureWithEvent: (NSEvent *) event
{
// Scintilla is only interested in this event as the starft of a zoom
#pragma unused(event)
  zoomDelta = 0.0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Sends a new notification of the given type to the default notification center.
 */
- (void) sendNotification: (NSString*) notificationName
{
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
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
- (void) notify: (NotificationType) type message: (NSString*) message location: (NSPoint) location
          value: (float) value
{
// These parameters are just to conform to the protocol
#pragma unused(message)
#pragma unused(location)
  switch (type)
  {
    case IBNZoomChanged:
    {
      // Compute point increase/decrease based on default font size.
      long fontSize = [self getGeneralProperty: SCI_STYLEGETSIZE parameter: STYLE_DEFAULT];
      int zoom = (int) (fontSize * (value - 1));
      [self setGeneralProperty: SCI_SETZOOM value: zoom];
      break;
    }
    default:
      break;
  };
}

//--------------------------------------------------------------------------------------------------

- (void) setCallback: (id <InfoBarCommunicator>) callback
{
// Not used. Only here to satisfy protocol.
#pragma unused(callback)
}

//--------------------------------------------------------------------------------------------------

/**
 * Prevents drawing of the inner view to avoid flickering when doing many visual updates
 * (like clearing all marks and setting new ones etc.).
 */
- (void) suspendDrawing: (BOOL) suspend
{
  if (suspend)
    [[self window] disableFlushWindow];
  else
    [[self window] enableFlushWindow];
}

//--------------------------------------------------------------------------------------------------

/**
 * Method receives notifications from Scintilla (e.g. for handling clicks on the
 * folder margin or changes in the editor).
 * A delegate can be set to receive all notifications. If set no handling takes place here, except
 * for action pertaining to internal stuff (like the info bar).
 */
- (void) notification: (Scintilla::SCNotification*)scn
{
  // Parent notification. Details are passed as SCNotification structure.

  if (mDelegate != nil)
  {
    [mDelegate notification: scn];
    if (scn->nmhdr.code != SCN_ZOOM && scn->nmhdr.code != SCN_UPDATEUI)
      return;
  }

  switch (scn->nmhdr.code)
  {
    case SCN_MARGINCLICK:
    {
      if (scn->margin == 2)
      {
	// Click on the folder margin. Toggle the current line if possible.
	long line = [self getGeneralProperty: SCI_LINEFROMPOSITION parameter: scn->position];
	[self setGeneralProperty: SCI_TOGGLEFOLD value: line];
      }
      break;
    };
    case SCN_MODIFIED:
    {
      // Decide depending on the modification type what to do.
      // There can be more than one modification carried by one notification.
      if (scn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
	[self sendNotification: NSTextDidChangeNotification];
      break;
    }
    case SCN_ZOOM:
    {
      // A zoom change happened. Notify info bar if there is one.
      float zoom = [self getGeneralProperty: SCI_GETZOOM parameter: 0];
      long fontSize = [self getGeneralProperty: SCI_STYLEGETSIZE parameter: STYLE_DEFAULT];
      float factor = (zoom / fontSize) + 1;
      [mInfoBar notify: IBNZoomChanged message: nil location: NSZeroPoint value: factor];
      break;
    }
    case SCN_UPDATEUI:
    {
      // Triggered whenever changes in the UI state need to be reflected.
      // These can be: caret changes, selection changes etc.
      NSPoint caretPosition = mBackend->GetCaretPosition();
      [mInfoBar notify: IBNCaretChanged message: nil location: caretPosition value: 0];
      [self sendNotification: SCIUpdateUINotification];
      if (scn->updated & (SC_UPDATE_SELECTION | SC_UPDATE_CONTENT))
      {
	[self sendNotification: NSTextViewDidChangeSelectionNotification];
      }
      break;
    }
    case SCN_FOCUSOUT:
      [self sendNotification: NSTextDidEndEditingNotification];
      break;
    case SCN_FOCUSIN: // Nothing to do for now.
      break;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Initialization of the view. Used to setup a few other things we need.
 */
- (id) initWithFrame: (NSRect) frame
{
  self = [super initWithFrame:frame];
  if (self)
  {
    mContent = [[[[[self class] contentViewClass] alloc] initWithFrame:NSZeroRect] autorelease];
    mContent.owner = self;

    // Initialize the scrollers but don't show them yet.
    // Pick an arbitrary size, just to make NSScroller selecting the proper scroller direction
    // (horizontal or vertical).
    NSRect scrollerRect = NSMakeRect(0, 0, 100, 10);
    scrollView = [[[NSScrollView alloc] initWithFrame: scrollerRect] autorelease];
    [scrollView setDocumentView: mContent];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setHasHorizontalScroller:YES];
    [scrollView setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    //[scrollView setScrollerStyle:NSScrollerStyleLegacy];
    //[scrollView setScrollerKnobStyle:NSScrollerKnobStyleDark];
    //[scrollView setHorizontalScrollElasticity:NSScrollElasticityNone];
    [self addSubview: scrollView];

    marginView = [[SCIMarginView alloc] initWithScrollView:scrollView];
    marginView.owner = self;
    [marginView setRuleThickness:[marginView requiredThickness]];
    [scrollView setVerticalRulerView:marginView];
    [scrollView setHasHorizontalRuler:NO];
    [scrollView setHasVerticalRuler:YES];
    [scrollView setRulersVisible:YES];

    mBackend = new ScintillaCocoa(mContent, marginView);

    // Establish a connection from the back end to this container so we can handle situations
    // which require our attention.
    mBackend->SetDelegate(self);

    // Setup a special indicator used in the editor to provide visual feedback for
    // input composition, depending on language, keyboard etc.
    [self setColorProperty: SCI_INDICSETFORE parameter: INDIC_IME fromHTML: @"#FF0000"];
    [self setGeneralProperty: SCI_INDICSETUNDER parameter: INDIC_IME value: 1];
    [self setGeneralProperty: SCI_INDICSETSTYLE parameter: INDIC_IME value: INDIC_PLAIN];
    [self setGeneralProperty: SCI_INDICSETALPHA parameter: INDIC_IME value: 100];

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(applicationDidResignActive:)
                   name:NSApplicationDidResignActiveNotification
                 object:nil];

    [center addObserver:self
               selector:@selector(applicationDidBecomeActive:)
                   name:NSApplicationDidBecomeActiveNotification
                 object:nil];

    [[scrollView contentView] setPostsBoundsChangedNotifications:YES];
    [center addObserver:self
	       selector:@selector(scrollerAction:)
		   name:NSViewBoundsDidChangeNotification
		 object:[scrollView contentView]];
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  delete mBackend;
  [marginView release];
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

- (void) applicationDidResignActive: (NSNotification *)note {
#pragma unused(note)
    mBackend->ActiveStateChanged(false);
}

//--------------------------------------------------------------------------------------------------

- (void) applicationDidBecomeActive: (NSNotification *)note {
#pragma unused(note)
    mBackend->ActiveStateChanged(true);
}

//--------------------------------------------------------------------------------------------------

- (void) viewDidMoveToWindow
{
  [super viewDidMoveToWindow];

  [self positionSubViews];

  // Enable also mouse move events for our window (and so this view).
  [[self window] setAcceptsMouseMovedEvents: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to position and size the parts of the editor (content, scrollers, info bar).
 */
- (void) positionSubViews
{
  CGFloat scrollerWidth = [NSScroller scrollerWidthForControlSize:NSRegularControlSize
						scrollerStyle:NSScrollerStyleLegacy];

  NSSize size = [self frame].size;
  NSRect barFrame = {{0, size.height - scrollerWidth}, {size.width, scrollerWidth}};
  BOOL infoBarVisible = mInfoBar != nil && ![mInfoBar isHidden];

  // Horizontal offset of the content. Almost always 0 unless the vertical scroller
  // is on the left side.
  CGFloat contentX = 0;
  NSRect scrollRect = {{contentX, 0}, {size.width, size.height}};

  // Info bar frame.
  if (infoBarVisible)
  {
    scrollRect.size.height -= scrollerWidth;
    // Initial value already is as if the bar is at top.
    if (!mInfoBarAtTop)
    {
      scrollRect.origin.y += scrollerWidth;
      barFrame.origin.y = 0;
    }
  }

  if (!NSEqualRects([scrollView frame], scrollRect)) {
    [scrollView setFrame: scrollRect];
  }

  if (infoBarVisible)
    [mInfoBar setFrame: barFrame];
}

//--------------------------------------------------------------------------------------------------

/**
 * Set the width of the margin.
 */
- (void) setMarginWidth: (int) width
{
  if (marginView.ruleThickness != width)
  {
    marginView.marginWidth = width;
    [marginView setRuleThickness:[marginView requiredThickness]];
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered by one of the scrollers when it gets manipulated by the user. Notify the backend
 * about the change.
 */
- (void) scrollerAction: (id) sender
{
  mBackend->UpdateForScroll();
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to reposition our content depending on the size of the view.
 */
- (void) setFrame: (NSRect) newFrame
{
  NSRect previousFrame = [self frame];
  [super setFrame: newFrame];
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
- (NSString*) selectedString
{
  NSString *result = @"";

  const long length = mBackend->WndProc(SCI_GETSELTEXT, 0, 0);
  if (length > 0)
  {
    std::string buffer(length + 1, '\0');
    try
    {
      mBackend->WndProc(SCI_GETSELTEXT, length + 1, (sptr_t) &buffer[0]);

      result = [NSString stringWithUTF8String: buffer.c_str()];
    }
    catch (...)
    {
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Delete a range from the document.
 */
- (void) deleteRange: (NSRange) aRange
{
  if (aRange.length > 0)
  {
    NSRange posRange = mBackend->PositionsFromCharacters(aRange);
    [self message: SCI_DELETERANGE wParam: posRange.location lParam: posRange.length];
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Getter for the current text in raw form (no formatting information included).
 * If there is no text available an empty string is returned.
 */
- (NSString*) string
{
  NSString *result = @"";

  const long length = mBackend->WndProc(SCI_GETLENGTH, 0, 0);
  if (length > 0)
  {
    std::string buffer(length + 1, '\0');
    try
    {
      mBackend->WndProc(SCI_GETTEXT, length + 1, (sptr_t) &buffer[0]);

      result = [NSString stringWithUTF8String: buffer.c_str()];
    }
    catch (...)
    {
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Setter for the current text (no formatting included).
 */
- (void) setString: (NSString*) aString
{
  const char* text = [aString UTF8String];
  mBackend->WndProc(SCI_SETTEXT, 0, (long) text);
}

//--------------------------------------------------------------------------------------------------

- (void) insertString: (NSString*) aString atOffset: (int)offset
{
  const char* text = [aString UTF8String];
  mBackend->WndProc(SCI_ADDTEXT, offset, (long) text);
}

//--------------------------------------------------------------------------------------------------

- (void) setEditable: (BOOL) editable
{
  mBackend->WndProc(SCI_SETREADONLY, editable ? 0 : 1, 0);
}

//--------------------------------------------------------------------------------------------------

- (BOOL) isEditable
{
  return mBackend->WndProc(SCI_GETREADONLY, 0, 0) == 0;
}

//--------------------------------------------------------------------------------------------------

- (SCIContentView*) content
{
  return mContent;
}

//--------------------------------------------------------------------------------------------------

- (void) updateMarginCursors {
  [[self window] invalidateCursorRectsForView: marginView];
}

//--------------------------------------------------------------------------------------------------

/**
 * Direct call into the backend to allow uninterpreted access to it. The values to be passed in and
 * the result heavily depend on the message that is used for the call. Refer to the Scintilla
 * documentation to learn what can be used here.
 */
+ (sptr_t) directCall: (ScintillaView*) sender message: (unsigned int) message wParam: (uptr_t) wParam
               lParam: (sptr_t) lParam
{
  return ScintillaCocoa::DirectFunction(
    reinterpret_cast<sptr_t>(sender->mBackend), message, wParam, lParam);
}

- (sptr_t) message: (unsigned int) message wParam: (uptr_t) wParam lParam: (sptr_t) lParam
{
  return mBackend->WndProc(message, wParam, lParam);
}

- (sptr_t) message: (unsigned int) message wParam: (uptr_t) wParam
{
  return mBackend->WndProc(message, wParam, 0);
}

- (sptr_t) message: (unsigned int) message
{
  return mBackend->WndProc(message, 0, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * This is a helper method to set properties in the backend, with native parameters.
 *
 * @param property Main property like SCI_STYLESETFORE for which a value is to be set.
 * @param parameter Additional info for this property like a parameter or index.
 * @param value The actual value. It depends on the property what this parameter means.
 */
- (void) setGeneralProperty: (int) property parameter: (long) parameter value: (long) value
{
  mBackend->WndProc(property, parameter, value);
}

//--------------------------------------------------------------------------------------------------

/**
 * A simplified version for setting properties which only require one parameter.
 *
 * @param property Main property like SCI_STYLESETFORE for which a value is to be set.
 * @param value The actual value. It depends on the property what this parameter means.
 */
- (void) setGeneralProperty: (int) property value: (long) value
{
  mBackend->WndProc(property, value, 0);
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
- (long) getGeneralProperty: (int) property parameter: (long) parameter extra: (long) extra
{
  return mBackend->WndProc(property, parameter, extra);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convenience function to avoid unneeded extra parameter.
 */
- (long) getGeneralProperty: (int) property parameter: (long) parameter
{
  return mBackend->WndProc(property, parameter, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Convenience function to avoid unneeded parameters.
 */
- (long) getGeneralProperty: (int) property
{
  return mBackend->WndProc(property, 0, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Use this variant if you have to pass in a reference to something (e.g. a text range).
 */
- (long) getGeneralProperty: (int) property ref: (const void*) ref
{
  return mBackend->WndProc(property, 0, (sptr_t) ref);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for colors.
 */
- (void) setColorProperty: (int) property parameter: (long) parameter value: (NSColor*) value
{
  if ([value colorSpaceName] != NSDeviceRGBColorSpace)
    value = [value colorUsingColorSpaceName: NSDeviceRGBColorSpace];
  long red = static_cast<long>([value redComponent] * 255);
  long green = static_cast<long>([value greenComponent] * 255);
  long blue = static_cast<long>([value blueComponent] * 255);

  long color = (blue << 16) + (green << 8) + red;
  mBackend->WndProc(property, parameter, color);
}

//--------------------------------------------------------------------------------------------------

/**
 * Another color property setting, which allows to specify the color as string like in HTML
 * documents (i.e. with leading # and either 3 hex digits or 6).
 */
- (void) setColorProperty: (int) property parameter: (long) parameter fromHTML: (NSString*) fromHTML
{
  if ([fromHTML length] > 3 && [fromHTML characterAtIndex: 0] == '#')
  {
    bool longVersion = [fromHTML length] > 6;
    int index = 1;

    char value[3] = {0, 0, 0};
    value[0] = static_cast<char>([fromHTML characterAtIndex: index++]);
    if (longVersion)
      value[1] = static_cast<char>([fromHTML characterAtIndex: index++]);
    else
      value[1] = value[0];

    unsigned rawRed;
    [[NSScanner scannerWithString: [NSString stringWithUTF8String: value]] scanHexInt: &rawRed];

    value[0] = static_cast<char>([fromHTML characterAtIndex: index++]);
    if (longVersion)
      value[1] = static_cast<char>([fromHTML characterAtIndex: index++]);
    else
      value[1] = value[0];

    unsigned rawGreen;
    [[NSScanner scannerWithString: [NSString stringWithUTF8String: value]] scanHexInt: &rawGreen];

    value[0] = static_cast<char>([fromHTML characterAtIndex: index++]);
    if (longVersion)
      value[1] = static_cast<char>([fromHTML characterAtIndex: index++]);
    else
      value[1] = value[0];

    unsigned rawBlue;
    [[NSScanner scannerWithString: [NSString stringWithUTF8String: value]] scanHexInt: &rawBlue];

    long color = (rawBlue << 16) + (rawGreen << 8) + rawRed;
    mBackend->WndProc(property, parameter, color);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for colors.
 */
- (NSColor*) getColorProperty: (int) property parameter: (long) parameter
{
  long color = mBackend->WndProc(property, parameter, 0);
  CGFloat red = (color & 0xFF) / 255.0;
  CGFloat green = ((color >> 8) & 0xFF) / 255.0;
  CGFloat blue = ((color >> 16) & 0xFF) / 255.0;
  NSColor* result = [NSColor colorWithDeviceRed: red green: green blue: blue alpha: 1];
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for references (pointers, addresses).
 */
- (void) setReferenceProperty: (int) property parameter: (long) parameter value: (const void*) value
{
  mBackend->WndProc(property, parameter, (sptr_t) value);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for references (pointers, addresses).
 */
- (const void*) getReferenceProperty: (int) property parameter: (long) parameter
{
  return (const void*) mBackend->WndProc(property, parameter, 0);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for string values.
 */
- (void) setStringProperty: (int) property parameter: (long) parameter value: (NSString*) value
{
  const char* rawValue = [value UTF8String];
  mBackend->WndProc(property, parameter, (sptr_t) rawValue);
}


//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for string values.
 */
- (NSString*) getStringProperty: (int) property parameter: (long) parameter
{
  const char* rawValue = (const char*) mBackend->WndProc(property, parameter, 0);
  return [NSString stringWithUTF8String: rawValue];
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property setter for lexer properties, which are commonly passed as strings.
 */
- (void) setLexerProperty: (NSString*) name value: (NSString*) value
{
  const char* rawName = [name UTF8String];
  const char* rawValue = [value UTF8String];
  mBackend->WndProc(SCI_SETPROPERTY, (sptr_t) rawName, (sptr_t) rawValue);
}

//--------------------------------------------------------------------------------------------------

/**
 * Specialized property getter for references (pointers, addresses).
 */
- (NSString*) getLexerProperty: (NSString*) name
{
  const char* rawName = [name UTF8String];
  const char* result = (const char*) mBackend->WndProc(SCI_SETPROPERTY, (sptr_t) rawName, 0);
  return [NSString stringWithUTF8String: result];
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets the notification callback
 */
- (void) registerNotifyCallback: (intptr_t) windowid value: (Scintilla::SciNotifyFunc) callback
{
	mBackend->RegisterNotifyCallback(windowid, callback);
}


//--------------------------------------------------------------------------------------------------

/**
 * Sets the new control which is displayed as info bar at the top or bottom of the editor.
 * Set newBar to nil if you want to hide the bar again.
 * The info bar's height is set to the height of the scrollbar.
 */
- (void) setInfoBar: (NSView <InfoBarCommunicator>*) newBar top: (BOOL) top
{
  if (mInfoBar != newBar)
  {
    [mInfoBar removeFromSuperview];

    mInfoBar = newBar;
    mInfoBarAtTop = top;
    if (mInfoBar != nil)
    {
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
- (void) setStatusText: (NSString*) text
{
  if (mInfoBar != nil)
    [mInfoBar notify: IBNStatusChanged message: text location: NSZeroPoint value: 0];
}

//--------------------------------------------------------------------------------------------------

- (NSRange) selectedRange
{
  return [mContent selectedRange];
}

//--------------------------------------------------------------------------------------------------

- (void)insertText: (id) aString
{
  if ([aString isKindOfClass:[NSString class]])
    mBackend->InsertText(aString);
  else if ([aString isKindOfClass:[NSAttributedString class]])
    mBackend->InsertText([aString string]);
}

//--------------------------------------------------------------------------------------------------

/**
 * For backwards compatibility.
 */
- (BOOL) findAndHighlightText: (NSString*) searchText
                    matchCase: (BOOL) matchCase
                    wholeWord: (BOOL) wholeWord
                     scrollTo: (BOOL) scrollTo
                         wrap: (BOOL) wrap
{
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
- (BOOL) findAndHighlightText: (NSString*) searchText
                    matchCase: (BOOL) matchCase
                    wholeWord: (BOOL) wholeWord
                     scrollTo: (BOOL) scrollTo
                         wrap: (BOOL) wrap
                    backwards: (BOOL) backwards
{
  int searchFlags= 0;
  if (matchCase)
    searchFlags |= SCFIND_MATCHCASE;
  if (wholeWord)
    searchFlags |= SCFIND_WHOLEWORD;

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
  const char* textToSearch = [searchText UTF8String];

  // The following call will also set the selection if something was found.
  if (backwards)
  {
    result = [ScintillaView directCall: self
                               message: SCI_SEARCHPREV
                                wParam: searchFlags
                                lParam: (sptr_t) textToSearch];
    if (result < 0 && wrap)
    {
      // Try again from the end of the document if nothing could be found so far and
      // wrapped search is set.
      [self getGeneralProperty: SCI_SETSELECTIONSTART parameter: [self getGeneralProperty: SCI_GETTEXTLENGTH parameter: 0]];
      [self setGeneralProperty: SCI_SEARCHANCHOR value: 0];
      result = [ScintillaView directCall: self
                                 message: SCI_SEARCHNEXT
                                  wParam: searchFlags
                                  lParam: (sptr_t) textToSearch];
    }
  }
  else
  {
    result = [ScintillaView directCall: self
                               message: SCI_SEARCHNEXT
                                wParam: searchFlags
                                lParam: (sptr_t) textToSearch];
    if (result < 0 && wrap)
    {
      // Try again from the start of the document if nothing could be found so far and
      // wrapped search is set.
      [self getGeneralProperty: SCI_SETSELECTIONSTART parameter: 0];
      [self setGeneralProperty: SCI_SEARCHANCHOR value: 0];
      result = [ScintillaView directCall: self
                                 message: SCI_SEARCHNEXT
                                  wParam: searchFlags
                                  lParam: (sptr_t) textToSearch];
    }
  }

  if (result >= 0)
  {
    if (scrollTo)
      [self setGeneralProperty: SCI_SCROLLCARET value: 0];
  }
  else
  {
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
- (int) findAndReplaceText: (NSString*) searchText
                    byText: (NSString*) newText
                 matchCase: (BOOL) matchCase
                 wholeWord: (BOOL) wholeWord
                     doAll: (BOOL) doAll
{
  // The current position is where we start searching for single occurrences. Otherwise we start at
  // the beginning of the document.
  long startPosition;
  if (doAll)
    startPosition = 0; // Start at the beginning of the text if we replace all occurrences.
  else
    // For a single replacement we start at the current caret position.
    startPosition = [self getGeneralProperty: SCI_GETCURRENTPOS];
  long endPosition = [self getGeneralProperty: SCI_GETTEXTLENGTH];

  int searchFlags= 0;
  if (matchCase)
    searchFlags |= SCFIND_MATCHCASE;
  if (wholeWord)
    searchFlags |= SCFIND_WHOLEWORD;
  [self setGeneralProperty: SCI_SETSEARCHFLAGS value: searchFlags];
  [self setGeneralProperty: SCI_SETTARGETSTART value: startPosition];
  [self setGeneralProperty: SCI_SETTARGETEND value: endPosition];

  const char* textToSearch = [searchText UTF8String];
  long sourceLength = strlen(textToSearch); // Length in bytes.
  const char* replacement = [newText UTF8String];
  long targetLength = strlen(replacement);  // Length in bytes.
  sptr_t result;

  int replaceCount = 0;
  if (doAll)
  {
    while (true)
    {
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
  }
  else
  {
    result = [ScintillaView directCall: self
                               message: SCI_SEARCHINTARGET
                                wParam: sourceLength
                                lParam: (sptr_t) textToSearch];
    replaceCount = (result < 0) ? 0 : 1;

    if (replaceCount > 0)
    {
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

- (void) setFontName: (NSString*) font
                size: (int) size
                bold: (BOOL) bold
                italic: (BOOL) italic
{
  for (int i = 0; i < 128; i++)
  {
    [self setGeneralProperty: SCI_STYLESETFONT
                   parameter: i
                       value: (sptr_t)[font UTF8String]];
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

