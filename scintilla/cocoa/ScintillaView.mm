
/**
 * Implementation of the native Cocoa View that serves as container for the scintilla parts.
 *
 * Created by Mike Lischke.
 *
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import "ScintillaView.h"

using namespace Scintilla;

// Two additional cursors we need, which aren't provided by Cocoa.
static NSCursor* reverseArrowCursor;
static NSCursor* waitCursor;

// The scintilla indicator used for keyboard input.
#define INPUT_INDICATOR INDIC_MAX - 1

NSString *SCIUpdateUINotification = @"SCIUpdateUI";

@implementation InnerView

@synthesize owner = mOwner;

//--------------------------------------------------------------------------------------------------

- (NSView*) initWithFrame: (NSRect) frame 
{
  self = [super initWithFrame: frame];
  
  if (self != nil)
  {
    // Some initialization for our view.
    mCurrentCursor = [[NSCursor arrowCursor] retain];
    mCurrentTrackingRect = 0;
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
    
    [self registerForDraggedTypes: [NSArray arrayWithObjects:
                                   NSStringPboardType, ScintillaRecPboardType, NSFilenamesPboardType, nil]];
  }
  
  return self;
}

//--------------------------------------------------------------------------------------------------

/**
 * When the view is resized we need to update our tracking rectangle and let the backend know.
 */
- (void) setFrame: (NSRect) frame
{
  [super setFrame: frame];

  // Make the content also a tracking rectangle for mouse events.
  if (mCurrentTrackingRect != 0)
    [self removeTrackingRect: mCurrentTrackingRect];
	mCurrentTrackingRect = [self addTrackingRect: [self bounds]
                                         owner: self
                                      userData: nil
                                  assumeInside: YES];
  mOwner.backend->Resize();
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the backend if a new cursor must be set for the view.
 */
- (void) setCursor: (Window::Cursor) cursor
{
  [mCurrentCursor autorelease];
  switch (cursor)
  {
    case Window::cursorText:
      mCurrentCursor = [NSCursor IBeamCursor];
      break;
    case Window::cursorArrow:
      mCurrentCursor = [NSCursor arrowCursor];
      break;
    case Window::cursorWait:
      mCurrentCursor = waitCursor;
      break;
    case Window::cursorHoriz:
      mCurrentCursor = [NSCursor resizeLeftRightCursor];
      break;
    case Window::cursorVert:
      mCurrentCursor = [NSCursor resizeUpDownCursor];
      break;
    case Window::cursorReverseArrow:
      mCurrentCursor = reverseArrowCursor;
      break;
    case Window::cursorUp:
    default:
      mCurrentCursor = [NSCursor arrowCursor];
      break;
  }
  
  [mCurrentCursor retain];
  
  // Trigger recreation of the cursor rectangle(s).
  [[self window] invalidateCursorRectsForView: self];
}

//--------------------------------------------------------------------------------------------------

/**
 * This method is called to give us the opportunity to define our mouse sensitive rectangle.
 */
- (void) resetCursorRects
{
  [super resetCursorRects];
  
  // We only have one cursor rect: our bounds.
  NSRect bounds = [self bounds];
  [self addCursorRect: [self bounds] cursor: mCurrentCursor];
  [mCurrentCursor setOnMouseEntered: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Gets called by the runtime when the view needs repainting.
 */
- (void) drawRect: (NSRect) rect
{
  CGContextRef context = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
  
  mOwner.backend->Draw(rect, context);
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

// Adoption of NSTextInput protocol.

- (NSAttributedString*) attributedSubstringFromRange: (NSRange) range
{
  return nil;
}

//--------------------------------------------------------------------------------------------------

- (NSUInteger) characterIndexForPoint: (NSPoint) point
{
  return NSNotFound;
}

//--------------------------------------------------------------------------------------------------

- (NSInteger) conversationIdentifier
{
  return (NSInteger) self;

}

//--------------------------------------------------------------------------------------------------

- (void) doCommandBySelector: (SEL) selector
{
  if ([self respondsToSelector: @selector(selector)])
    [self performSelector: selector withObject: nil];
}

//--------------------------------------------------------------------------------------------------

- (NSRect) firstRectForCharacterRange: (NSRange) range
{
  return NSZeroRect;
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
 */
- (void) insertText: (id) aString
{
  // Remove any previously marked text first.
  [self removeMarkedText];
  mOwner.backend->InsertText((NSString*) aString);
}

//--------------------------------------------------------------------------------------------------

- (NSRange) markedRange
{
  return mMarkedTextRange;
}

//--------------------------------------------------------------------------------------------------

- (NSRange) selectedRange
{
  int begin = [mOwner getGeneralProperty: SCI_GETSELECTIONSTART parameter: 0];
  int end = [mOwner getGeneralProperty: SCI_GETSELECTIONEND parameter: 0];
  return NSMakeRange(begin, end - begin);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the input manager to set text which might be combined with further input to form
 * the final text (e.g. composition of ^ and a to â).
 *
 * @param aString The text to insert, either what has been marked already or what is selected already
 *                or simply added at the current insertion point. Depending on what is available.
 * @param range The range of the new text to select (given relative to the insertion point of the new text).
 */
- (void) setMarkedText: (id) aString selectedRange: (NSRange) range
{
  // Since we did not return any valid attribute for marked text (see validAttributesForMarkedText)
  // we can safely assume the passed in text is an NSString instance.
  NSString* newText = (NSString*) aString;
  int currentPosition = [mOwner getGeneralProperty: SCI_GETCURRENTPOS parameter: 0];

  // Replace marked text if there is one.
  if (mMarkedTextRange.length > 0)
  {
    // We have already marked text. Replace that.
    [mOwner setGeneralProperty: SCI_SETSELECTIONSTART
                         value: mMarkedTextRange.location];
    [mOwner setGeneralProperty: SCI_SETSELECTIONEND 
                         value: mMarkedTextRange.location + mMarkedTextRange.length];
    currentPosition = mMarkedTextRange.location;
  }

  // Note: Scintilla internally works almost always with bytes instead chars, so we need to take
  //       this into account when determining selection ranges and such.
  std::string raw_text = [newText UTF8String];
  mOwner.backend->InsertText(newText);

  mMarkedTextRange.location = currentPosition;
  mMarkedTextRange.length = raw_text.size();
    
  // Mark the just inserted text. Keep the marked range for later reset.
  [mOwner setGeneralProperty: SCI_SETINDICATORCURRENT value: INPUT_INDICATOR];
  [mOwner setGeneralProperty: SCI_INDICATORFILLRANGE
                   parameter: mMarkedTextRange.location
                       value: mMarkedTextRange.length];
  
  // Select the part which is indicated in the given range. It does not scroll the caret into view.
  if (range.length > 0)
  {
    [mOwner setGeneralProperty: SCI_SETSELECTIONSTART
                     value: currentPosition + range.location];
    [mOwner setGeneralProperty: SCI_SETSELECTIONEND 
                     value: currentPosition + range.location + range.length];
  }
}

//--------------------------------------------------------------------------------------------------

- (void) unmarkText
{
  if (mMarkedTextRange.length > 0)
  {
    [mOwner setGeneralProperty: SCI_SETINDICATORCURRENT value: INPUT_INDICATOR];
    [mOwner setGeneralProperty: SCI_INDICATORCLEARRANGE
                     parameter: mMarkedTextRange.location
                         value: mMarkedTextRange.length];
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes any currently marked text.
 */
- (void) removeMarkedText
{
  if (mMarkedTextRange.length > 0)
  {
    // We have already marked text. Replace that.
    [mOwner setGeneralProperty: SCI_SETSELECTIONSTART
                     value: mMarkedTextRange.location];
    [mOwner setGeneralProperty: SCI_SETSELECTIONEND 
                     value: mMarkedTextRange.location + mMarkedTextRange.length];
    mOwner.backend->InsertText(@"");
    mMarkedTextRange = NSMakeRange(NSNotFound, 0);
  }
}

//--------------------------------------------------------------------------------------------------

- (NSArray*) validAttributesForMarkedText
{
  return nil;
}

// End of the NSTextInput protocol adoption.

//--------------------------------------------------------------------------------------------------

/**
 * Generic input method. It is used to pass on keyboard input to Scintilla. The control itself only
 * handles shortcuts. The input is then forwarded to the Cocoa text input system, which in turn does
 * its own input handling (character composition via NSTextInput protocol):
 */
- (void) keyDown: (NSEvent *) theEvent
{
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

- (void) scrollWheel: (NSEvent *) theEvent
{
  mOwner.backend->MouseWheel(theEvent);
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
  return YES;
}

//--------------------------------------------------------------------------------------------------

- (BOOL) performDragOperation: (id <NSDraggingInfo>) sender
{
  return mOwner.backend->PerformDragOperation(sender);  
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns operations we allow as drag source.
 */
- (NSDragOperation) draggingSourceOperationMaskForLocal: (BOOL) flag
{
  return NSDragOperationCopy | NSDragOperationMove;
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
  mOwner.backend->SelectAll();
}

- (void) deleteBackward: (id) sender
{
  mOwner.backend->DeleteBackward();
}

- (void) cut: (id) sender
{
  mOwner.backend->Cut();
}

- (void) copy: (id) sender
{
  mOwner.backend->Copy();
}

- (void) paste: (id) sender
{
  mOwner.backend->Paste();
}

- (void) undo: (id) sender
{
  mOwner.backend->Undo();
}

- (void) redo: (id) sender
{
  mOwner.backend->Redo();
}

- (BOOL) canUndo
{
  return mOwner.backend->CanUndo();
}

- (BOOL) canRedo
{
  return mOwner.backend->CanRedo();
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
@synthesize owner   = mOwner;

/**
 * ScintiallView is a composite control made from an NSView and an embedded NSView that is
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
    
    NSString* path = [bundle pathForResource: @"mac_cursor_busy" ofType: @"png" inDirectory: nil];
    NSImage* image = [[[NSImage alloc] initWithContentsOfFile: path] autorelease];
    waitCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint(2, 2)];
    
    path = [bundle pathForResource: @"mac_cursor_flipped" ofType: @"png" inDirectory: nil];
    image = [[[NSImage alloc] initWithContentsOfFile: path] autorelease];
    reverseArrowCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint(12, 2)];
  }
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
 * Called by a connected compontent (usually the info bar) if something changed there.
 *
 * @param type The type of the notification.
 * @param message Carries the new status message if the type is a status message change.
 * @param location Carries the new location (e.g. caret) if the type is a caret change or similar type.
 * @param location Carries the new zoom value if the type is a zoom change.
 */
- (void) notify: (NotificationType) type message: (NSString*) message location: (NSPoint) location
          value: (float) value
{
  switch (type)
  {
    case IBNZoomChanged:
    {
      // Compute point increase/decrease based on default font size.
      int fontSize = [self getGeneralProperty: SCI_STYLEGETSIZE parameter: STYLE_DEFAULT];
      int zoom = (int) (fontSize * (value - 1));
      [self setGeneralProperty: SCI_SETZOOM value: zoom];
      break;
    }
  };
}

//--------------------------------------------------------------------------------------------------

- (void) setCallback: (id <InfoBarCommunicator>) callback
{
  // Not used. Only here to satisfy protocol.
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
 * Notification function used by Scintilla to call us back (e.g. for handling clicks on the 
 * folder margin or changes in the editor).
 */
static void notification(intptr_t windowid, unsigned int iMessage, uintptr_t wParam, uintptr_t lParam)
{
  // WM_NOTIFY means we got a parent notification with a special notification structure.
  // Here we don't really differentiate between parent and own notifications and handle both.
  ScintillaView* editor;
  switch (iMessage)
  {
    case WM_NOTIFY:
    {
      // Parent notification. Details are passed as SCNotification structure.
      SCNotification* scn = reinterpret_cast<SCNotification*>(lParam);
      editor = reinterpret_cast<InnerView*>(scn->nmhdr.idFrom).owner;
      switch (scn->nmhdr.code)
      {
        case SCN_MARGINCLICK:
        {
          if (scn->margin == 2)
          {
            // Click on the folder margin. Toggle the current line if possible.
            int line = [editor getGeneralProperty: SCI_LINEFROMPOSITION parameter: scn->position];
            [editor setGeneralProperty: SCI_TOGGLEFOLD value: line];
          }
          break;
          };
        case SCN_MODIFIED:
        {
          // Decide depending on the modification type what to do.
          // There can be more than one modification carried by one notification.
          if (scn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
            [editor sendNotification: NSTextDidChangeNotification];
          break;
        }
        case SCN_ZOOM:
        {
          // A zoom change happend. Notify info bar if there is one.
          float zoom = [editor getGeneralProperty: SCI_GETZOOM parameter: 0];
          int fontSize = [editor getGeneralProperty: SCI_STYLEGETSIZE parameter: STYLE_DEFAULT];
          float factor = (zoom / fontSize) + 1;
          [editor->mInfoBar notify: IBNZoomChanged message: nil location: NSZeroPoint value: factor];
          break;
        }
        case SCN_UPDATEUI:
        {
          // Triggered whenever changes in the UI state need to be reflected.
          // These can be: caret changes, selection changes etc.
          NSPoint caretPosition = editor->mBackend->GetCaretPosition();
          [editor->mInfoBar notify: IBNCaretChanged message: nil location: caretPosition value: 0];
          [editor sendNotification: SCIUpdateUINotification];
          [editor sendNotification: NSTextViewDidChangeSelectionNotification];
          break;
      }
      }
      break;
    }
    case WM_COMMAND:
    {
      // Notifications for the editor itself.
      ScintillaCocoa* backend = reinterpret_cast<ScintillaCocoa*>(lParam);
      editor = backend->TopContainer();
      switch (wParam >> 16)
      {
        case SCEN_KILLFOCUS:
          [editor sendNotification: NSTextDidEndEditingNotification];
          break;
        case SCEN_SETFOCUS: // Nothing to do for now.
          break;
      }
      break;
    }
  };
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
    mContent = [[[InnerView alloc] init] autorelease];
    mBackend = new ScintillaCocoa(mContent);
    mContent.owner = self;
    [self addSubview: mContent];
    
    // Initialize the scrollers but don't show them yet.
    // Pick an arbitrary size, just to make NSScroller selecting the proper scroller direction
    // (horizontal or vertical).
    NSRect scrollerRect = NSMakeRect(0, 0, 100, 10);
    mHorizontalScroller = [[[NSScroller alloc] initWithFrame: scrollerRect] autorelease];
    [mHorizontalScroller setHidden: YES];
    [mHorizontalScroller setTarget: self];
    [mHorizontalScroller setAction: @selector(scrollerAction:)];
    [self addSubview: mHorizontalScroller];
    
    scrollerRect.size = NSMakeSize(10, 100);
    mVerticalScroller = [[[NSScroller alloc] initWithFrame: scrollerRect] autorelease];
    [mVerticalScroller setHidden: YES];
    [mVerticalScroller setTarget: self];
    [mVerticalScroller setAction: @selector(scrollerAction:)];
    [self addSubview: mVerticalScroller];
    
    // Establish a connection from the back end to this container so we can handle situations
    // which require our attention.
    mBackend->RegisterNotifyCallback(nil, notification);
    
    // Setup a special indicator used in the editor to provide visual feedback for 
    // input composition, depending on language, keyboard etc.
    [self setColorProperty: SCI_INDICSETFORE parameter: INPUT_INDICATOR fromHTML: @"#FF9A00"];
    [self setGeneralProperty: SCI_INDICSETUNDER parameter: INPUT_INDICATOR value: 1];
    [self setGeneralProperty: SCI_INDICSETSTYLE parameter: INPUT_INDICATOR value: INDIC_ROUNDBOX];
    [self setGeneralProperty: SCI_INDICSETALPHA parameter: INPUT_INDICATOR value: 100];
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  [mInfoBar release];
  delete mBackend;
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

- (void) viewDidMoveToWindow
{
  [super viewDidMoveToWindow];
  
  [self layout];
  
  // Enable also mouse move events for our window (and so this view).
  [[self window] setAcceptsMouseMovedEvents: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to position and size the parts of the editor (content, scrollers, info bar).
 */
- (void) layout
{
  int scrollerWidth = [NSScroller scrollerWidth];

  NSSize size = [self frame].size;
  NSRect hScrollerRect = {0, 0, size.width, scrollerWidth};
  NSRect vScrollerRect = {size.width - scrollerWidth, 0, scrollerWidth, size.height};
  NSRect barFrame = {0, size.height - scrollerWidth, size.width, scrollerWidth};
  BOOL infoBarVisible = mInfoBar != nil && ![mInfoBar isHidden];
  
  // Horizontal offset of the content. Almost always 0 unless the vertical scroller
  // is on the left side.
  int contentX = 0;
  
  // Vertical scroller frame calculation.
  if (![mVerticalScroller isHidden])
  {
    // Consider user settings (left vs right vertical scrollbar).
    BOOL isLeft = [[[NSUserDefaults standardUserDefaults] stringForKey: @"NSScrollerPosition"] 
                   isEqualToString: @"left"];
    if (isLeft)
    {
      vScrollerRect.origin.x = 0;
      hScrollerRect.origin.x = scrollerWidth;
      contentX = scrollerWidth;
    };
    
    size.width -= scrollerWidth;
    hScrollerRect.size.width -= scrollerWidth;
  }
  
  // Same for horizontal scroller.
  if (![mHorizontalScroller isHidden])
  {
    // Make room for the h-scroller.
    size.height -= scrollerWidth;
    vScrollerRect.size.height -= scrollerWidth;
    vScrollerRect.origin.y += scrollerWidth;
  };
  
  // Info bar frame.
  if (infoBarVisible)
  {
    // Initial value already is as if the bar is at top.
    if (mInfoBarAtTop)
    {
      vScrollerRect.size.height -= scrollerWidth;
      size.height -= scrollerWidth;
    }
    else
    {
      // Layout info bar and h-scroller side by side in a friendly manner.
      int nativeWidth = mInitialInfoBarWidth;
      int remainingWidth = barFrame.size.width;
      
      barFrame.origin.y = 0;

      if ([mHorizontalScroller isHidden])
      {
        // H-scroller is not visible, so take the full space.
        vScrollerRect.origin.y += scrollerWidth;
        vScrollerRect.size.height -= scrollerWidth;
        size.height -= scrollerWidth;
      }
      else
      {
        // If the left offset of the h-scroller is > 0 then the v-scroller is on the left side.
        // In this case we take the full width, otherwise what has been given to the h-scroller 
        // and content up to now.
        if (hScrollerRect.origin.x == 0)
          remainingWidth = size.width;

        // Note: remainingWidth can become < 0, which hides the scroller.
        remainingWidth -= nativeWidth;

        hScrollerRect.origin.x = nativeWidth;
        hScrollerRect.size.width = remainingWidth;
        barFrame.size.width = nativeWidth;
      }
    }
  }
  
  NSRect contentRect = {contentX, vScrollerRect.origin.y, size.width, size.height};
  [mContent setFrame: contentRect];
  
  if (infoBarVisible)
    [mInfoBar setFrame: barFrame];
  if (![mHorizontalScroller isHidden])
    [mHorizontalScroller setFrame: hScrollerRect];
  if (![mVerticalScroller isHidden])
    [mVerticalScroller setFrame: vScrollerRect];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the backend to adjust the vertical scroller (range and page).
 *
 * @param range Determines the total size of the scroll area used in the editor.
 * @param page Determines how many pixels a page is.
 * @result Returns YES if anything changed, otherwise NO.
 */
- (BOOL) setVerticalScrollRange: (int) range page: (int) page
{
  BOOL result = NO;
  BOOL hideScroller = page >= range;
  
  if ([mVerticalScroller isHidden] != hideScroller)
  {
    result = YES;
    [mVerticalScroller setHidden: hideScroller];
    if (!hideScroller)
      [mVerticalScroller setFloatValue: 0];
    [self layout];
  }
  
  if (!hideScroller)
  {
    [mVerticalScroller setEnabled: YES];
    
    CGFloat currentProportion = [mVerticalScroller knobProportion];
    CGFloat newProportion = page / (CGFloat) range;
    if (currentProportion != newProportion)
    {
      result = YES;
      [mVerticalScroller setKnobProportion: newProportion];
    }
  }
  
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to set the position of the vertical scroll thumb.
 *
 * @param position The relative position in the rang [0..1];
 */
- (void) setVerticalScrollPosition: (float) position
{
  [mVerticalScroller setFloatValue: position];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the backend to adjust the horizontal scroller (range and page).
 *
 * @param range Determines the total size of the scroll area used in the editor.
 * @param page Determines how many pixels a page is.
 * @result Returns YES if anything changed, otherwise NO.
 */
- (BOOL) setHorizontalScrollRange: (int) range page: (int) page
{
  BOOL result = NO;
  BOOL hideScroller = page >= range;
  
  if ([mHorizontalScroller isHidden] != hideScroller)
  {
    result = YES;
    [mHorizontalScroller setHidden: hideScroller];
    [self layout];
  }
  
  if (!hideScroller)
  {
    [mHorizontalScroller setEnabled: YES];
    
    CGFloat currentProportion = [mHorizontalScroller knobProportion];
    CGFloat newProportion = page / (CGFloat) range;
    if (currentProportion != newProportion)
    {
      result = YES;
      [mHorizontalScroller setKnobProportion: newProportion];
    }
  }
  
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to set the position of the vertical scroll thumb.
 *
 * @param position The relative position in the rang [0..1];
 */
- (void) setHorizontalScrollPosition: (float) position
{
  [mHorizontalScroller setFloatValue: position];
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered by one of the scrollers when it gets manipulated by the user. Notify the backend
 * about the change.
 */
- (void) scrollerAction: (id) sender
{
  float position = [sender doubleValue];
  mBackend->DoScroll(position, [sender hitPart], sender == mHorizontalScroller);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to reposition our content depending on the size of the view.
 */
- (void) setFrame: (NSRect) newFrame
{
  [super setFrame: newFrame];
  [self layout];
}

//--------------------------------------------------------------------------------------------------

/**
 * Getter for the currently selected text in raw form (no formatting information included).
 * If there is no text available an empty string is returned.
 */
- (NSString*) selectedString
{
  NSString *result = @"";
  
  char *buffer(0);
  const int length = mBackend->WndProc(SCI_GETSELTEXT, 0, 0);
  if (length > 0)
  {
    buffer = new char[length + 1];
    try
    {
      mBackend->WndProc(SCI_GETSELTEXT, length + 1, (sptr_t) buffer);
      mBackend->WndProc(SCI_SETSAVEPOINT, 0, 0);
      
      result = [NSString stringWithUTF8String: buffer];
      delete[] buffer;
    }
    catch (...)
    {
      delete[] buffer;
      buffer = 0;
    }
  }
  
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Getter for the current text in raw form (no formatting information included).
 * If there is no text available an empty string is returned.
 */
- (NSString*) string
{
  NSString *result = @"";
  
  char *buffer(0);
  const int length = mBackend->WndProc(SCI_GETLENGTH, 0, 0);
  if (length > 0)
  {
    buffer = new char[length + 1];
    try
    {
      mBackend->WndProc(SCI_GETTEXT, length + 1, (sptr_t) buffer);
      mBackend->WndProc(SCI_SETSAVEPOINT, 0, 0);
      
      result = [NSString stringWithUTF8String: buffer];
      delete[] buffer;
    }
    catch (...)
    {
      delete[] buffer;
      buffer = 0;
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

- (InnerView*) content
{
  return mContent;
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
  return ScintillaCocoa::DirectFunction(sender->mBackend, message, wParam, lParam);
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
  long red = [value redComponent] * 255;
  long green = [value greenComponent] * 255;
  long blue = [value blueComponent] * 255;
  
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
    value[0] = [fromHTML characterAtIndex: index++];
    if (longVersion)
      value[1] = [fromHTML characterAtIndex: index++];
    else
      value[1] = value[0];

    unsigned rawRed;
    [[NSScanner scannerWithString: [NSString stringWithUTF8String: value]] scanHexInt: &rawRed];

    value[0] = [fromHTML characterAtIndex: index++];
    if (longVersion)
      value[1] = [fromHTML characterAtIndex: index++];
    else
      value[1] = value[0];
    
    unsigned rawGreen;
    [[NSScanner scannerWithString: [NSString stringWithUTF8String: value]] scanHexInt: &rawGreen];

    value[0] = [fromHTML characterAtIndex: index++];
    if (longVersion)
      value[1] = [fromHTML characterAtIndex: index++];
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
  int color = mBackend->WndProc(property, parameter, 0);
  float red = (color & 0xFF) / 255.0;
  float green = ((color >> 8) & 0xFF) / 255.0;
  float blue = ((color >> 16) & 0xFF) / 255.0;
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
 * Sets the new control which is displayed as info bar at the top or bottom of the editor.
 * Set newBar to nil if you want to hide the bar again.
 * When aligned to bottom position then the info bar and the horizontal scroller share the available
 * space. The info bar will then only get the width it is currently set to less a minimal amount
 * reserved for the scroller. At the top position it gets the full width of the control.
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
      
      // Keep the initial width as reference for layout changes.
      mInitialInfoBarWidth = [mInfoBar frame].size.width;
    }
    
    [self layout];
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

- (void)insertText: (NSString*)text
{
  [mContent insertText: text];
}

//--------------------------------------------------------------------------------------------------

/**
 * Searches and marks the first occurance of the given text and optionally scrolls it into view.
 */
- (void) findAndHighlightText: (NSString*) searchText
                    matchCase: (BOOL) matchCase
                    wholeWord: (BOOL) wholeWord
                     scrollTo: (BOOL) scrollTo
                         wrap: (BOOL) wrap
{
  // The current position is where we start searching. That is either the end of the current
  // (main) selection or the caret position. That ensures we do proper "search next" too.
  int currentPosition = [self getGeneralProperty: SCI_GETCURRENTPOS parameter: 0];
  int length = [self getGeneralProperty: SCI_GETTEXTLENGTH parameter: 0];

  int searchFlags= 0;
  if (matchCase)
    searchFlags |= SCFIND_MATCHCASE;
  if (wholeWord)
    searchFlags |= SCFIND_WHOLEWORD;

  Sci_TextToFind ttf;
  ttf.chrg.cpMin = currentPosition;
  ttf.chrg.cpMax = length;
  ttf.lpstrText = (char*) [searchText UTF8String];
  int position = mBackend->WndProc(SCI_FINDTEXT, searchFlags, (sptr_t) &ttf);
  
  if (position < 0 && wrap)
  {
    ttf.chrg.cpMin = 0;
    ttf.chrg.cpMax = currentPosition;
    position = mBackend->WndProc(SCI_FINDTEXT, searchFlags, (sptr_t) &ttf);
  }
  
  if (position >= 0)
  {
    // Highlight the found text.
    [self setGeneralProperty: SCI_SETSELECTIONSTART
                       value: position];
    [self setGeneralProperty: SCI_SETSELECTIONEND
                       value: position + [searchText length]];
    
    if (scrollTo)
      [self setGeneralProperty: SCI_SCROLLCARET value: 0];
  }
}

//--------------------------------------------------------------------------------------------------

- (void) setFontName: (NSString*) font
                size: (int) size
                bold: (BOOL) bold
                italic: (BOOL) italic
{
  for (int i = 0; i < 32; i++)
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

