
/**
 * Scintilla source code edit control
 * ScintillaCocoa.mm - Cocoa subclass of ScintillaBase
 * 
 * Written by Mike Lischke <mlischke@sun.com>
 *
 * Loosely based on ScintillaMacOSX.cxx.
 * Copyright 2003 by Evan Jones <ejones@uwaterloo.ca>
 * Based on ScintillaGTK.cxx Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed.
  *
 * Copyright (c) 2009, 2010 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#import <Cocoa/Cocoa.h>

#import <Carbon/Carbon.h> // Temporary

#include "ScintillaView.h"
#include "PlatCocoa.h"

using namespace Scintilla;

#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif

NSString* ScintillaRecPboardType = @"com.scintilla.utf16-plain-text.rectangular";

//--------------------------------------------------------------------------------------------------

// Define keyboard shortcuts (equivalents) the Mac way.
#define SCI_CMD ( SCI_ALT | SCI_CTRL)
#define SCI_SCMD ( SCI_CMD | SCI_SHIFT)

static const KeyToCommand macMapDefault[] =
{
  {SCK_DOWN,      SCI_CMD,    SCI_DOCUMENTEND},
  {SCK_UP,        SCI_CMD,    SCI_DOCUMENTSTART},
  {SCK_LEFT,      SCI_CMD,    SCI_VCHOME},
  {SCK_LEFT,      SCI_SCMD,   SCI_VCHOMEEXTEND},
  {SCK_RIGHT,     SCI_CMD,    SCI_LINEEND},
  {SCK_RIGHT,     SCI_SCMD,   SCI_LINEENDEXTEND},
  {SCK_DOWN,      SCI_NORM,   SCI_LINEDOWN},
  {SCK_DOWN,      SCI_SHIFT,  SCI_LINEDOWNEXTEND},
  {SCK_DOWN,      SCI_CTRL,   SCI_LINESCROLLDOWN},
  {SCK_DOWN,      SCI_ASHIFT, SCI_LINEDOWNRECTEXTEND},
  {SCK_UP,        SCI_NORM,   SCI_LINEUP},
  {SCK_UP,        SCI_SHIFT,  SCI_LINEUPEXTEND},
  {SCK_UP,        SCI_CTRL,   SCI_LINESCROLLUP},
  {SCK_UP,        SCI_ASHIFT, SCI_LINEUPRECTEXTEND},
  {'[',           SCI_CTRL,   SCI_PARAUP},
  {'[',           SCI_CSHIFT, SCI_PARAUPEXTEND},
  {']',           SCI_CTRL,   SCI_PARADOWN},
  {']',           SCI_CSHIFT, SCI_PARADOWNEXTEND},
  {SCK_LEFT,      SCI_NORM,   SCI_CHARLEFT},
  {SCK_LEFT,      SCI_SHIFT,  SCI_CHARLEFTEXTEND},
  {SCK_LEFT,      SCI_ALT,    SCI_WORDLEFT},
  {SCK_LEFT,      SCI_CSHIFT, SCI_WORDLEFTEXTEND},
  {SCK_LEFT,      SCI_ASHIFT, SCI_WORDLEFTEXTEND},
  {SCK_RIGHT,     SCI_NORM,   SCI_CHARRIGHT},
  {SCK_RIGHT,     SCI_SHIFT,  SCI_CHARRIGHTEXTEND},
  {SCK_RIGHT,     SCI_ALT,    SCI_WORDRIGHT},
  {SCK_RIGHT,     SCI_CSHIFT, SCI_WORDRIGHTEXTEND},
  {SCK_RIGHT,     SCI_ASHIFT, SCI_WORDRIGHTEXTEND},
  {'/',           SCI_CTRL,   SCI_WORDPARTLEFT},
  {'/',           SCI_CSHIFT, SCI_WORDPARTLEFTEXTEND},
  {'\\',          SCI_CTRL,   SCI_WORDPARTRIGHT},
  {'\\',          SCI_CSHIFT, SCI_WORDPARTRIGHTEXTEND},
  {SCK_HOME,      SCI_NORM,   SCI_VCHOME},
  {SCK_HOME,      SCI_SHIFT,  SCI_VCHOMEEXTEND},
  {SCK_HOME,      SCI_CTRL,   SCI_DOCUMENTSTART},
  {SCK_HOME,      SCI_CSHIFT, SCI_DOCUMENTSTARTEXTEND},
  {SCK_HOME,      SCI_ALT,    SCI_HOMEDISPLAY},
  {SCK_HOME,      SCI_ASHIFT, SCI_VCHOMERECTEXTEND},
  {SCK_END,       SCI_NORM,   SCI_LINEEND},
  {SCK_END,       SCI_SHIFT,  SCI_LINEENDEXTEND},
  {SCK_END,       SCI_CTRL,   SCI_DOCUMENTEND},
  {SCK_END,       SCI_CSHIFT, SCI_DOCUMENTENDEXTEND},
  {SCK_END,       SCI_ALT,    SCI_LINEENDDISPLAY},
  {SCK_END,       SCI_ASHIFT, SCI_LINEENDRECTEXTEND},
  {SCK_PRIOR,     SCI_NORM,   SCI_PAGEUP},
  {SCK_PRIOR,     SCI_SHIFT,  SCI_PAGEUPEXTEND},
  {SCK_PRIOR,     SCI_ASHIFT, SCI_PAGEUPRECTEXTEND},
  {SCK_NEXT,      SCI_NORM,   SCI_PAGEDOWN},
  {SCK_NEXT,      SCI_SHIFT,  SCI_PAGEDOWNEXTEND},
  {SCK_NEXT,      SCI_ASHIFT, SCI_PAGEDOWNRECTEXTEND},
  {SCK_DELETE,    SCI_NORM,   SCI_CLEAR},
  {SCK_DELETE,    SCI_SHIFT,  SCI_CUT},
  {SCK_DELETE,    SCI_CTRL,   SCI_DELWORDRIGHT},
  {SCK_DELETE,    SCI_CSHIFT, SCI_DELLINERIGHT},
  {SCK_INSERT,    SCI_NORM,   SCI_EDITTOGGLEOVERTYPE},
  {SCK_INSERT,    SCI_SHIFT,  SCI_PASTE},
  {SCK_INSERT,    SCI_CTRL,   SCI_COPY},
  {SCK_ESCAPE,    SCI_NORM,   SCI_CANCEL},
  {SCK_BACK,      SCI_NORM,   SCI_DELETEBACK},
  {SCK_BACK,      SCI_SHIFT,  SCI_DELETEBACK},
  {SCK_BACK,      SCI_CTRL,   SCI_DELWORDLEFT},
  {SCK_BACK,      SCI_ALT,    SCI_UNDO},
  {SCK_BACK,      SCI_CSHIFT, SCI_DELLINELEFT},
  {'z',           SCI_CMD,    SCI_UNDO},
  {'z',           SCI_SCMD,   SCI_REDO},
  {'x',           SCI_CMD,    SCI_CUT},
  {'c',           SCI_CMD,    SCI_COPY},
  {'v',           SCI_CMD,    SCI_PASTE},
  {'a',           SCI_CMD,    SCI_SELECTALL},
  {SCK_TAB,       SCI_NORM,   SCI_TAB},
  {SCK_TAB,       SCI_SHIFT,  SCI_BACKTAB},
  {SCK_RETURN,    SCI_NORM,   SCI_NEWLINE},
  {SCK_RETURN,    SCI_SHIFT,  SCI_NEWLINE},
  {SCK_ADD,       SCI_CMD,    SCI_ZOOMIN},
  {SCK_SUBTRACT,  SCI_CMD,    SCI_ZOOMOUT},
  {SCK_DIVIDE,    SCI_CMD,    SCI_SETZOOM},
  {'l',           SCI_CMD,    SCI_LINECUT},
  {'l',           SCI_CSHIFT, SCI_LINEDELETE},
  {'t',           SCI_CSHIFT, SCI_LINECOPY},
  {'t',           SCI_CTRL,   SCI_LINETRANSPOSE},
  {'d',           SCI_CTRL,   SCI_SELECTIONDUPLICATE},
  {'u',           SCI_CTRL,   SCI_LOWERCASE},
  {'u',           SCI_CSHIFT, SCI_UPPERCASE},
  {0, 0, 0},
};

//--------------------------------------------------------------------------------------------------

@implementation TimerTarget

- (id) init: (void*) target
{
  [super init];
  if (self != nil)
  {
    mTarget = target;

    // Get the default notification queue for the thread which created the instance (usually the
    // main thread). We need that later for idle event processing.
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter]; 
    notificationQueue = [[NSNotificationQueue alloc] initWithNotificationCenter: center];
    [center addObserver: self selector: @selector(idleTriggered:) name: @"Idle" object: nil]; 
  }
  return self;
}

//--------------------------------------------------------------------------------------------------

- (void) dealloc
{
  [notificationQueue release];
  [super dealloc];
}

//--------------------------------------------------------------------------------------------------

/**
 * Method called by a timer installed by ScintillaCocoa. This two step approach is needed because
 * a native Obj-C class is required as target for the timer.
 */
- (void) timerFired: (NSTimer*) timer
{
  reinterpret_cast<ScintillaCocoa*>(mTarget)->TimerFired(timer);
}

//--------------------------------------------------------------------------------------------------

/**
 * Another timer callback for the idle timer.
 */
- (void) idleTimerFired: (NSTimer*) timer
{
  // Idle timer event.
  // Post a new idle notification, which gets executed when the run loop is idle.
  // Since we are coalescing on name and sender there will always be only one actual notification
  // even for multiple requests.
  NSNotification *notification = [NSNotification notificationWithName: @"Idle" object: self]; 
  [notificationQueue enqueueNotification: notification
                            postingStyle: NSPostWhenIdle
                            coalesceMask: (NSNotificationCoalescingOnName | NSNotificationCoalescingOnSender)
                                forModes: nil]; 
}

//--------------------------------------------------------------------------------------------------

/**
 * Another step for idle events. The timer (for idle events) simply requests a notification on
 * idle time. Only when this notification is send we actually call back the editor.
 */
- (void) idleTriggered: (NSNotification*) notification
{
  reinterpret_cast<ScintillaCocoa*>(mTarget)->IdleTimerFired();
}

@end

//----------------- ScintillaCocoa -----------------------------------------------------------------

ScintillaCocoa::ScintillaCocoa(NSView* view)
{
  wMain= [view retain];
  timerTarget = [[[TimerTarget alloc] init: this] retain];
  Initialise();
}

//--------------------------------------------------------------------------------------------------

ScintillaCocoa::~ScintillaCocoa()
{
  SetTicking(false);
  [timerTarget release];
  NSView* container = ContentView();
  [container release];
}

//--------------------------------------------------------------------------------------------------

/**
 * Core initialization of the control. Everything that needs to be set up happens here.
 */
void ScintillaCocoa::Initialise() 
{
  notifyObj = NULL;
  notifyProc = NULL;
  
  capturedMouse = false;
  
  // Tell Scintilla not to buffer: Quartz buffers drawing for us.
  WndProc(SCI_SETBUFFEREDDRAW, 0, 0);
  
  // We are working with Unicode exclusively.
  WndProc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);

  // Add Mac specific key bindings.
  for (int i = 0; macMapDefault[i].key; i++) 
    kmap.AssignCmdKey(macMapDefault[i].key, macMapDefault[i].modifiers, macMapDefault[i].msg);
  
}

//--------------------------------------------------------------------------------------------------

/**
 * We need some clean up. Do it here.
 */
void ScintillaCocoa::Finalise()
{
  SetTicking(false);
  ScintillaBase::Finalise();
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to get the outer container which represents the Scintilla editor on application side.
 */
ScintillaView* ScintillaCocoa::TopContainer()
{
  NSView* container = static_cast<NSView*>(wMain.GetID());
  return static_cast<ScintillaView*>([container superview]);
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper function to get the inner container which represents the actual "canvas" we work with.
 */
NSView* ScintillaCocoa::ContentView()
{
  return static_cast<NSView*>(wMain.GetID());
}

//--------------------------------------------------------------------------------------------------

/**
 * Instead of returning the size of the inner view we have to return the visible part of it
 * in order to make scrolling working properly.
 */
PRectangle ScintillaCocoa::GetClientRectangle()
{
  NSView* host = ContentView();
  NSSize size = [host frame].size;
  return PRectangle(0, 0, size.width, size.height);
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the given point from base coordinates to local coordinates and at the same time into
 * a native Point structure. Base coordinates are used for the top window used in the view hierarchy. 
 */
Scintilla::Point ScintillaCocoa::ConvertPoint(NSPoint point)
{
  NSView* container = ContentView();
  NSPoint result = [container convertPoint: point fromView: nil];
  
  return Point(result.x, result.y);
}

//--------------------------------------------------------------------------------------------------

/**
 * A function to directly execute code that would usually go the long way via window messages.
 * However this is a Windows metapher and not used here, hence we just call our fake
 * window proc. The given parameters directly reflect the message parameters used on Windows.
 *
 * @param sciThis The target which is to be called.
 * @param iMessage A code that indicates which message was sent.
 * @param wParam One of the two free parameters for the message. Traditionally a word sized parameter 
 *               (hence the w prefix).
 * @param lParam The other of the two free parameters. A signed long.
 */
sptr_t ScintillaCocoa::DirectFunction(ScintillaCocoa *sciThis, unsigned int iMessage, uptr_t wParam, 
                                      sptr_t lParam)
{
  return sciThis->WndProc(iMessage, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

/**
 * This method is very similar to DirectFunction. On Windows it sends a message (not in the Obj-C sense)
 * to the target window. Here we simply call our fake window proc.
 */
sptr_t scintilla_send_message(void* sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam)
{
  ScintillaView *control = reinterpret_cast<ScintillaView*>(sci);
  ScintillaCocoa* scintilla = [control backend];
  return scintilla->WndProc(iMessage, wParam, lParam);
}

//--------------------------------------------------------------------------------------------------

/** 
 * That's our fake window procedure. On Windows each window has a dedicated procedure to handle
 * commands (also used to synchronize UI and background threads), which is not the case in Cocoa.
 * 
 * Messages handled here are almost solely for special commands of the backend. Everything which
 * would be sytem messages on Windows (e.g. for key down, mouse move etc.) are handled by
 * directly calling appropriate handlers.
 */
sptr_t ScintillaCocoa::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam)
{
  switch (iMessage)
  {
    case SCI_GETDIRECTFUNCTION:
      return reinterpret_cast<sptr_t>(DirectFunction);
      
    case SCI_GETDIRECTPOINTER:
      return reinterpret_cast<sptr_t>(this);
      
    case SCI_GRABFOCUS:
      // TODO: implement it
      break;

    case WM_UNICHAR: 
      // Special case not used normally. Characters passed in this way will be inserted
      // regardless of their value or modifier states. That means no command interpretation is
      // performed.
      if (IsUnicodeMode())
      {
        NSString* input = [[NSString stringWithCharacters: (const unichar*) &wParam length: 1] autorelease];
        const char* utf8 = [input UTF8String];
        AddCharUTF((char*) utf8, strlen(utf8), false);
        return 1;
      }
      return 0;
      
    default:
      unsigned int r = ScintillaBase::WndProc(iMessage, wParam, lParam);
      
      return r;
  }
  return 0l;
}

//--------------------------------------------------------------------------------------------------

/**
 * In Windows lingo this is the handler which handles anything that wasn't handled in the normal 
 * window proc which would usually send the message back to generic window proc that Windows uses.
 */
sptr_t ScintillaCocoa::DefWndProc(unsigned int, uptr_t, sptr_t)
{
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Enables or disables a timer that can trigger background processing at a regular interval, like
 * drag scrolling or caret blinking.
 */
void ScintillaCocoa::SetTicking(bool on)
{
  if (timer.ticking != on)
  {
    timer.ticking = on;
    if (timer.ticking)
    {
      // Scintilla ticks = milliseconds
      // Using userInfo as flag to distinct between tick and idle timer.
      NSTimer* tickTimer = [NSTimer scheduledTimerWithTimeInterval: timer.tickSize / 1000.0
                                                            target: timerTarget
                                                          selector: @selector(timerFired:)
                                                          userInfo: nil
                                                           repeats: YES];
      timer.tickerID = reinterpret_cast<TickerID>(tickTimer);
    }
    else
      if (timer.tickerID != NULL)
      {
        [reinterpret_cast<NSTimer*>(timer.tickerID) invalidate];
        timer.tickerID = 0;
      }
  }
  timer.ticksToWait = caret.period;
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::SetIdle(bool on)
{
  if (idler.state != on)
  {
    idler.state = on;
    if (idler.state)
    {
      // Scintilla ticks = milliseconds
      NSTimer* idleTimer = [NSTimer scheduledTimerWithTimeInterval: timer.tickSize / 1000.0
                                                            target: timerTarget
                                                          selector: @selector(idleTimerFired:)
                                                          userInfo: nil
                                                           repeats: YES];
      idler.idlerID = reinterpret_cast<IdlerID>(idleTimer);
    }
    else
      if (idler.idlerID != NULL)
      {
        [reinterpret_cast<NSTimer*>(idler.idlerID) invalidate];
        idler.idlerID = 0;
      }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::CopyToClipboard(const SelectionText &selectedText)
{
  SetPasteboardData([NSPasteboard generalPasteboard], selectedText);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Copy()
{
  if (!sel.Empty())
  {
    SelectionText selectedText;
    CopySelectionRange(&selectedText);
    CopyToClipboard(selectedText);
  }
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanPaste()
{
  if (!Editor::CanPaste())
    return false;
  
  return GetPasteboardData([NSPasteboard generalPasteboard], NULL);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Paste()
{
  Paste(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Pastes data from the paste board into the editor.
 */
void ScintillaCocoa::Paste(bool forceRectangular)
{
  SelectionText selectedText;
  bool ok = GetPasteboardData([NSPasteboard generalPasteboard], &selectedText);
  if (forceRectangular)
    selectedText.rectangular = forceRectangular;
  
  if (!ok || !selectedText.s)
    // No data or no flavor we support.
    return;
  
  pdoc->BeginUndoAction();
  ClearSelection();
  if (selectedText.rectangular)
  {
    SelectionPosition selStart = sel.RangeMain().Start();
    PasteRectangular(selStart, selectedText.s, selectedText.len);
  }
  else 
    if (pdoc->InsertString(sel.RangeMain().caret.Position(), selectedText.s, selectedText.len))
      SetEmptySelection(sel.RangeMain().caret.Position() + selectedText.len);
  
  pdoc->EndUndoAction();
  
  Redraw();
  EnsureCaretVisible();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::CreateCallTipWindow(PRectangle rc)
{
/*
  // create a calltip window
  if (!ct.wCallTip.Created()) {
    WindowClass windowClass = kHelpWindowClass;
    WindowAttributes attributes = kWindowNoAttributes;
    Rect contentBounds;
    WindowRef outWindow;
    
    // convert PRectangle to Rect
    // this adjustment gets the calltip window placed in the correct location relative
    // to our editor window
    Rect bounds;
    OSStatus err;
    err = GetWindowBounds( this->GetOwner(), kWindowGlobalPortRgn, &bounds );
    assert( err == noErr );
    contentBounds.top = rc.top + bounds.top;
    contentBounds.bottom = rc.bottom + bounds.top;
    contentBounds.right = rc.right + bounds.left;
    contentBounds.left = rc.left + bounds.left;
    
    // create our calltip hiview
    HIViewRef ctw = scintilla_calltip_new();
    CallTip* objectPtr = &ct;
    ScintillaCocoa* sciThis = this;
    SetControlProperty( ctw, scintillaMacOSType, 0, sizeof( this ), &sciThis );
    SetControlProperty( ctw, scintillaCallTipType, 0, sizeof( objectPtr ), &objectPtr );
    
    CreateNewWindow(windowClass, attributes, &contentBounds, &outWindow);
    ControlRef root;
    CreateRootControl(outWindow, &root);
    
    HIViewRef hiroot = HIViewGetRoot (outWindow);
    HIViewAddSubview(hiroot, ctw);
    
    HIRect boundsRect;
    HIViewGetFrame(hiroot, &boundsRect);
    HIViewSetFrame( ctw, &boundsRect );
    
    // bind the size of the calltip to the size of it's container window
    HILayoutInfo layout = {
      kHILayoutInfoVersionZero,
      {
        { NULL, kHILayoutBindTop, 0 },
        { NULL, kHILayoutBindLeft, 0 },
        { NULL, kHILayoutBindBottom, 0 },
        { NULL, kHILayoutBindRight, 0 }
      },
      {
        { NULL, kHILayoutScaleAbsolute, 0 },
        { NULL, kHILayoutScaleAbsolute, 0 }
        
      },
      {
        { NULL, kHILayoutPositionTop, 0 },
        { NULL, kHILayoutPositionLeft, 0 }
      }
    };
    HIViewSetLayoutInfo(ctw, &layout);
    
    ct.wCallTip = root;
    ct.wDraw = ctw;
    ct.wCallTip.SetWindow(outWindow);
    HIViewSetVisible(ctw,true);
    
  }
*/
}


void ScintillaCocoa::AddToPopUp(const char *label, int cmd, bool enabled)
{
  NSMenuItem* item;
  ScintillaContextMenu *menu= reinterpret_cast<ScintillaContextMenu*>(popup.GetID());
  [menu setOwner: this];
  [menu setAutoenablesItems: NO];
  
  if (cmd == 0)
    item = [NSMenuItem separatorItem];
  else
    item = [[NSMenuItem alloc] init];
  
  [item setTarget: menu];
  [item setAction: @selector(handleCommand:)];
  [item setTag: cmd];
  [item setTitle: [NSString stringWithUTF8String: label]];
  [item setEnabled: enabled];
  
  [menu addItem: item];
}

// -------------------------------------------------------------------------------------------------

void ScintillaCocoa::ClaimSelection()
{
  // Mac OS X does not have a primary selection.
}

// -------------------------------------------------------------------------------------------------

/**
 * Returns the current caret position (which is tracked as an offset into the entire text string)
 * as a row:column pair. The result is zero-based.
 */
NSPoint ScintillaCocoa::GetCaretPosition()
{
  NSPoint result;

  result.y = pdoc->LineFromPosition(sel.RangeMain().caret.Position());
  result.x = sel.RangeMain().caret.Position() - pdoc->LineStart(result.y);
  return result;
}

// -------------------------------------------------------------------------------------------------

#pragma segment Drag

/**
 * Triggered by the tick timer on a regular basis to scroll the content during a drag operation.
 */
void ScintillaCocoa::DragScroll()
{
  if (!posDrag.IsValid())
  {
    scrollSpeed = 1;
    scrollTicks = 2000;
    return;
  }

  // TODO: does not work for wrapped lines, fix it.
  int line = pdoc->LineFromPosition(posDrag.Position());
  int currentVisibleLine = cs.DisplayFromDoc(line);
  int lastVisibleLine = Platform::Minimum(topLine + LinesOnScreen(), cs.LinesDisplayed()) - 2;
    
  if (currentVisibleLine <= topLine && topLine > 0)
    ScrollTo(topLine - scrollSpeed);
  else
    if (currentVisibleLine >= lastVisibleLine)
      ScrollTo(topLine + scrollSpeed);
    else
    {
      scrollSpeed = 1;
      scrollTicks = 2000;
      return;
    }
  
  // TODO: also handle horizontal scrolling.
  
  if (scrollSpeed == 1)
  {
    scrollTicks -= timer.tickSize;
    if (scrollTicks <= 0)
    {
      scrollSpeed = 5;
      scrollTicks = 2000;
    }
  }
  
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when a drag operation was initiated from within Scintilla.
 */
void ScintillaCocoa::StartDrag()
{
  if (sel.Empty())
    return;

  // Put the data to be dragged on the drag pasteboard.
  SelectionText selectedText;
  NSPasteboard* pasteboard = [NSPasteboard pasteboardWithName: NSDragPboard];
  CopySelectionRange(&selectedText);
  SetPasteboardData(pasteboard, selectedText);
  
  // Prepare drag image.
  PRectangle localRectangle = RectangleFromRange(sel.RangeMain().Start().Position(), sel.RangeMain().End().Position());
  NSRect selectionRectangle = PRectangleToNSRect(localRectangle);
  
  NSView* content = ContentView();

#if 0 // TODO: fix initialization of the drag image with CGImageRef.
  // To get a bitmap of the text we're dragging, we just use Paint on a pixmap surface.
  SurfaceImpl *sw = new SurfaceImpl();
  SurfaceImpl *pixmap = NULL;
  
  bool lastHideSelection = hideSelection;
  hideSelection = true;
  if (sw)
  {
    pixmap = new SurfaceImpl();
    if (pixmap)
    {
      PRectangle client = GetClientRectangle();
      PRectangle imageRect = NSRectToPRectangle(selectionRectangle);
      paintState = painting;
      //sw->InitPixMap(client.Width(), client.Height(), NULL, NULL);
      sw->InitPixMap(imageRect.Width(), imageRect.Height(), NULL, NULL);
      paintingAllText = true;
      Paint(sw, imageRect);
      paintState = notPainting;
      
      pixmap->InitPixMap(imageRect.Width(), imageRect.Height(), NULL, NULL);
      
      CGContextRef gc = pixmap->GetContext(); 
      
      // To make Paint() work on a bitmap, we have to flip our coordinates and translate the origin
      CGContextTranslateCTM(gc, 0, imageRect.Height());
      CGContextScaleCTM(gc, 1.0, -1.0);
      
      pixmap->CopyImageRectangle(*sw, imageRect, PRectangle(0, 0, imageRect.Width(), imageRect.Height()));
      // XXX TODO: overwrite any part of the image that is not part of the
      //           selection to make it transparent.  right now we just use
      //           the full rectangle which may include non-selected text.
    }
    sw->Release();
    delete sw;
  }
  hideSelection = lastHideSelection;
  
  NSBitmapImageRep* bitmap = NULL;
  if (pixmap)
  {
    bitmap = [[[NSBitmapImageRep alloc] initWithCGImage: pixmap->GetImage()] autorelease];
    pixmap->Release();
    delete pixmap;
  }
#else
  
  // Poor man's drag image: take a snapshot of the content view.
  [content lockFocus];
  NSBitmapImageRep* bitmap = [[[NSBitmapImageRep alloc] initWithFocusedViewRect: selectionRectangle] autorelease];
  [bitmap setColorSpaceName: NSDeviceRGBColorSpace];
  [content unlockFocus];
  
#endif
  
  NSImage* image = [[[NSImage alloc] initWithSize: selectionRectangle.size] autorelease];
  [image addRepresentation: bitmap];
  
  NSImage* dragImage = [[[NSImage alloc] initWithSize: selectionRectangle.size] autorelease];
  [dragImage setBackgroundColor: [NSColor clearColor]];
  [dragImage lockFocus];
  [image dissolveToPoint: NSMakePoint(0.0, 0.0) fraction: 0.5];
  [dragImage unlockFocus];
  
  NSPoint startPoint;
  startPoint.x = selectionRectangle.origin.x;
  startPoint.y = selectionRectangle.origin.y + selectionRectangle.size.height;
  [content dragImage: dragImage 
                  at: startPoint
              offset: NSZeroSize
               event: lastMouseEvent // Set in MouseMove.
          pasteboard: pasteboard
              source: content
           slideBack: YES];
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when a drag operation reaches the control which was initiated outside.
 */
NSDragOperation ScintillaCocoa::DraggingEntered(id <NSDraggingInfo> info)
{
  inDragDrop = ddDragging;
  return DraggingUpdated(info);
}

//--------------------------------------------------------------------------------------------------

/**
 * Called frequently during a drag operation if we are the target. Keep telling the caller
 * what drag operation we accept and update the drop caret position to indicate the
 * potential insertion point of the dragged data.
 */
NSDragOperation ScintillaCocoa::DraggingUpdated(id <NSDraggingInfo> info)
{
  // Convert the drag location from window coordinates to view coordinates and 
  // from there to a text position to finally set the drag position.
  Point location = ConvertPoint([info draggingLocation]);
  SetDragPosition(SPositionFromLocation(location));
  
  NSDragOperation sourceDragMask = [info draggingSourceOperationMask];
  if (sourceDragMask == NSDragOperationNone)
    return sourceDragMask;
  
  NSPasteboard* pasteboard = [info draggingPasteboard];
  
  // Return what type of operation we will perform. Prefer move over copy.
  if ([[pasteboard types] containsObject: NSStringPboardType] ||
      [[pasteboard types] containsObject: ScintillaRecPboardType])
    return (sourceDragMask & NSDragOperationMove) ? NSDragOperationMove : NSDragOperationCopy;
  
  if ([[pasteboard types] containsObject: NSFilenamesPboardType])
    return (sourceDragMask & NSDragOperationGeneric);
  return NSDragOperationNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Resets the current drag position as we are no longer the drag target.
 */
void ScintillaCocoa::DraggingExited(id <NSDraggingInfo> info)
{
  SetDragPosition(SelectionPosition(invalidPosition));
  inDragDrop = ddNone;
}

//--------------------------------------------------------------------------------------------------

/**
 * Here is where the real work is done. Insert the text from the pasteboard.
 */
bool ScintillaCocoa::PerformDragOperation(id <NSDraggingInfo> info)
{
  NSPasteboard* pasteboard = [info draggingPasteboard];
  
  if ([[pasteboard types] containsObject: NSFilenamesPboardType])
  {
    NSArray* files = [pasteboard propertyListForType: NSFilenamesPboardType];
    for (NSString* uri in files)
      NotifyURIDropped([uri UTF8String]);
  }
  else
  {
    SelectionText text;
    GetPasteboardData(pasteboard, &text);
    
    if (text.len > 0)
    {
      NSDragOperation operation = [info draggingSourceOperationMask];
      bool moving = (operation & NSDragOperationMove) != 0;
      
      DropAt(posDrag, text.s, moving, text.rectangular);
    };
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetPasteboardData(NSPasteboard* board, const SelectionText &selectedText)
{
  if (selectedText.len == 0)
    return;

  NSString *string;
  string = [NSString stringWithUTF8String: selectedText.s];

  [board declareTypes:[NSArray arrayWithObjects:
                       NSStringPboardType,
                       selectedText.rectangular ? ScintillaRecPboardType : nil,
                       nil] owner:nil];
  
  if (selectedText.rectangular)
  {
    // This is specific to scintilla, allows us to drag rectangular selections around the document.
    [board setString: string forType: ScintillaRecPboardType];
  }
  
  [board setString: string forType: NSStringPboardType];
  
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper method to retrieve the best fitting alternative from the general pasteboard.
 */
bool ScintillaCocoa::GetPasteboardData(NSPasteboard* board, SelectionText* selectedText)
{
  NSArray* supportedTypes = [NSArray arrayWithObjects: ScintillaRecPboardType, 
                             NSStringPboardType, 
                             nil];
  NSString *bestType = [board availableTypeFromArray: supportedTypes];
  NSString* data = [board stringForType: bestType];
  
  if (data != nil)
  {
    if (selectedText != nil)
    {
      char* text = (char*) [data UTF8String];
      bool rectangular = bestType == ScintillaRecPboardType;
      selectedText->Copy(text, strlen(text) + 1, SC_CP_UTF8, SC_CHARSET_DEFAULT , rectangular, false);
    }
    return true;
  }
  
  return false;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetMouseCapture(bool on)
{
  capturedMouse = on;
  /*
  if (mouseDownCaptures)
  {
    if (capturedMouse)
      WndProc(SCI_SETCURSOR, Window::cursorArrow, 0);
    else
      // Reset to normal. Actual image will be set on mouse move.
      WndProc(SCI_SETCURSOR, (unsigned int) SC_CURSORNORMAL, 0);
  }
   */
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::HaveMouseCapture()
{
  return capturedMouse;
}

//--------------------------------------------------------------------------------------------------

/**
 * Synchronously paint a rectangle of the window.
 */
void ScintillaCocoa::SyncPaint(void* gc, PRectangle rc)
{
  paintState = painting;
  rcPaint = rc;
  PRectangle rcText = GetTextRectangle();
  paintingAllText = rcPaint.Contains(rcText);
  Surface *sw = Surface::Allocate();
  if (sw)
  {
    sw->Init(gc, wMain.GetID());
    Paint(sw, rc);
    if (paintState == paintAbandoned)
    {
      // Do a full paint.
      rcPaint = GetClientRectangle();
      paintState = painting;
      paintingAllText = true;
      Paint(sw, rcPaint);
    }
    sw->Release();
    delete sw;
  }
  paintState = notPainting;
}

//--------------------------------------------------------------------------------------------------

/**
 * Modfies the vertical scroll position to make the current top line show up as such.
 */
void ScintillaCocoa::SetVerticalScrollPos()
{
  ScintillaView* topContainer = TopContainer();
  
  // Convert absolute coordinate into the range [0..1]. Keep in mind that the visible area
  // does *not* belong to the scroll range.
  float relativePosition = (float) topLine / MaxScrollPos();
  [topContainer setVerticalScrollPosition: relativePosition];
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::SetHorizontalScrollPos()
{
  ScintillaView* topContainer = TopContainer();
  PRectangle textRect = GetTextRectangle();
  
  // Convert absolute coordinate into the range [0..1]. Keep in mind that the visible area
  // does *not* belong to the scroll range.
  float relativePosition = (float) xOffset / (scrollWidth - textRect.Width());
  [topContainer setHorizontalScrollPosition: relativePosition];
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to adjust both scrollers to reflect the current scroll range and position in the editor.
 *
 * @param nMax Number of lines in the editor.
 * @param nPage Number of lines per scroll page.
 * @return True if there was a change, otherwise false.
 */
bool ScintillaCocoa::ModifyScrollBars(int nMax, int nPage)
{
  // Input values are given in lines, not pixels, so we have to convert.
  int lineHeight = WndProc(SCI_TEXTHEIGHT, 0, 0);
  PRectangle bounds = GetTextRectangle();
  ScintillaView* topContainer = TopContainer();

  // Set page size to the same value as the scroll range to hide the scrollbar.
  int scrollRange = lineHeight * (nMax + 1); // +1 because the caller subtracted one.
  int pageSize;
  if (verticalScrollBarVisible)
    pageSize = bounds.Height();
  else
    pageSize = scrollRange;
  bool verticalChange = [topContainer setVerticalScrollRange: scrollRange page: pageSize];
  
  scrollRange = scrollWidth;
  if (horizontalScrollBarVisible)
    pageSize = bounds.Width();
  else
    pageSize = scrollRange;
  bool horizontalChange = [topContainer setHorizontalScrollRange: scrollRange page: pageSize];
  
  return verticalChange || horizontalChange;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::Resize()
{
  ChangeSize();
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the frontend control when the user manipulates one of the scrollers.
 *
 * @param position The relative position of the scroller in the range of [0..1].
 * @param part Specifies which part was clicked on by the user, so we can handle thumb tracking
 *             as well as page and line scrolling.
 * @param horizontal True if the horizontal scroller was hit, otherwise false.
 */
void ScintillaCocoa::DoScroll(float position, NSScrollerPart part, bool horizontal)
{
  // If the given scroller part is not the knob (or knob slot) then the given position is not yet
  // current and we have to update it.
  if (horizontal)
  {
    // Horizontal offset is given in pixels.
    PRectangle textRect = GetTextRectangle();
    int offset = (int) (position * (scrollWidth - textRect.Width()));
    int smallChange = (int) (textRect.Width() / 30);
    if (smallChange < 5)
      smallChange = 5;
    switch (part)
    {
      case NSScrollerDecrementLine:
        offset -= smallChange;
        break;
      case NSScrollerDecrementPage:
        offset -= textRect.Width();
        break;
      case NSScrollerIncrementLine:
        offset += smallChange;
        break;
      case NSScrollerIncrementPage:
        offset += textRect.Width();
        break;
    };
    HorizontalScrollTo(offset);
  }
  else
  {
    // VerticalScrolling is by line.
    int topLine = (int) (position * MaxScrollPos());
    int page = LinesOnScreen();
    switch (part)
    {
      case NSScrollerDecrementLine:
        topLine--;
        break;
      case NSScrollerDecrementPage:
        topLine -= page;
        break;
      case NSScrollerIncrementLine:
        topLine++;
        break;
      case NSScrollerIncrementPage:
        topLine += page;
        break;
    };
    ScrollTo(topLine, true);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to register a callback function for a given window. This is used to emulate the way
 * Windows notfies other controls (mainly up in the view hierarchy) about certain events.
 *
 * @param windowid A handle to a window. That value is generic and can be anything. It is passed
 *                 through to the callback.
 * @param callback The callback function to be used for future notifications. If NULL then no
 *                 notifications will be sent anymore.
 */
void ScintillaCocoa::RegisterNotifyCallback(intptr_t windowid, SciNotifyFunc callback)
{
  notifyObj = windowid;
  notifyProc = callback;
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyChange()
{
  if (notifyProc != NULL)
    notifyProc(notifyObj, WM_COMMAND, (uintptr_t) (SCEN_CHANGE << 16), (uintptr_t) this);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyFocus(bool focus)
{
  if (notifyProc != NULL)
    notifyProc(notifyObj, WM_COMMAND, (uintptr_t) ((focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS) << 16), (uintptr_t) this);
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to send a notification (as WM_NOTIFY call) to the procedure, which has been set by the call
 * to RegisterNotifyCallback (so it is not necessarily the parent window).
 *
 * @param scn The notification to send.
 */
void ScintillaCocoa::NotifyParent(SCNotification scn)
{ 
  if (notifyProc != NULL)
  {
    scn.nmhdr.hwndFrom = (void*) this;
    scn.nmhdr.idFrom = (unsigned int) wMain.GetID();
    notifyProc(notifyObj, WM_NOTIFY, (uintptr_t) 0, (uintptr_t) &scn);
  }
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::NotifyURIDropped(const char *uri)
{
  SCNotification scn;
  scn.nmhdr.code = SCN_URIDROPPED;
  scn.text = uri;
  
  NotifyParent(scn);
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::HasSelection()
{
  return !sel.Empty();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanUndo()
{
  return pdoc->CanUndo();
}

//--------------------------------------------------------------------------------------------------

bool ScintillaCocoa::CanRedo()
{
  return pdoc->CanRedo();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::TimerFired(NSTimer* timer)
{
  Tick();
  DragScroll();
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::IdleTimerFired()
{
  bool more = Idle();
  if (!more)
    SetIdle(false);
}

//--------------------------------------------------------------------------------------------------

/**
 * Main entry point for drawing the control.
 *
 * @param rect The area to paint, given in the sender's coordinate.
 * @param gc The context we can use to paint.
 */
void ScintillaCocoa::Draw(NSRect rect, CGContextRef gc)
{
  SyncPaint(gc, NSRectToPRectangle(rect));
}

//--------------------------------------------------------------------------------------------------
    
/**
 * Helper function to translate OS X key codes to Scintilla key codes.
 */
static inline UniChar KeyTranslate(UniChar unicodeChar)
{
  switch (unicodeChar)
  {
    case NSDownArrowFunctionKey:
      return SCK_DOWN;
    case NSUpArrowFunctionKey:
      return SCK_UP;
    case NSLeftArrowFunctionKey:
      return SCK_LEFT;
    case NSRightArrowFunctionKey:
      return SCK_RIGHT;
    case NSHomeFunctionKey:
      return SCK_HOME;
    case NSEndFunctionKey:
      return SCK_END;
    case NSPageUpFunctionKey:
      return SCK_PRIOR;
    case NSPageDownFunctionKey:
      return SCK_NEXT;
    case NSDeleteFunctionKey:
      return SCK_DELETE;
    case NSInsertFunctionKey:
      return SCK_INSERT;
    case '\n':
    case 3:
      return SCK_RETURN;
    case 27:
      return SCK_ESCAPE;
    case 127:
      return SCK_BACK;
    case '\t':
    case 25: // Shift tab, return to unmodified tab and handle that via modifiers.
      return SCK_TAB;
    default:
      return unicodeChar;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Main keyboard input handling method. It is called for any key down event, including function keys,
 * numeric keypad input and whatnot. 
 *
 * @param event The event instance associated with the key down event.
 * @return True if the input was handled, false otherwise.
 */
bool ScintillaCocoa::KeyboardInput(NSEvent* event)
{
  // For now filter out function keys.
  NSUInteger modifiers = [event modifierFlags];
  
  NSString* input = [event characters];
  
  bool control = (modifiers & NSControlKeyMask) != 0;
  bool shift = (modifiers & NSShiftKeyMask) != 0;
  bool command = (modifiers & NSCommandKeyMask) != 0;
  bool alt = (modifiers & NSAlternateKeyMask) != 0;
  
  bool handled = false;
  
  // Handle each entry individually. Usually we only have one entry anway.
  for (int i = 0; i < input.length; i++)
  {
    const UniChar originalKey = [input characterAtIndex: i];
    UniChar key = KeyTranslate(originalKey);
    
    bool consumed = false; // Consumed as command?
    
    // Signal command as control + alt. This leaves us without command + control and command + alt
    // but that's what we get when we have a modifier key more than other platforms.
    if (KeyDown(key, shift, control || command, alt || command, &consumed))
      handled = true;
    if (consumed)
      handled = true;
  }
  
  return handled;
}

//--------------------------------------------------------------------------------------------------

/**
 * Used to insert already processed text provided by the Cocoa text input system.
 */
int ScintillaCocoa::InsertText(NSString* input)
{
  const char* utf8 = [input UTF8String];
  AddCharUTF((char*) utf8, strlen(utf8), false);
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the owning view when the mouse pointer enters the control.
 */
void ScintillaCocoa::MouseEntered(NSEvent* event)
{
  if (!HaveMouseCapture())
  {
    WndProc(SCI_SETCURSOR, (long int)SC_CURSORNORMAL, 0);
    
    // Mouse location is given in screen coordinates and might also be outside of our bounds.
    Point location = ConvertPoint([event locationInWindow]);
    ButtonMove(location);
  }
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseExited(NSEvent* event)
{
  // Nothing to do here.
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseDown(NSEvent* event)
{
  Point location = ConvertPoint([event locationInWindow]);
  NSTimeInterval time = [event timestamp];
  bool command = ([event modifierFlags] & NSCommandKeyMask) != 0;
  bool shift = ([event modifierFlags] & NSShiftKeyMask) != 0;
  bool control = ([event modifierFlags] & NSControlKeyMask) != 0;
    
  ButtonDown(Point(location.x, location.y), (int) (time * 1000), shift, control, command);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseMove(NSEvent* event)
{
  lastMouseEvent = event;
  
  ButtonMove(ConvertPoint([event locationInWindow]));
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseUp(NSEvent* event)
{
  NSTimeInterval time = [event timestamp];
  bool control = ([event modifierFlags] & NSControlKeyMask) != 0;

  ButtonUp(ConvertPoint([event locationInWindow]), (int) (time * 1000), control);
}

//--------------------------------------------------------------------------------------------------

void ScintillaCocoa::MouseWheel(NSEvent* event)
{
  bool command = ([event modifierFlags] & NSCommandKeyMask) != 0;
  bool shift = ([event modifierFlags] & NSShiftKeyMask) != 0;
  int delta;
  if (shift)
    delta = 10 * [event deltaX]; // Arbitrary scale factor.
  else
  {
    // In order to make scrolling with larger offset smoother we scroll less lines the larger the 
    // delta value is.
    if ([event deltaY] < 0)
      delta = -(int) sqrt(-10.0 * [event deltaY]);
    else
      delta = (int) sqrt(10.0 * [event deltaY]);
  }
  
  if (command)
  {
    // Zoom! We play with the font sizes in the styles.
    // Number of steps/line is ignored, we just care if sizing up or down.
    if (delta > 0)
      KeyCommand(SCI_ZOOMIN);
    else
      KeyCommand(SCI_ZOOMOUT);
  }
  else
    if (shift)
      HorizontalScrollTo(xOffset - delta);
    else
      ScrollTo(topLine - delta, true);
}

//--------------------------------------------------------------------------------------------------

// Helper methods for NSResponder actions.

void ScintillaCocoa::SelectAll()
{
  Editor::SelectAll();
}

void ScintillaCocoa::DeleteBackward()
{
  KeyDown(SCK_BACK, false, false, false, nil);
}

void ScintillaCocoa::Cut()
{
  Editor::Cut();
}

void ScintillaCocoa::Undo()
{
  Editor::Undo();
}

void ScintillaCocoa::Redo()
{
  Editor::Undo();
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates and returns a popup menu, which is then displayed by the Cocoa framework.
 */
NSMenu* ScintillaCocoa::CreateContextMenu(NSEvent* event)
{
  // Call ScintillaBase to create the context menu.
  ContextMenu(Point(0, 0));
  
  return reinterpret_cast<NSMenu*>(popup.GetID());
}

//--------------------------------------------------------------------------------------------------

/**
 * An intermediate function to forward context menu commands from the menu action handler to
 * scintilla.
 */
void ScintillaCocoa::HandleCommand(NSInteger command)
{
  Command(command);
}

//--------------------------------------------------------------------------------------------------

//OSStatus ScintillaCocoa::ActiveStateChanged()
//{
//  // If the window is being deactivated, lose the focus and turn off the ticking
//  if ( ! this->IsActive() ) {
//    DropCaret();
//    //SetFocusState( false );
//    SetTicking( false );
//  } else {
//    ShowCaretAtCurrentPosition();
//  }
//  return noErr;
//}
//

//--------------------------------------------------------------------------------------------------

