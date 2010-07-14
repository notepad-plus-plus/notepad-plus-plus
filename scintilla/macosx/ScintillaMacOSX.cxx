// Scintilla source code edit control
// ScintillaMacOSX.cxx - Mac OS X subclass of ScintillaBase
// Copyright 2003 by Evan Jones <ejones@uwaterloo.ca>
// Based on ScintillaGTK.cxx Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.


#include "ScintillaMacOSX.h"
#ifdef EXT_INPUT
// External Input Editor
#include "ExtInput.h"
#else
#include "UniConversion.h"
#endif

using namespace Scintilla;

const CFStringRef ScintillaMacOSX::kScintillaClassID = CFSTR( "org.scintilla.scintilla" );
const ControlKind ScintillaMacOSX::kScintillaKind = { 'ejon', 'Scin' };

extern "C" HIViewRef scintilla_calltip_new(void);

#ifndef WM_UNICHAR
#define WM_UNICHAR                      0x0109
#endif

// required for paste/dragdrop, see comment in paste function below
static int BOMlen(unsigned char *cstr) {
  switch(cstr[0]) { 
  case 0xEF: // BOM_UTF8
    if (cstr[1] == 0xBB && cstr[2] == 0xBF) {
      return 3;
    }
    break;
  case 0xFE:
    if (cstr[1] == 0xFF) {
      if (cstr[2] == 0x00 && cstr[3] == 0x00) {
        return 4;
      }
      return 2;
    }
    break;
  case 0xFF:
    if (cstr[1] == 0xFE) {
      if (cstr[2] == 0x00 && cstr[3] == 0x00) {
        return 4;
      }
      return 2;
    }
    break;
  case 0x00:
    if (cstr[1] == 0x00) {
      if (cstr[2] == 0xFE && cstr[3] == 0xFF) {
        return 4;
      }
      if (cstr[2] == 0xFF && cstr[3] == 0xFE) {
        return 4;
      }
      return 2;
    }
    break;
  }

  return 0;
}

#ifdef EXT_INPUT
#define SCI_CMD ( SCI_ALT | SCI_CTRL | SCI_SHIFT)

static const KeyToCommand macMapDefault[] = {
    {SCK_DOWN,		SCI_CMD,	SCI_DOCUMENTEND},
    {SCK_UP,		SCI_CMD,	SCI_DOCUMENTSTART},
    {SCK_LEFT,		SCI_CMD,	SCI_VCHOME},
    {SCK_RIGHT,		SCI_CMD,	SCI_LINEEND},
    {SCK_DOWN,		SCI_NORM,	SCI_LINEDOWN},
    {SCK_DOWN,		SCI_SHIFT,	SCI_LINEDOWNEXTEND},
    {SCK_DOWN,		SCI_CTRL,	SCI_LINESCROLLDOWN},
    {SCK_DOWN,		SCI_ASHIFT,	SCI_LINEDOWNRECTEXTEND},
    {SCK_UP,		SCI_NORM,	SCI_LINEUP},
    {SCK_UP,			SCI_SHIFT,	SCI_LINEUPEXTEND},
    {SCK_UP,			SCI_CTRL,	SCI_LINESCROLLUP},
    {SCK_UP,		SCI_ASHIFT,	SCI_LINEUPRECTEXTEND},
    {'[',			SCI_CTRL,		SCI_PARAUP},
    {'[',			SCI_CSHIFT,	SCI_PARAUPEXTEND},
    {']',			SCI_CTRL,		SCI_PARADOWN},
    {']',			SCI_CSHIFT,	SCI_PARADOWNEXTEND},
    {SCK_LEFT,		SCI_NORM,	SCI_CHARLEFT},
    {SCK_LEFT,		SCI_SHIFT,	SCI_CHARLEFTEXTEND},
    {SCK_LEFT,		SCI_ALT,	SCI_WORDLEFT},
    {SCK_LEFT,		SCI_CSHIFT,	SCI_WORDLEFTEXTEND},
    {SCK_LEFT,		SCI_ASHIFT,	SCI_CHARLEFTRECTEXTEND},
    {SCK_RIGHT,		SCI_NORM,	SCI_CHARRIGHT},
    {SCK_RIGHT,		SCI_SHIFT,	SCI_CHARRIGHTEXTEND},
    {SCK_RIGHT,		SCI_ALT,	SCI_WORDRIGHT},
    {SCK_RIGHT,		SCI_CSHIFT,	SCI_WORDRIGHTEXTEND},
    {SCK_RIGHT,		SCI_ASHIFT,	SCI_CHARRIGHTRECTEXTEND},
    {'/',		SCI_CTRL,		SCI_WORDPARTLEFT},
    {'/',		SCI_CSHIFT,	SCI_WORDPARTLEFTEXTEND},
    {'\\',		SCI_CTRL,		SCI_WORDPARTRIGHT},
    {'\\',		SCI_CSHIFT,	SCI_WORDPARTRIGHTEXTEND},
    {SCK_HOME,		SCI_NORM,	SCI_VCHOME},
    {SCK_HOME, 		SCI_SHIFT, 	SCI_VCHOMEEXTEND},
    {SCK_HOME, 		SCI_CTRL, 	SCI_DOCUMENTSTART},
    {SCK_HOME, 		SCI_CSHIFT, 	SCI_DOCUMENTSTARTEXTEND},
    {SCK_HOME, 		SCI_ALT, 	SCI_HOMEDISPLAY},
//    {SCK_HOME,		SCI_ASHIFT,	SCI_HOMEDISPLAYEXTEND},
    {SCK_HOME,		SCI_ASHIFT,	SCI_VCHOMERECTEXTEND},
    {SCK_END,	 	SCI_NORM,	SCI_LINEEND},
    {SCK_END,	 	SCI_SHIFT, 	SCI_LINEENDEXTEND},
    {SCK_END, 		SCI_CTRL, 	SCI_DOCUMENTEND},
    {SCK_END, 		SCI_CSHIFT, 	SCI_DOCUMENTENDEXTEND},
    {SCK_END, 		SCI_ALT, 	SCI_LINEENDDISPLAY},
//    {SCK_END,		SCI_ASHIFT,	SCI_LINEENDDISPLAYEXTEND},
    {SCK_END,		SCI_ASHIFT,	SCI_LINEENDRECTEXTEND},
    {SCK_PRIOR,		SCI_NORM,	SCI_PAGEUP},
    {SCK_PRIOR,		SCI_SHIFT, 	SCI_PAGEUPEXTEND},
    {SCK_PRIOR,		SCI_ASHIFT,	SCI_PAGEUPRECTEXTEND},
    {SCK_NEXT, 		SCI_NORM, 	SCI_PAGEDOWN},
    {SCK_NEXT, 		SCI_SHIFT, 	SCI_PAGEDOWNEXTEND},
    {SCK_NEXT,		SCI_ASHIFT,	SCI_PAGEDOWNRECTEXTEND},
    {SCK_DELETE, 	SCI_NORM,	SCI_CLEAR},
    {SCK_DELETE, 	SCI_SHIFT,	SCI_CUT},
    {SCK_DELETE, 	SCI_CTRL,	SCI_DELWORDRIGHT},
    {SCK_DELETE,	SCI_CSHIFT,	SCI_DELLINERIGHT},
    {SCK_INSERT, 		SCI_NORM,	SCI_EDITTOGGLEOVERTYPE},
    {SCK_INSERT, 		SCI_SHIFT,	SCI_PASTE},
    {SCK_INSERT, 		SCI_CTRL,	SCI_COPY},
    {SCK_ESCAPE,  	SCI_NORM,	SCI_CANCEL},
    {SCK_BACK,		SCI_NORM, 	SCI_DELETEBACK},
    {SCK_BACK,		SCI_SHIFT, 	SCI_DELETEBACK},
    {SCK_BACK,		SCI_CTRL, 	SCI_DELWORDLEFT},
    {SCK_BACK, 		SCI_ALT,	SCI_UNDO},
    {SCK_BACK,		SCI_CSHIFT,	SCI_DELLINELEFT},
    {'Z', 			SCI_CTRL,	SCI_UNDO},
    {'Y', 			SCI_CTRL,	SCI_REDO},
    {'X', 			SCI_CTRL,	SCI_CUT},
    {'C', 			SCI_CTRL,	SCI_COPY},
    {'V', 			SCI_CTRL,	SCI_PASTE},
    {'A', 			SCI_CTRL,	SCI_SELECTALL},
    {SCK_TAB,		SCI_NORM,	SCI_TAB},
    {SCK_TAB,		SCI_SHIFT,	SCI_BACKTAB},
    {SCK_RETURN, 	SCI_NORM,	SCI_NEWLINE},
    {SCK_RETURN, 	SCI_SHIFT,	SCI_NEWLINE},
    {SCK_ADD, 		SCI_CTRL,	SCI_ZOOMIN},
    {SCK_SUBTRACT,	SCI_CTRL,	SCI_ZOOMOUT},
    {SCK_DIVIDE,	SCI_CTRL,	SCI_SETZOOM},
    //'L', 			SCI_CTRL,		SCI_FORMFEED,
    {'L', 			SCI_CTRL,	SCI_LINECUT},
    {'L', 			SCI_CSHIFT,	SCI_LINEDELETE},
    {'T', 			SCI_CSHIFT,	SCI_LINECOPY},
    {'T', 			SCI_CTRL,	SCI_LINETRANSPOSE},
    {'D', 			SCI_CTRL,	SCI_SELECTIONDUPLICATE},
    {'U', 			SCI_CTRL,	SCI_LOWERCASE},
    {'U', 			SCI_CSHIFT,	SCI_UPPERCASE},
    {0,0,0},
};
#endif

ScintillaMacOSX::ScintillaMacOSX( void* windowid ) :
        TView( reinterpret_cast<HIViewRef>( windowid ) )
{
    notifyObj = NULL;
    notifyProc = NULL;
    wMain = windowid;
    OSStatus err;
    err = GetThemeMetric( kThemeMetricScrollBarWidth, &scrollBarFixedSize );
    assert( err == noErr );

    mouseTrackingRef = NULL;
    mouseTrackingID.signature = scintillaMacOSType;
    mouseTrackingID.id = (SInt32)this;
    capturedMouse = false;

    // Enable keyboard events and mouse events
#if !defined(CONTAINER_HANDLES_EVENTS)
    ActivateInterface( kKeyboardFocus );
    ActivateInterface( kMouse );
    ActivateInterface( kDragAndDrop );
#endif
    ActivateInterface( kMouseTracking );

    Initialise();

    // Create some bounds rectangle which will just get reset to the correct rectangle later
    Rect tempScrollRect;
    tempScrollRect.top = -1;
    tempScrollRect.left = 400;
    tempScrollRect.bottom = 300;
    tempScrollRect.right = 450;

    // Create the scroll bar with fake values that will get set correctly later
    err = CreateScrollBarControl( this->GetOwner(), &tempScrollRect, 0, 0, 100, 100, true, LiveScrollHandler, &vScrollBar );
    assert( vScrollBar != NULL && err == noErr );
    err = CreateScrollBarControl( this->GetOwner(), &tempScrollRect, 0, 0, 100, 100, true, LiveScrollHandler, &hScrollBar );
    assert( hScrollBar != NULL && err == noErr );

    // Set a property on the scrollbars to store a pointer to the Scintilla object
    ScintillaMacOSX* objectPtr = this;
    err = SetControlProperty( vScrollBar, scintillaMacOSType, 0, sizeof( this ), &objectPtr );
    assert( err == noErr );
    err = SetControlProperty( hScrollBar, scintillaMacOSType, 0, sizeof( this ), &objectPtr );
    assert( err == noErr );

    // set this into our parent control so we can be retrieved easily at a later time 
    // (see scintilla_send below)
    err = SetControlProperty( reinterpret_cast<HIViewRef>( windowid ), scintillaMacOSType, 0, sizeof( this ), &objectPtr );
    assert( err == noErr );

    // Tell Scintilla not to buffer: Quartz buffers drawing for us
    // TODO: Can we disable this option on Mac OS X?
    WndProc( SCI_SETBUFFEREDDRAW, 0, 0 );
    // Turn on UniCode mode
    WndProc( SCI_SETCODEPAGE, SC_CP_UTF8, 0 );

    const EventTypeSpec commandEventInfo[] = {
        { kEventClassCommand, kEventProcessCommand },
        { kEventClassCommand, kEventCommandUpdateStatus },
    };

    err = InstallEventHandler( GetControlEventTarget( reinterpret_cast<HIViewRef>( windowid ) ), 
                   CommandEventHandler,
                   GetEventTypeCount( commandEventInfo ), 
                   commandEventInfo,
                   this, NULL);
#ifdef EXT_INPUT
    ExtInput::attach (GetViewRef());
    for (int i = 0; macMapDefault[i].key; i++) 
    {
        this->kmap.AssignCmdKey(macMapDefault[i].key, macMapDefault[i].modifiers, macMapDefault[i].msg);
    }
#endif
}

ScintillaMacOSX::~ScintillaMacOSX() {
    // If the window is closed and the timer is not removed,
    // A segment violation will occur when it attempts to fire the timer next.
    if ( mouseTrackingRef != NULL ) {
        ReleaseMouseTrackingRegion(mouseTrackingRef);
    }
    mouseTrackingRef = NULL;
    SetTicking(false);
#ifdef EXT_INPUT
    ExtInput::detach (GetViewRef());
#endif
}

void ScintillaMacOSX::Initialise() {
    // TODO: Do anything here? Maybe this stuff should be here instead of the constructor?
}

void ScintillaMacOSX::Finalise() {
    SetTicking(false);
    ScintillaBase::Finalise();
}

// --------------------------------------------------------------------------------------------------------------
//
// IsDropInFinderTrash - Returns true if the given dropLocation AEDesc is a descriptor of the Finder's Trash.
//
#pragma segment Drag

Boolean IsDropInFinderTrash(AEDesc *dropLocation)
{
    OSErr      result;
    AEDesc      dropSpec;
    FSSpec      *theSpec;
    CInfoPBRec    thePB;
    short      trashVRefNum;
    long      trashDirID;
  
    //  Coerce the dropLocation descriptor into an FSSpec. If there's no dropLocation or
    //  it can't be coerced into an FSSpec, then it couldn't have been the Trash.

  if ((dropLocation->descriptorType != typeNull) &&
    (AECoerceDesc(dropLocation, typeFSS, &dropSpec) == noErr)) 
    {
        unsigned char flags = HGetState((Handle)dropSpec.dataHandle);
        
        HLock((Handle)dropSpec.dataHandle);
        theSpec = (FSSpec *) *dropSpec.dataHandle;
        
        //  Get the directory ID of the given dropLocation object.
        
        thePB.dirInfo.ioCompletion = 0L;
        thePB.dirInfo.ioNamePtr = (StringPtr) &theSpec->name;
        thePB.dirInfo.ioVRefNum = theSpec->vRefNum;
        thePB.dirInfo.ioFDirIndex = 0;
        thePB.dirInfo.ioDrDirID = theSpec->parID;
        
        result = PBGetCatInfoSync(&thePB);
        
        HSetState((Handle)dropSpec.dataHandle, flags);
        AEDisposeDesc(&dropSpec);
        
        if (result != noErr)
            return false;
        
        //  If the result is not a directory, it must not be the Trash.
        
        if (!(thePB.dirInfo.ioFlAttrib & (1 << 4)))
            return false;
        
        //  Get information about the Trash folder.
        
        FindFolder(theSpec->vRefNum, kTrashFolderType, kCreateFolder, &trashVRefNum, &trashDirID);
        
        //  If the directory ID of the dropLocation object is the same as the directory ID
        //  returned by FindFolder, then the drop must have occurred into the Trash.
        
        if (thePB.dirInfo.ioDrDirID == trashDirID)
            return true;
    }

    return false;

} // IsDropInFinderTrash

HIPoint ScintillaMacOSX::GetLocalPoint(::Point pt)
{
    // get the mouse position so we can offset it
    Rect bounds;
    GetWindowBounds( GetOwner(), kWindowStructureRgn, &bounds );

    PRectangle hbounds = wMain.GetPosition();
    HIViewRef parent = HIViewGetSuperview(GetViewRef());
    Rect pbounds;
    GetControlBounds(parent, &pbounds);
      
    bounds.left += pbounds.left + hbounds.left;
    bounds.top += pbounds.top + hbounds.top;

    HIPoint offset = { pt.h - bounds.left, pt.v - bounds.top };
    return offset;
}

void ScintillaMacOSX::StartDrag() {
    if (sel.Empty()) return;

    // calculate the bounds of the selection
        PRectangle client = GetTextRectangle();
    int selStart = sel.RangeMain().Start().Position();
    int selEnd = sel.RangeMain().End().Position();
    int startLine = pdoc->LineFromPosition(selStart);
    int endLine = pdoc->LineFromPosition(selEnd);
    Point pt;
    int startPos, endPos, ep;
    Rect rcSel;
    rcSel.top = rcSel.bottom = rcSel.right = rcSel.left = -1;
    for (int l = startLine; l <= endLine; l++) {
        startPos = WndProc(SCI_GETLINESELSTARTPOSITION, l, 0);
        endPos = WndProc(SCI_GETLINESELENDPOSITION, l, 0);
        if (endPos == startPos) continue;
        // step back a position if we're counting the newline
        ep = WndProc(SCI_GETLINEENDPOSITION, l, 0);
        if (endPos > ep) endPos = ep;
  
        pt = LocationFromPosition(startPos); // top left of line selection
        if (pt.x < rcSel.left || rcSel.left < 0) rcSel.left = pt.x;
        if (pt.y < rcSel.top || rcSel.top < 0) rcSel.top = pt.y;
  
        pt = LocationFromPosition(endPos); // top right of line selection
        pt.y += vs.lineHeight; // get to the bottom of the line
        if (pt.x > rcSel.right || rcSel.right < 0) {
            if (pt.x > client.right)
                rcSel.right = client.right;
            else
                rcSel.right = pt.x;
        }
        if (pt.y > rcSel.bottom || rcSel.bottom < 0) {
            if (pt.y > client.bottom)
                rcSel.bottom = client.bottom;
            else
                rcSel.bottom = pt.y;
        }
    }

    // must convert to global coordinates for drag regions, but also save the
    // image rectangle for further calculations and copy operations
    PRectangle imageRect = PRectangle(rcSel.left, rcSel.top, rcSel.right, rcSel.bottom);
    QDLocalToGlobalRect(GetWindowPort(GetOwner()), &rcSel);

    // get the mouse position so we can offset it
    HIPoint offset = GetLocalPoint(mouseDownEvent.where);
    offset.y = (imageRect.top * 1.0) - offset.y;
    offset.x = (imageRect.left * 1.0) - offset.x;

    // to get a bitmap of the text we're dragging, we just use Paint on a 
    // pixmap surface.
    SurfaceImpl *sw = new SurfaceImpl();
    SurfaceImpl *pixmap = NULL;

    if (sw) {
        pixmap = new SurfaceImpl();
        if (pixmap) {
            client = GetClientRectangle();
            paintState = painting;
            sw->InitPixMap( client.Width(), client.Height(), NULL, NULL );
            paintingAllText = true;
            Paint(sw, imageRect);
            paintState = notPainting;
    
            pixmap->InitPixMap( imageRect.Width(), imageRect.Height(), NULL, NULL );
    
            CGContextRef gc = pixmap->GetContext(); 
    
            // to make Paint() work on a bitmap, we have to flip our coordinates
            // and translate the origin
            //fprintf(stderr, "translate to %d\n", client.Height() );
            CGContextTranslateCTM(gc, 0, imageRect.Height());
            CGContextScaleCTM(gc, 1.0, -1.0);
    
            pixmap->CopyImageRectangle( *sw, imageRect, PRectangle( 0, 0, imageRect.Width(), imageRect.Height() ));
            // XXX TODO: overwrite any part of the image that is not part of the
            //           selection to make it transparent.  right now we just use
            //           the full rectangle which may include non-selected text.
        }
        sw->Release();
        delete sw;
    }

    // now we initiate the drag session

    RgnHandle dragRegion = NewRgn();
    RgnHandle tempRegion;
    DragRef inDrag;
    DragAttributes attributes;
    AEDesc dropLocation;
    SInt16 mouseDownModifiers, mouseUpModifiers;
    bool copyText;
    CGImageRef image = NULL;

    RectRgn(dragRegion, &rcSel);

    SelectionText selectedText;
    CopySelectionRange(&selectedText);
    PasteboardRef theClipboard;
    SetPasteboardData(theClipboard, selectedText);
    NewDragWithPasteboard( theClipboard, &inDrag);
    CFRelease( theClipboard );

    //  Set the item's bounding rectangle in global coordinates.
    SetDragItemBounds(inDrag, 1, &rcSel);

    //  Prepare the drag region.
    tempRegion = NewRgn();
    CopyRgn(dragRegion, tempRegion);
    InsetRgn(tempRegion, 1, 1);
    DiffRgn(dragRegion, tempRegion, dragRegion);
    DisposeRgn(tempRegion);

    // if we have a pixmap, lets use that
    if (pixmap) {
        image = pixmap->GetImage();
        SetDragImageWithCGImage (inDrag, image, &offset, kDragStandardTranslucency);
    }

    //  Drag the text. TrackDrag will return userCanceledErr if the drop whooshed back for any reason.
    inDragDrop = ddDragging;
    OSErr error = TrackDrag(inDrag, &mouseDownEvent, dragRegion);
    inDragDrop = ddNone;

    //  Check to see if the drop occurred in the Finder's Trash. If the drop occurred
    //  in the Finder's Trash and a copy operation wasn't specified, delete the
    //  source selection. Note that we can continute to get the attributes, drop location
    //  modifiers, etc. of the drag until we dispose of it using DisposeDrag.
    if (error == noErr) {
        GetDragAttributes(inDrag, &attributes);
        if (!(attributes & kDragInsideSenderApplication))
        {
            GetDropLocation(inDrag, &dropLocation);
    
            GetDragModifiers(inDrag, 0L, &mouseDownModifiers, &mouseUpModifiers);
            copyText = (mouseDownModifiers | mouseUpModifiers) & optionKey;
    
            if ((!copyText) && (IsDropInFinderTrash(&dropLocation)))
            {
                // delete the selected text from the buffer
                ClearSelection();
            }
    
            AEDisposeDesc(&dropLocation);
        }
    }

    // Dispose of this drag, 'cause we're done.
    DisposeDrag(inDrag);
    DisposeRgn(dragRegion);

    if (pixmap) {
        CGImageRelease(image);
        pixmap->Release();
        delete pixmap;
    }
}

void ScintillaMacOSX::SetDragCursor(DragRef inDrag)
{
    DragAttributes attributes;
    SInt16 modifiers = 0; 
    ThemeCursor cursor = kThemeCopyArrowCursor;
    GetDragAttributes( inDrag, &attributes );

    if ( attributes & kDragInsideSenderWindow ) {
        GetDragModifiers(inDrag, &modifiers, NULL, NULL);
        switch (modifiers & ~btnState)  // Filter out btnState (on for drop)
        {
        case optionKey:
            // it's a copy, leave it as a copy arrow
            break;
      
        case cmdKey:
        case cmdKey | optionKey:
        default:
            // what to do with these?  rectangular drag?
            cursor = kThemeArrowCursor;
            break;
        }
    }
    SetThemeCursor(cursor);
}

bool ScintillaMacOSX::DragEnter(DragRef inDrag )
{
    if (!DragWithin(inDrag))
        return false;

    DragAttributes attributes;
    GetDragAttributes( inDrag, &attributes );
    
    // only show the drag hilight if the drag has left the sender window per HI spec
    if( attributes & kDragHasLeftSenderWindow )
    {
        HIRect    textFrame;
        RgnHandle  hiliteRgn = NewRgn();
        
        // get the text view's frame ...
        HIViewGetFrame( GetViewRef(), &textFrame );
        
        // ... and convert it into a region for ShowDragHilite
        HIShapeRef textShape = HIShapeCreateWithRect( &textFrame );
        HIShapeGetAsQDRgn( textShape, hiliteRgn );
        CFRelease( textShape );
        
        // add the drag hilight to the inside of the text view
        ShowDragHilite( inDrag, hiliteRgn, true );
        
        DisposeRgn( hiliteRgn );
    }
    SetDragCursor(inDrag);
    return true;
}

Scintilla::Point ScintillaMacOSX::GetDragPoint(DragRef inDrag)
{
    ::Point mouse, globalMouse;
    GetDragMouse(inDrag, &mouse, &globalMouse);
    HIPoint hiPoint = GetLocalPoint (globalMouse);
    return Point(static_cast<int>(hiPoint.x), static_cast<int>(hiPoint.y));
}


void ScintillaMacOSX::DragScroll()
{
#define RESET_SCROLL_TIMER(lines) \
  scrollSpeed = (lines); \
  scrollTicks = 2000;

    if (!posDrag.IsValid()) {
        RESET_SCROLL_TIMER(1);
        return;
    }
    Point dragMouse = LocationFromPosition(posDrag);
    int line = pdoc->LineFromPosition(posDrag.Position());
    int currentVisibleLine = cs.DisplayFromDoc(line);
    int lastVisibleLine = Platform::Minimum(topLine + LinesOnScreen() - 1, pdoc->LinesTotal() - 1);

    if (currentVisibleLine <= topLine && topLine > 0) {
        ScrollTo( topLine - scrollSpeed );
    } else if (currentVisibleLine >= lastVisibleLine) {
        ScrollTo( topLine + scrollSpeed );
    } else {
        RESET_SCROLL_TIMER(1);
        return;
    }
    if (scrollSpeed == 1) {
        scrollTicks -= timer.tickSize;
        if (scrollTicks <= 0) {
            RESET_SCROLL_TIMER(5);
        }
    }

    SetDragPosition(SPositionFromLocation(dragMouse));

#undef RESET_SCROLL_TIMER
}

bool ScintillaMacOSX::DragWithin(DragRef inDrag )
{
    PasteboardRef pasteBoard;
    bool isFileURL = false;
    if (!GetDragData(inDrag, pasteBoard, NULL, &isFileURL)) {
        return false;
    }

    Point pt = GetDragPoint (inDrag);
    SetDragPosition(SPositionFromLocation(pt));
    SetDragCursor(inDrag);

    return true;
}

bool ScintillaMacOSX::DragLeave(DragRef inDrag )
{
    HideDragHilite( inDrag );
    SetDragPosition(SelectionPosition(invalidPosition));
    WndProc(SCI_SETCURSOR, Window::cursorArrow, 0);
    return true;
}

enum
{
    kFormatBad,
    kFormatText,
    kFormatUnicode,
    kFormatUTF8,
    kFormatFile
};

bool ScintillaMacOSX::GetDragData(DragRef inDrag, PasteboardRef &pasteBoard,
                                  SelectionText *selectedText, bool *isFileURL)
{
  // TODO: add support for special flavors: flavorTypeHFS and flavorTypePromiseHFS so we
  //       can handle files being dropped on the editor
    OSStatus status;
    status = GetDragPasteboard(inDrag, &pasteBoard);
    if (status != noErr) {
        return false;
    }
    return GetPasteboardData(pasteBoard, selectedText, isFileURL);
}

void ScintillaMacOSX::SetPasteboardData(PasteboardRef &theClipboard, const SelectionText &selectedText)
{
    if (selectedText.len == 0)
        return;

    CFStringEncoding encoding = ( IsUnicodeMode() ? kCFStringEncodingUTF8 : kCFStringEncodingMacRoman);

    // Create a CFString from the ASCII/UTF8 data, convert it to UTF16
    CFStringRef string = CFStringCreateWithBytes( NULL, reinterpret_cast<UInt8*>( selectedText.s ), selectedText.len - 1, encoding, false );

    PasteboardCreate( kPasteboardClipboard, &theClipboard );
    PasteboardClear( theClipboard );

    CFDataRef data = NULL;
    if (selectedText.rectangular) {
        // This is specific to scintilla, allows us to drag rectangular selections
        // around the document
        data = CFStringCreateExternalRepresentation ( kCFAllocatorDefault, string, kCFStringEncodingUnicode, 0 );
        if (data) {
            PasteboardPutItemFlavor( theClipboard, (PasteboardItemID)1, 
                              CFSTR("com.scintilla.utf16-plain-text.rectangular"),
                              data, 0 );
            CFRelease(data);
        }
    }
    data = CFStringCreateExternalRepresentation ( kCFAllocatorDefault, string, kCFStringEncodingUnicode, 0 );
    if (data) {
        PasteboardPutItemFlavor( theClipboard, (PasteboardItemID)1, 
                                CFSTR("public.utf16-plain-text"),
                                data, 0 );
        CFRelease(data);
        data = NULL;
    }
    data = CFStringCreateExternalRepresentation ( kCFAllocatorDefault, string, kCFStringEncodingMacRoman, 0 );
    if (data) {
        PasteboardPutItemFlavor( theClipboard, (PasteboardItemID)1, 
                                CFSTR("com.apple.traditional-mac-plain-text"),
                                data, 0 );
        CFRelease(data);
        data = NULL;
    }
    CFRelease(string);
}

bool ScintillaMacOSX::GetPasteboardData(PasteboardRef &pasteBoard,
                                        SelectionText *selectedText,
                                        bool *isFileURL)
{
    // how many items in the pasteboard?
    CFDataRef data;
    CFStringRef textString = NULL;
    bool isRectangular = selectedText ? selectedText->rectangular : false;
    ItemCount i, itemCount;
    OSStatus status = PasteboardGetItemCount(pasteBoard, &itemCount);
    if (status != noErr) {
        return false;
    }

    // as long as we didn't get our text, let's loop on the items. We stop as soon as we get it
    CFArrayRef flavorTypeArray = NULL;
    bool haveMatch = false;
    for (i = 1; i <= itemCount; i++)
    {
        PasteboardItemID itemID;
        CFIndex j, flavorCount = 0;

        status = PasteboardGetItemIdentifier(pasteBoard, i, &itemID);
        if (status != noErr) {
            return false;
        }

        // how many flavors in this item?
        status = PasteboardCopyItemFlavors(pasteBoard, itemID, &flavorTypeArray);
        if (status != noErr) {
            return false;
        }

        if (flavorTypeArray != NULL)
            flavorCount = CFArrayGetCount(flavorTypeArray);

        // as long as we didn't get our text, let's loop on the flavors. We stop as soon as we get it
        for(j = 0; j < flavorCount; j++)
        {
            CFDataRef flavorData;
            CFStringRef flavorType = (CFStringRef)CFArrayGetValueAtIndex(flavorTypeArray, j);
            if (flavorType != NULL) 
            {
                int format = kFormatBad;
                if (UTTypeConformsTo(flavorType, CFSTR("public.file-url"))) {
                    format = kFormatFile;
                    *isFileURL = true;
                }
                else if (UTTypeConformsTo(flavorType, CFSTR("com.scintilla.utf16-plain-text.rectangular"))) {
                    format = kFormatUnicode;
                    isRectangular = true;
                }
                else if (UTTypeConformsTo(flavorType, CFSTR("public.utf16-plain-text"))) { // this is 'utxt'
                    format = kFormatUnicode;
                }
                else if (UTTypeConformsTo(flavorType, CFSTR("public.utf8-plain-text"))) {
                    format = kFormatUTF8;
                }
                else if (UTTypeConformsTo(flavorType, CFSTR("com.apple.traditional-mac-plain-text"))) { // this is 'TEXT'
                    format = kFormatText;
                }
                if (format == kFormatBad)
                    continue;
                
                // if we got a flavor match, and we have no textString, we just want
                // to know that we can accept this data, so jump out now
                if (selectedText == NULL) {
                    haveMatch = true;
                    goto PasteboardDataRetrieved;
                }
                if (PasteboardCopyItemFlavorData(pasteBoard, itemID, flavorType, &flavorData) == noErr)
                {
                    CFIndex dataSize = CFDataGetLength (flavorData);
                    const UInt8* dataBytes = CFDataGetBytePtr (flavorData);
                    switch (format)
                    {
                        case kFormatFile:
                        case kFormatText:
                            data = CFDataCreate (NULL, dataBytes, dataSize);
                            textString = CFStringCreateFromExternalRepresentation (NULL, data, kCFStringEncodingMacRoman);
                            break;
                        case kFormatUnicode:
                            data = CFDataCreate (NULL, dataBytes, dataSize);
                            textString = CFStringCreateFromExternalRepresentation (NULL, data, kCFStringEncodingUnicode);
                            break;
                        case kFormatUTF8:
                            data = CFDataCreate (NULL, dataBytes, dataSize);
                            textString = CFStringCreateFromExternalRepresentation (NULL, data, kCFStringEncodingUTF8);
                            break;
                    }
                    CFRelease (flavorData);
                    goto PasteboardDataRetrieved;
                }
            }
        }
    }
PasteboardDataRetrieved:
    if (flavorTypeArray != NULL) CFRelease(flavorTypeArray);
        int newlen = 0;
    if (textString != NULL) {
        selectedText->s = GetStringFromCFString(textString, &selectedText->len);
        selectedText->rectangular = isRectangular;
        // Default allocator releases both the CFString and the UniChar buffer (text)
        CFRelease( textString );
        textString = NULL;
    }
    if (haveMatch || selectedText != NULL && selectedText->s != NULL) {
        return true;
    }
    return false;
}

char *ScintillaMacOSX::GetStringFromCFString(CFStringRef &textString, int *textLen)
{

    // Allocate a buffer, plus the null byte
    CFIndex numUniChars = CFStringGetLength( textString );
    CFStringEncoding encoding = ( IsUnicodeMode() ? kCFStringEncodingUTF8 : kCFStringEncodingMacRoman);
    CFIndex maximumByteLength = CFStringGetMaximumSizeForEncoding( numUniChars, encoding ) + 1;
    char* cstring = new char[maximumByteLength];
    CFIndex usedBufferLength = 0;
    CFIndex numCharsConverted;
    numCharsConverted = CFStringGetBytes( textString, CFRangeMake( 0, numUniChars ), encoding,
                              '?', false, reinterpret_cast<UInt8*>( cstring ),
                              maximumByteLength, &usedBufferLength );
    cstring[usedBufferLength] = '\0'; // null terminate the ASCII/UTF8 string

    // determine whether a BOM is in the string.  Apps like Emacs prepends a BOM
    // to the string, CFStrinGetBytes reflects that (though it may change in the conversion)
    // so we need to remove it before pasting into our buffer.  TextWrangler has no
    // problem dealing with BOM when pasting into it.
    int bomLen = BOMlen((unsigned char *)cstring);

    // convert line endings to the document line ending
    *textLen = 0;
    char *result = Document::TransformLineEnds(textLen,
                                      cstring + bomLen,
                                      usedBufferLength - bomLen,
                                      pdoc->eolMode);
    delete[] cstring;
    return result;
}

OSStatus ScintillaMacOSX::DragReceive(DragRef inDrag )
{
    // dragleave IS called, but for some reason (probably to do with inDrag)
    // the hide hilite does not happen unless we do it here
    HideDragHilite( inDrag );

    PasteboardRef pasteBoard;
    SelectionText selectedText;
    CFStringRef textString = NULL;
    bool isFileURL = false;
    if (!GetDragData(inDrag, pasteBoard, &selectedText, &isFileURL)) {
        return dragNotAcceptedErr;
    }

    if (isFileURL) {
        NotifyURIDropped(selectedText.s);
    } else {
        // figure out if this is a move or a paste
        DragAttributes attributes;
        SInt16 modifiers = 0; 
        GetDragAttributes( inDrag, &attributes );
        bool moving = true;
    
        SelectionPosition position = SPositionFromLocation(GetDragPoint(inDrag));
        if ( attributes & kDragInsideSenderWindow ) {
            GetDragModifiers(inDrag, NULL, NULL, &modifiers);
            switch (modifiers & ~btnState)  // Filter out btnState (on for drop)
            {
            case optionKey:
                // default is copy text
                moving = false;
                break;
            case cmdKey:
            case cmdKey | optionKey:
            default:
                // what to do with these?  rectangular drag?
                break;
            }
        }

        DropAt(position, selectedText.s, moving, selectedText.rectangular);
    }

    return noErr;
}

// Extended UTF8-UTF6-conversion to handle surrogate pairs correctly (CL265070)
void ScintillaMacOSX::InsertCharacters (const UniChar* buf, int len)
{
    CFStringRef str = CFStringCreateWithCharactersNoCopy (NULL, buf, (UInt32) len, kCFAllocatorNull);
    CFStringEncoding encoding = ( IsUnicodeMode() ? kCFStringEncodingUTF8 : kCFStringEncodingMacRoman);
    CFRange range = { 0, len };
    CFIndex bufLen;
    CFStringGetBytes (str, range, encoding, '?', false, NULL, 0, &bufLen);
    UInt8* utf8buf = new UInt8 [bufLen];
    CFStringGetBytes (str, range, encoding, '?', false, utf8buf, bufLen, NULL);
    AddCharUTF ((char*) utf8buf, bufLen, false);
    delete [] utf8buf;
    CFRelease (str);
}

/** The simulated message loop. */
sptr_t ScintillaMacOSX::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
    switch (iMessage) {
    case SCI_GETDIRECTFUNCTION:
        Platform::DebugDisplay( "ScintillaMacOSX::WndProc: Returning DirectFunction address.\n" );
        return reinterpret_cast<sptr_t>( DirectFunction );
    
    case SCI_GETDIRECTPOINTER:
        Platform::DebugDisplay( "ScintillaMacOSX::WndProc: Returning Direct pointer address.\n" );
        return reinterpret_cast<sptr_t>( this );
    
    case SCI_GRABFOCUS:
        Platform::DebugDisplay( "ScintillaMacOSX::WndProc: Got an unhandled message. Ignoring it.\n" );
         break;
    case WM_UNICHAR:
        if (IsUnicodeMode()) {
            // Extended UTF8-UTF6-conversion to handle surrogate pairs correctly (CL265070)
            UniChar wcs[1] = { (UniChar) wParam};
            InsertCharacters(wcs, 1);
            return 1;
        } else {
            return 0;
        }

    default:
        unsigned int r = ScintillaBase::WndProc(iMessage, wParam, lParam);

        return r;
    }
    return 0l;
}

sptr_t ScintillaMacOSX::DefWndProc(unsigned int, uptr_t, sptr_t) {
    return 0;
}

void ScintillaMacOSX::SetTicking(bool on) {
    if (timer.ticking != on) {
        timer.ticking = on;
        if (timer.ticking) {
            // Scintilla ticks = milliseconds
            EventLoopTimerRef timerRef = NULL;
            InstallTimer( timer.tickSize * kEventDurationMillisecond, &timerRef );
            assert( timerRef != NULL );
            timer.tickerID = reinterpret_cast<TickerID>( timerRef );
        } else if ( timer.tickerID != NULL ) {
            RemoveEventLoopTimer( reinterpret_cast<EventLoopTimerRef>( timer.tickerID ) );
        }
    }
    timer.ticksToWait = caret.period;
}

bool ScintillaMacOSX::SetIdle(bool on) {
    if (on) {
        // Start idler, if it's not running.
        if (idler.state == false) {
            idler.state = true;
            EventLoopTimerRef idlTimer;
            InstallEventLoopIdleTimer(GetCurrentEventLoop(),
                                      timer.tickSize * kEventDurationMillisecond,
                                      75 * kEventDurationMillisecond,
                                      IdleTimerEventHandler, this, &idlTimer);
            idler.idlerID = reinterpret_cast<IdlerID>( idlTimer );
        }
    } else {
        // Stop idler, if it's running
        if (idler.state == true) {
            idler.state = false;
            if (idler.idlerID != NULL)
                RemoveEventLoopTimer( reinterpret_cast<EventLoopTimerRef>( idler.idlerID ) );
        }
    }
    return true;
}

pascal void ScintillaMacOSX::IdleTimerEventHandler( EventLoopTimerRef inTimer, 
                                                    EventLoopIdleTimerMessage inState,
                                                    void *scintilla )
{
    ScintillaMacOSX *sciThis = reinterpret_cast<ScintillaMacOSX*>( scintilla );
    bool ret = sciThis->Idle();
    if (ret == false) {
        sciThis->SetIdle(false);
    }
}

void ScintillaMacOSX::SetMouseCapture(bool on) {
    capturedMouse = on;
    if (mouseDownCaptures) {
        if (capturedMouse) {
            WndProc(SCI_SETCURSOR, Window::cursorArrow, 0);
        } else {
            // reset to normal, buttonmove will change for other area's in the editor
            WndProc(SCI_SETCURSOR, (long int)SC_CURSORNORMAL, 0);
        }
    }
}

bool ScintillaMacOSX::HaveMouseCapture() {
    return capturedMouse;
}

// The default GetClientRectangle calls GetClientPosition on wMain.
// We override it to return "view local" co-ordinates so we can draw properly
// plus we need to remove the space occupied by the scroll bars
PRectangle ScintillaMacOSX::GetClientRectangle() {
    PRectangle rc = wMain.GetClientPosition();
    if (verticalScrollBarVisible)
        rc.right -= scrollBarFixedSize + 1;
    if (horizontalScrollBarVisible && (wrapState == eWrapNone))
        rc.bottom -= scrollBarFixedSize + 1;
    // Move to origin
    rc.right -= rc.left;
    rc.bottom -= rc.top;
    rc.left = 0;
    rc.top = 0;
    return rc;
}

// Synchronously paint a rectangle of the window.
void ScintillaMacOSX::SyncPaint(void* gc, PRectangle rc) {
    paintState = painting;
    rcPaint = rc;
    PRectangle rcText = GetTextRectangle();
    paintingAllText = rcPaint.Contains(rcText);
    //Platform::DebugPrintf("ScintillaMacOSX::SyncPaint %0d,%0d %0d,%0d\n",
    //  rcPaint.left, rcPaint.top, rcPaint.right, rcPaint.bottom);
    Surface *sw = Surface::Allocate();
    if (sw) {
        sw->Init( gc, wMain.GetID() );
        Paint(sw, rc);
        if (paintState == paintAbandoned) {
          // do a FULL paint.
          rcPaint = GetClientRectangle();
          paintState = painting;
          paintingAllText = true;
          Paint(sw, rcPaint);
          wMain.InvalidateAll();
        }
        sw->Release();
        delete sw;
    }
    paintState = notPainting;
}

void ScintillaMacOSX::ScrollText(int /*linesToMove*/) {
    // This function will invalidate the correct regions of the view,
    // So shortly after this happens, draw will be called.
    // But I'm not quite sure how this works ...
    // I have a feeling that it is only supposed to work in conjunction with an HIScrollView.
    // TODO: Cook up my own bitblt scroll: Grab the bits on screen, blit them shifted, invalidate the remaining stuff
    //CGRect r = CGRectMake( 0, 0, rc.Width(), rc.Height() );
    //HIViewScrollRect( reinterpret_cast<HIViewRef>( wMain.GetID() ), NULL, 0, vs.lineHeight * linesToMove );
    wMain.InvalidateAll();
}

void ScintillaMacOSX::SetVerticalScrollPos() {
    SetControl32BitValue( vScrollBar, topLine );
}

void ScintillaMacOSX::SetHorizontalScrollPos() {
    SetControl32BitValue( hScrollBar, xOffset );
}

bool ScintillaMacOSX::ModifyScrollBars(int nMax, int nPage) {
    Platform::DebugPrintf( "nMax: %d nPage: %d hScroll (%d -> %d) page: %d\n", nMax, nPage, 0, scrollWidth, GetTextRectangle().Width() );
    // Minimum value = 0
    // TODO: This is probably not needed, since we set this when the scroll bars are created
    SetControl32BitMinimum( vScrollBar, 0 );
    SetControl32BitMinimum( hScrollBar, 0 );

    // Maximum vertical value = nMax + 1 - nPage (lines available to scroll)
    SetControl32BitMaximum( vScrollBar, Platform::Maximum( nMax + 1 - nPage, 0 ) );
    // Maximum horizontal value = scrollWidth - GetTextRectangle().Width() (pixels available to scroll)
    SetControl32BitMaximum( hScrollBar, Platform::Maximum( scrollWidth - GetTextRectangle().Width(), 0 ) );

    // Vertical page size = nPage
    SetControlViewSize( vScrollBar, nPage );
    // Horizontal page size = TextRectangle().Width()
    SetControlViewSize( hScrollBar, GetTextRectangle().Width() );

    // TODO: Verify what this return value is for
    // The scroll bar components will handle if they need to be rerendered or not
    return false;
}

void ScintillaMacOSX::ReconfigureScrollBars() {
    PRectangle rc = wMain.GetClientPosition();
    Resize(rc.Width(), rc.Height());
}

void ScintillaMacOSX::Resize(int width, int height) {
    // Get the horizontal/vertical size of the scroll bars
    GetThemeMetric( kThemeMetricScrollBarWidth, &scrollBarFixedSize );

    bool showSBHorizontal = horizontalScrollBarVisible && (wrapState == eWrapNone);
    HIRect scrollRect;
    if (verticalScrollBarVisible) {
        scrollRect.origin.x = width - scrollBarFixedSize;
        scrollRect.origin.y = 0;
        scrollRect.size.width = scrollBarFixedSize;
        if (showSBHorizontal) {
            scrollRect.size.height = Platform::Maximum(1, height - scrollBarFixedSize);
        } else {
            scrollRect.size.height = height;
        }

        HIViewSetFrame( vScrollBar, &scrollRect );
        if (HIViewGetSuperview(vScrollBar) == NULL) {
            HIViewSetDrawingEnabled( vScrollBar, true );
            HIViewSetVisible(vScrollBar, true);
            HIViewAddSubview(GetViewRef(), vScrollBar );
            Draw1Control(vScrollBar);
        }
    } else if (HIViewGetSuperview(vScrollBar) != NULL) {
        HIViewSetDrawingEnabled( vScrollBar, false );
        HIViewRemoveFromSuperview(vScrollBar);
    }

    if (showSBHorizontal) {
        scrollRect.origin.x = 0;
        // Always draw the scrollbar to avoid the "potiential" horizontal scroll bar and to avoid the resize box.
        // This should be "good enough". Best would be to avoid the resize box.
        // Even better would be to embed Scintilla inside an HIScrollView, which would handle this for us.
        scrollRect.origin.y = height - scrollBarFixedSize;
        if (verticalScrollBarVisible) {
            scrollRect.size.width = Platform::Maximum( 1, width - scrollBarFixedSize );
        } else {
            scrollRect.size.width = width;
        }
        scrollRect.size.height = scrollBarFixedSize;

        HIViewSetFrame( hScrollBar, &scrollRect );
        if (HIViewGetSuperview(hScrollBar) == NULL) {
            HIViewSetDrawingEnabled( hScrollBar, true );
            HIViewAddSubview( GetViewRef(), hScrollBar );
            Draw1Control(hScrollBar);
        }
    } else  if (HIViewGetSuperview(hScrollBar) != NULL) {
        HIViewSetDrawingEnabled( hScrollBar, false );
        HIViewRemoveFromSuperview(hScrollBar);
    }

    ChangeSize();

    // fixup mouse tracking regions, this causes mouseenter/exit to work
    if (HIViewGetSuperview(GetViewRef()) != NULL) {
        RgnHandle rgn = NewRgn();
        HIRect r;
        HIViewGetFrame( reinterpret_cast<HIViewRef>( GetViewRef() ), &r );
        SetRectRgn(rgn, short (r.origin.x), short (r.origin.y), 
                  short (r.origin.x + r.size.width - (verticalScrollBarVisible ? scrollBarFixedSize : 0)), 
                  short (r.origin.y + r.size.height - (showSBHorizontal ? scrollBarFixedSize : 0)));
        if (mouseTrackingRef == NULL) {
            CreateMouseTrackingRegion(GetOwner(), rgn, NULL, 
                                        kMouseTrackingOptionsLocalClip,
                                        mouseTrackingID, NULL, 
                                        GetControlEventTarget( GetViewRef() ), 
                                        &mouseTrackingRef);
        } else {
            ChangeMouseTrackingRegion(mouseTrackingRef, rgn, NULL);
        }
        DisposeRgn(rgn);
    } else {
        if (mouseTrackingRef != NULL) {
            ReleaseMouseTrackingRegion(mouseTrackingRef);
        }
        mouseTrackingRef = NULL;
    }
}

pascal void ScintillaMacOSX::LiveScrollHandler( HIViewRef control, SInt16 part )
{
    int currentValue = GetControl32BitValue( control );
    int min = GetControl32BitMinimum( control );
    int max = GetControl32BitMaximum( control );
    int page = GetControlViewSize( control );

    // Get a reference to the Scintilla C++ object
    ScintillaMacOSX* scintilla = NULL;
    OSStatus err;
    err = GetControlProperty( control, scintillaMacOSType, 0, sizeof( scintilla ), NULL, &scintilla );
    assert( err == noErr && scintilla != NULL );

    int singleScroll = 0;
    if ( control == scintilla->vScrollBar )
    {
        // Vertical single scroll = one line
        // TODO: Is there a Scintilla preference for this somewhere?
        singleScroll = 1;
    } else {
        assert( control == scintilla->hScrollBar );
        // Horizontal single scroll = 20 pixels (hardcoded from ScintillaWin)
        // TODO: Is there a Scintilla preference for this somewhere?
        singleScroll = 20;
    }

    // Determine the new value
    int newValue = 0;
    switch ( part )
    {
    case kControlUpButtonPart:
        newValue = Platform::Maximum( currentValue - singleScroll, min );
        break;

    case kControlDownButtonPart:
        // the the user scrolls to the right, allow more scroll space
        if ( control == scintilla->hScrollBar && currentValue >= max) {
          // change the max value
          scintilla->scrollWidth += singleScroll;
          SetControl32BitMaximum( control,
                           Platform::Maximum( scintilla->scrollWidth - scintilla->GetTextRectangle().Width(), 0 ) );
          max = GetControl32BitMaximum( control );
          scintilla->SetScrollBars();
        }
        newValue =  Platform::Minimum( currentValue + singleScroll, max );
        break;

    case kControlPageUpPart:
        newValue = Platform::Maximum( currentValue - page, min );
        break;

    case kControlPageDownPart:
        newValue = Platform::Minimum( currentValue + page, max );
        break;

    case kControlIndicatorPart:
    case kControlNoPart:
        newValue = currentValue;
        break;

    default:
        assert( false );
        return;
    }

    // Set the new value
    if ( control == scintilla->vScrollBar )
    {
        scintilla->ScrollTo( newValue );
    } else {
        assert( control == scintilla->hScrollBar );
        scintilla->HorizontalScrollTo( newValue );
    }
}

bool ScintillaMacOSX::ScrollBarHit(HIPoint location) {
	// is this on our scrollbars?  If so, track them
	HIViewRef view;
	// view is null if on editor, otherwise on scrollbar
	HIViewGetSubviewHit(reinterpret_cast<ControlRef>(wMain.GetID()),
                            &location, true, &view);
	if (view) {
		HIViewPartCode part;

		// make the point local to a scrollbar 
                PRectangle client = GetClientRectangle();
		if (view == vScrollBar) {
			location.x -= client.Width();
		} else if (view == hScrollBar) {
			location.y -= client.Height();
		} else {
			fprintf(stderr, "got a subview hit, but not a scrollbar???\n");
                        return false;
		}
                
		HIViewGetPartHit(view, &location, &part);
	
		switch (part)
		{
			case kControlUpButtonPart:
			case kControlDownButtonPart:
			case kControlPageUpPart:
			case kControlPageDownPart:
			case kControlIndicatorPart:
                                ::Point p;
                                p.h = location.x;
                                p.v = location.y;
				// We are assuming Appearance 1.1 or later, so we
				// have the "live scroll" variant of the scrollbar,
				// which lets you pass the action proc to TrackControl
				// for the thumb (this was illegal in previous
				// versions of the defproc).
				isTracking = true;
				::TrackControl(view, p, ScintillaMacOSX::LiveScrollHandler);
				::HiliteControl(view, 0);
				isTracking = false;
				// The mouseup was eaten by TrackControl, however if we
				// do not get a mouseup in the scintilla xbl widget,
				// many bad focus issues happen.  Simply post a mouseup
				// and this firey pit becomes a bit cooler.
				PostEvent(mouseUp, 0);
				break;
			default:
				fprintf(stderr, "PlatformScrollBarHit part %d\n", part);
		}
		return true;
	}
	return false;
}

void ScintillaMacOSX::NotifyFocus(bool focus) {
#ifdef EXT_INPUT
    ExtInput::activate (GetViewRef(), focus);
#endif
    if (NULL != notifyProc)
        notifyProc (notifyObj, WM_COMMAND, 
                (uintptr_t) ((focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS) << 16),
                (uintptr_t) GetViewRef());
}

void ScintillaMacOSX::NotifyChange() {
    if (NULL != notifyProc)
        notifyProc (notifyObj, WM_COMMAND, 
                (uintptr_t) (SCEN_CHANGE << 16),
                (uintptr_t) GetViewRef());
}

void ScintillaMacOSX::registerNotifyCallback(intptr_t windowid, SciNotifyFunc callback) {
    notifyObj = windowid;
    notifyProc = callback;
}

void ScintillaMacOSX::NotifyParent(SCNotification scn) { 
    if (NULL != notifyProc) {
        scn.nmhdr.hwndFrom = (void*) this;
        scn.nmhdr.idFrom = (unsigned int)wMain.GetID();
        notifyProc (notifyObj, WM_NOTIFY, (uintptr_t) 0, (uintptr_t) &scn);
    }
}

void ScintillaMacOSX::NotifyKey(int key, int modifiers) {
    SCNotification scn;
    scn.nmhdr.code = SCN_KEY;
    scn.ch = key;
    scn.modifiers = modifiers;

    NotifyParent(scn);
}

void ScintillaMacOSX::NotifyURIDropped(const char *list) {
    SCNotification scn;
    scn.nmhdr.code = SCN_URIDROPPED;
    scn.text = list;

    NotifyParent(scn);
}

#ifndef EXT_INPUT
// Extended UTF8-UTF6-conversion to handle surrogate pairs correctly (CL265070)
int ScintillaMacOSX::KeyDefault(int key, int modifiers) {
    if (!(modifiers & SCI_CTRL) && !(modifiers & SCI_ALT) && (key < 256)) {
        AddChar(key);
        return 1;
    } else {
        // Pass up to container in case it is an accelerator
        NotifyKey(key, modifiers);
        return 0;
    }
    //Platform::DebugPrintf("SK-key: %d %x %x\n",key, modifiers);
}
#endif

template <class T, class U>
struct StupidMap
{
public:
    T key;
    U value;
};

template <class T, class U>
inline static U StupidMapFindFunction( const StupidMap<T, U>* elements, size_t length, const T& desiredKey )
{
    for ( size_t i = 0; i < length; ++ i )
    {
        if ( elements[i].key == desiredKey )
        {
            return elements[i].value;
        }
    }

    return NULL;
}

// NOTE: If this macro is used on a StupidMap that isn't defined by StupidMap x[] = ...
// The size calculation will fail!
#define StupidMapFind( x, y ) StupidMapFindFunction( x, sizeof(x)/sizeof(*x), y )

pascal OSStatus ScintillaMacOSX::CommandEventHandler( EventHandlerCallRef /*inCallRef*/, EventRef event, void* data )
{
    // TODO: Verify automatically that each constant only appears once?
    const StupidMap<UInt32, void (ScintillaMacOSX::*)()> processCommands[] = {
        { kHICommandCopy, &ScintillaMacOSX::Copy },
        { kHICommandPaste, &ScintillaMacOSX::Paste },
        { kHICommandCut, &ScintillaMacOSX::Cut },
        { kHICommandUndo, &ScintillaMacOSX::Undo },
        { kHICommandRedo, &ScintillaMacOSX::Redo },
        { kHICommandClear, &ScintillaMacOSX::ClearSelection },
        { kHICommandSelectAll, &ScintillaMacOSX::SelectAll },
    };
    const StupidMap<UInt32, bool (ScintillaMacOSX::*)()> canProcessCommands[] = {
        { kHICommandCopy, &ScintillaMacOSX::HasSelection },
        { kHICommandPaste, &ScintillaMacOSX::CanPaste },
        { kHICommandCut, &ScintillaMacOSX::HasSelection },
        { kHICommandUndo, &ScintillaMacOSX::CanUndo },
        { kHICommandRedo, &ScintillaMacOSX::CanRedo },
        { kHICommandClear, &ScintillaMacOSX::HasSelection },
        { kHICommandSelectAll, &ScintillaMacOSX::AlwaysTrue },
    };
    
    HICommand command;  
    OSStatus result = GetEventParameter( event, kEventParamDirectObject, typeHICommand, NULL, sizeof( command ), NULL, &command );
    assert( result == noErr );

    UInt32 kind = GetEventKind( event );
    Platform::DebugPrintf("ScintillaMacOSX::CommandEventHandler kind %d\n", kind);

    ScintillaMacOSX* scintilla = reinterpret_cast<ScintillaMacOSX*>( data );
    assert( scintilla != NULL );
    
    if ( kind == kEventProcessCommand )
    {
#ifdef EXT_INPUT
        // We are getting a HI command, so stop extended input
        ExtInput::stop (scintilla->GetViewRef());
#endif
        // Find the method pointer that matches this command
        void (ScintillaMacOSX::*methodPtr)() = StupidMapFind( processCommands, command.commandID );

        if ( methodPtr != NULL )
        {
            // Call the method if we found it, and tell the caller that we handled this event
            (scintilla->*methodPtr)();
            result = noErr;
        } else {
            // tell the caller that we did not handle the event
            result = eventNotHandledErr;
        }
    }
    // The default Mac OS X text editor does not handle these events to enable/disable menu items
    // Why not? I think it should, so Scintilla does.
    else if ( kind == kEventCommandUpdateStatus && ( command.attributes & kHICommandFromMenu ) )
    {
        // Find the method pointer that matches this command
        bool (ScintillaMacOSX::*methodPtr)() = StupidMapFind( canProcessCommands, command.commandID );

        if ( methodPtr != NULL ) {
            // Call the method if we found it: enabling/disabling menu items
            if ( (scintilla->*methodPtr)() ) {
                EnableMenuItem( command.menu.menuRef, command.menu.menuItemIndex );
            } else {
                DisableMenuItem( command.menu.menuRef, command.menu.menuItemIndex );
            }
            result = noErr;
        } else {
            // tell the caller that we did not handle the event
            result = eventNotHandledErr;
        }
    } else {
        // Unhandled event: We should never get here
        assert( false );
        result = eventNotHandledErr;
    }
    
    return result;
}

bool ScintillaMacOSX::HasSelection()
{
    return ( !sel.Empty() );
}

bool ScintillaMacOSX::CanUndo()
{
    return pdoc->CanUndo();
}

bool ScintillaMacOSX::CanRedo()
{
    return pdoc->CanRedo();
}

bool ScintillaMacOSX::AlwaysTrue()
{
    return true;
}

void ScintillaMacOSX::CopyToClipboard(const SelectionText &selectedText) {
    PasteboardRef theClipboard;
    SetPasteboardData(theClipboard, selectedText);
    // Done with the CFString
    CFRelease( theClipboard );
}

void ScintillaMacOSX::Copy()
{
    if (!sel.Empty()) {
#ifdef EXT_INPUT
        ExtInput::stop (GetViewRef()); 
#endif
        SelectionText selectedText;
        CopySelectionRange(&selectedText);
        fprintf(stderr, "copied text is rectangular? %d\n", selectedText.rectangular);
        CopyToClipboard(selectedText);
    }
}

bool ScintillaMacOSX::CanPaste()
{
    if (!Editor::CanPaste())
        return false;

    PasteboardRef theClipboard;
    bool isFileURL = false;
    
    PasteboardCreate( kPasteboardClipboard, &theClipboard );
    bool ok = GetPasteboardData(theClipboard, NULL, &isFileURL);
    CFRelease( theClipboard );
    return ok;
}

void ScintillaMacOSX::Paste()
{
    Paste(false);
}

// XXX there is no system flag (I can find) to tell us that a paste is rectangular, so 
//     applications must implement an additional command (eg. option-V like BBEdit)
//     in order to provide rectangular paste
void ScintillaMacOSX::Paste(bool forceRectangular)
{
    PasteboardRef theClipboard;
    SelectionText selectedText;
    selectedText.rectangular = forceRectangular;
    bool isFileURL = false;
    PasteboardCreate( kPasteboardClipboard, &theClipboard );
    bool ok = GetPasteboardData(theClipboard, &selectedText, &isFileURL);
    CFRelease( theClipboard );
    fprintf(stderr, "paste is rectangular? %d\n", selectedText.rectangular);
    if (!ok || !selectedText.s)
        // no data or no flavor we support
        return;

    pdoc->BeginUndoAction();
    ClearSelection();
    if (selectedText.rectangular) {
        SelectionPosition selStart = sel.RangeMain().Start();
        PasteRectangular(selStart, selectedText.s, selectedText.len);
    } else 
    if ( pdoc->InsertString( sel.RangeMain().caret.Position(), selectedText.s, selectedText.len ) ) {
        SetEmptySelection( sel.RangeMain().caret.Position() + selectedText.len );
    }

    pdoc->EndUndoAction();

    Redraw();
    EnsureCaretVisible();
}

void ScintillaMacOSX::CreateCallTipWindow(PRectangle rc) {
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
        ScintillaMacOSX* sciThis = this;
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
}

void ScintillaMacOSX::CallTipClick() 
{
    ScintillaBase::CallTipClick();
}

void ScintillaMacOSX::AddToPopUp( const char *label, int cmd, bool enabled )
{
    // Translate stuff into menu item attributes
    MenuItemAttributes attributes = 0;
    if ( label[0] == '\0' ) attributes |= kMenuItemAttrSeparator;
    if ( ! enabled ) attributes |= kMenuItemAttrDisabled;

    // Translate Scintilla commands into Mac OS commands
    // TODO: If I create an AEDesc, OS X may insert these standard
    // text editing commands into the menu for me
    MenuCommand macCommand;
    switch( cmd )
    {
    case idcmdUndo:
        macCommand = kHICommandUndo;
        break;
    case idcmdRedo:
        macCommand = kHICommandRedo;
        break;
    case idcmdCut:
        macCommand = kHICommandCut;
        break;
    case idcmdCopy:
        macCommand = kHICommandCopy;
        break;
    case idcmdPaste:
        macCommand = kHICommandPaste;
        break;
    case idcmdDelete:
        macCommand = kHICommandClear;
        break;
    case idcmdSelectAll:
        macCommand = kHICommandSelectAll;
        break;
    case 0:
        macCommand = 0;
        break;
    default:
        assert( false );
        return;
    }

    CFStringRef string = CFStringCreateWithCString( NULL, label,  kCFStringEncodingUTF8 );
    OSStatus err;
    err = AppendMenuItemTextWithCFString( reinterpret_cast<MenuRef>( popup.GetID() ),
                               string, attributes, macCommand, NULL );
    assert( err == noErr );

    CFRelease( string );
    string = NULL;
}

void ScintillaMacOSX::ClaimSelection() {
    // Mac OS X does not have a primary selection
}

/** A wrapper function to permit external processes to directly deliver messages to our "message loop". */
sptr_t ScintillaMacOSX::DirectFunction(
    ScintillaMacOSX *sciThis, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
    return sciThis->WndProc(iMessage, wParam, lParam);
}

sptr_t scintilla_send_message(void* sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
        HIViewRef control = reinterpret_cast<HIViewRef>(sci);
    // Platform::DebugPrintf("scintilla_send_message %08X control %08X\n",sci,control);
    // Get a reference to the Scintilla C++ object
    ScintillaMacOSX* scintilla = NULL;
    OSStatus err;
    err = GetControlProperty( control, scintillaMacOSType, 0, sizeof( scintilla ), NULL, &scintilla );
    assert( err == noErr && scintilla != NULL );
    //Platform::DebugPrintf("scintilla_send_message scintilla %08X\n",scintilla);

    return scintilla->WndProc(iMessage, wParam, lParam);
}

void ScintillaMacOSX::TimerFired( EventLoopTimerRef )
{
    Tick();
    DragScroll();
}

OSStatus ScintillaMacOSX::BoundsChanged( UInt32 /*inOptions*/, const HIRect& inOriginalBounds, const HIRect& inCurrentBounds, RgnHandle /*inInvalRgn*/ )
{
    // If the width or height changed, modify the scroll bars and notify Scintilla
    // This event is also delivered when the window moves, and we don't care about that
    if ( inOriginalBounds.size.width != inCurrentBounds.size.width || inOriginalBounds.size.height != inCurrentBounds.size.height )
    {
        Resize( static_cast<int>( inCurrentBounds.size.width ), static_cast<int>( inCurrentBounds.size.height ) );
    }
    return noErr;
}

void ScintillaMacOSX::Draw( RgnHandle rgn, CGContextRef gc )
{
    Rect invalidRect;
    GetRegionBounds( rgn, &invalidRect );

    // NOTE: We get draw events that include the area covered by the scroll bar. No fear: Scintilla correctly ignores them
    SyncPaint( gc, PRectangle( invalidRect.left, invalidRect.top, invalidRect.right, invalidRect.bottom ) );
}

ControlPartCode ScintillaMacOSX::HitTest( const HIPoint& where )
{
    if ( CGRectContainsPoint( Bounds(), where ) )
        return 1;
    else
        return kControlNoPart;
}

OSStatus ScintillaMacOSX::SetFocusPart( ControlPartCode desiredFocus, RgnHandle /*invalidRgn*/, Boolean /*inFocusEverything*/, ControlPartCode* outActualFocus )
{
    assert( outActualFocus != NULL );

    if ( desiredFocus == 0 ) {
        // We are losing the focus
        SetFocusState(false);
    } else {
        // We are getting the focus
        SetFocusState(true);
    }
    
    *outActualFocus = desiredFocus;
    return noErr;
}

// Map Mac Roman character codes to their equivalent Scintilla codes
static inline int KeyTranslate( UniChar unicodeChar )
{
    switch ( unicodeChar )
    {
    case kDownArrowCharCode:
        return SCK_DOWN;
    case kUpArrowCharCode:
        return SCK_UP;
    case kLeftArrowCharCode:
        return SCK_LEFT;
    case kRightArrowCharCode:
        return SCK_RIGHT;
    case kHomeCharCode:
        return SCK_HOME;
    case kEndCharCode:
        return SCK_END;
#ifndef EXT_INPUT
    case kPageUpCharCode:
        return SCK_PRIOR;
    case kPageDownCharCode:
        return SCK_NEXT;
#endif
    case kDeleteCharCode:
        return SCK_DELETE;
    // TODO: Is there an insert key in the mac world? My insert key is the "help" key
    case kHelpCharCode:
        return SCK_INSERT;
    case kEnterCharCode:
    case kReturnCharCode:
        return SCK_RETURN;
#ifdef EXT_INPUT
    // BP 2006-08-22: These codes below should not be translated. Otherwise TextInput() will fail for keys like SCK_ADD, which is '+'.
    case kBackspaceCharCode:
        return SCK_BACK;
    case kFunctionKeyCharCode:
    case kBellCharCode:
    case kVerticalTabCharCode:
    case kFormFeedCharCode:
    case 14:
    case 15:
    case kCommandCharCode:
    case kCheckCharCode:
    case kAppleLogoCharCode:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case kEscapeCharCode:
        return 0; // ignore
    default:
        return unicodeChar;
#else
    case kEscapeCharCode:
        return SCK_ESCAPE;
    case kBackspaceCharCode:
        return SCK_BACK;
    case '\t':
         return SCK_TAB;
    case '+':
        return SCK_ADD;
    case '-':
        return SCK_SUBTRACT;
    case '/':
        return SCK_DIVIDE;
    case kFunctionKeyCharCode:
        return kFunctionKeyCharCode;
    default:
        return 0;
#endif
    }
}

static inline UniChar GetCharacterWithoutModifiers( EventRef rawKeyboardEvent )
{
    UInt32 keyCode;
    // Get the key code from the raw key event
    GetEventParameter( rawKeyboardEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof( keyCode ), NULL, &keyCode );

    // Get the current keyboard layout
 // TODO: If this is a performance sink, we need to cache these values
    SInt16 lastKeyLayoutID = GetScriptVariable( /*currentKeyScript*/ GetScriptManagerVariable(smKeyScript), smScriptKeys);
    Handle uchrHandle = GetResource('uchr', lastKeyLayoutID);

	if (uchrHandle) {
		// Translate the key press ignoring ctrl and option
		UInt32 ignoredDeadKeys = 0;
		UInt32 ignoredActualLength = 0;
		UniChar unicodeKey = 0;
		// (((modifiers & shiftKey) >> 8) & 0xFF)
		OSStatus err;
		err = UCKeyTranslate( reinterpret_cast<UCKeyboardLayout*>( *uchrHandle ), keyCode, kUCKeyActionDown,
									/* modifierKeyState */ 0, LMGetKbdType(), kUCKeyTranslateNoDeadKeysMask, &ignoredDeadKeys,
									/* buffer length */ 1,
									/* actual length */ &ignoredActualLength,
									/* string */ &unicodeKey );
		assert( err == noErr );

		return unicodeKey;
	}
	return 0;
}

// Text input is very annoying:
// If the control key is pressed, or if the key is a "special" key (eg. arrow keys, function keys, whatever)
//  we let Scintilla handle it. If scintilla does not handle it, we do nothing (eventNotHandledErr).
// Otherwise, the event is just some text and we add it to the buffer
OSStatus ScintillaMacOSX::TextInput( TCarbonEvent& event )
{
    // Obtain the number of bytes of text
    UInt32 actualSize = 0;
    OSStatus err;
    err = event.GetParameterSize( kEventParamTextInputSendText, &actualSize );
    assert( err == noErr );
    assert( actualSize != 0 );

    const int numUniChars = actualSize / sizeof( UniChar );

#ifdef EXT_INPUT
    UniChar* text = new UniChar [numUniChars];
    err = event.GetParameter( kEventParamTextInputSendText, typeUnicodeText, actualSize, text );
    PLATFORM_ASSERT( err == noErr );

    int modifiers = GetCurrentEventKeyModifiers();
    
    // Loop over all characters in sequence
    for (int i = 0; i < numUniChars; i++)
    {
        UniChar key = KeyTranslate( text[i] );
        if (!key)
            continue;

        bool consumed = false;

        // need to go here first so e.g. Tab indentation works
        KeyDown ((int) key, (modifiers & shiftKey) != 0 || (modifiers & cmdKey) != 0, (modifiers & controlKey) != 0 || (modifiers & cmdKey) != 0,
                 (modifiers & optionKey) != 0 || (modifiers & cmdKey) != 0, &consumed);

        // BP 2007-01-08: 1452623 Second Cmd+s to save doc inserts an "s" into the text on Mac.
        // At this point we need to ignore all cmd/option keys with char value smaller than 32
        if( !consumed )
            consumed = ( modifiers & ( cmdKey | optionKey ) ) != 0 && text[i] < 32;

        // If not consumed, insert the original key
        if (!consumed)
            InsertCharacters (text+i, 1);
    }

    delete[] text;
    return noErr;
#else
    // Allocate a buffer for the text using Core Foundation
    UniChar* text = reinterpret_cast<UniChar*>( CFAllocatorAllocate( CFAllocatorGetDefault(), actualSize, 0 ) );
    assert( text != NULL );

    // Get a copy of the text
    err = event.GetParameter( kEventParamTextInputSendText, typeUnicodeText, actualSize, text );
    assert( err == noErr );

    // TODO: This is a gross hack to ignore function keys
    // Surely we can do better?
    if ( numUniChars == 1 && text[0] == kFunctionKeyCharCode ) return eventNotHandledErr;
    int modifiers = GetCurrentEventKeyModifiers();
    int scintillaKey = KeyTranslate( text[0] );

    // Create a CFString which wraps and takes ownership of the "text" buffer
    CFStringRef string = CFStringCreateWithCharactersNoCopy( NULL, text, numUniChars, NULL );
    assert( string != NULL );
    //delete text;
    text = NULL;

    // If we have a single unicode character that is special or 
    // to process a command. Try to do some translation.
    if ( numUniChars == 1 && ( modifiers & controlKey || scintillaKey != 0 ) ) {
        // If we have a modifier, we need to get the character without modifiers
        if ( modifiers & controlKey ) {
            EventRef rawKeyboardEvent = NULL;
            event.GetParameter(
                      kEventParamTextInputSendKeyboardEvent,
                      typeEventRef,
                      sizeof( EventRef ),
                      &rawKeyboardEvent );
            assert( rawKeyboardEvent != NULL );
            scintillaKey = GetCharacterWithoutModifiers( rawKeyboardEvent );

            // Make sure that we still handle special characters correctly
            int temp = KeyTranslate( scintillaKey );
            if ( temp != 0 ) scintillaKey = temp;

            // TODO: This is a gross Unicode hack: ASCII chars have a value < 127
            if ( scintillaKey <= 127 ) {
                scintillaKey = toupper( (char) scintillaKey );
            }
        }
            
        // Code taken from Editor::KeyDown
        // It is copied here because we don't want to feed the key via
        // KeyDefault if there is no special action
        DwellEnd(false);
        int scintillaModifiers = ( (modifiers & shiftKey) ? SCI_SHIFT : 0) | ( (modifiers & controlKey) ? SCI_CTRL : 0) |
            ( (modifiers & optionKey) ? SCI_ALT : 0);
        int msg = kmap.Find( scintillaKey, scintillaModifiers );
        if (msg) {
            // The keymap has a special event for this key: perform the operation
            WndProc(msg, 0, 0);
            err = noErr;
        } else {
            // We do not handle this event
            err = eventNotHandledErr;
        }
    } else {
        CFStringEncoding encoding = ( IsUnicodeMode() ? kCFStringEncodingUTF8 : kCFStringEncodingASCII);

        // Allocate the buffer (don't forget the null!)
        CFIndex maximumByteLength = CFStringGetMaximumSizeForEncoding( numUniChars, encoding ) + 1;
        char* buffer = new char[maximumByteLength];

        CFIndex usedBufferLength = 0;
        CFIndex numCharsConverted;
        numCharsConverted = CFStringGetBytes( string, CFRangeMake( 0, numUniChars ), encoding,
                                                '?', false, reinterpret_cast<UInt8*>( buffer ),
                                                maximumByteLength, &usedBufferLength );
        assert( numCharsConverted == numUniChars );
        buffer[usedBufferLength] = '\0'; // null terminate
        
        // Add all the characters to the document
        // NOTE: OS X doesn't specify that text input events provide only a single character
        // if we get a single character, add it as a character
        // otherwise, we insert the entire string
        if ( numUniChars == 1 ) {
            AddCharUTF( buffer, usedBufferLength );
        } else {
            // WARNING: This is an untested code path as with my US keyboard, I only enter a single character at a time
            if (pdoc->InsertString(sel.RangeMain().caret.Position(), buffer, usedBufferLength)) {
                SetEmptySelection(sel.RangeMain().caret.Position() + usedBufferLength);
            }
        }

        // Free the buffer that was allocated
        delete[] buffer;
        buffer = NULL;
        err = noErr;
    }

    // Default allocator releases both the CFString and the UniChar buffer (text)
    CFRelease( string );
    string = NULL;  
    
    return err;
#endif
}

UInt32 ScintillaMacOSX::GetBehaviors()
{
    return TView::GetBehaviors() | kControlGetsFocusOnClick | kControlSupportsEmbedding;
}

OSStatus ScintillaMacOSX::MouseEntered(HIPoint& location, UInt32 /*inKeyModifiers*/, EventMouseButton /*inMouseButton*/, UInt32 /*inClickCount*/ )
{
    if (!HaveMouseCapture() && HIViewGetSuperview(GetViewRef()) != NULL) {
        HIViewRef view;
        HIViewGetSubviewHit(reinterpret_cast<ControlRef>(wMain.GetID()), &location, true, &view);
        if (view) {
            // the hit is on a subview (ie. scrollbars)
            WndProc(SCI_SETCURSOR, Window::cursorArrow, 0);
        } else {
            // reset to normal, buttonmove will change for other area's in the editor
            WndProc(SCI_SETCURSOR, (long int)SC_CURSORNORMAL, 0);
            ButtonMove( Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ) );
        }
        return noErr;
    }
    return eventNotHandledErr;
}

OSStatus ScintillaMacOSX::MouseExited(HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount )
{
    if (HIViewGetSuperview(GetViewRef()) != NULL) {
        if (HaveMouseCapture()) {
            ButtonUp( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ),
                      static_cast<int>( GetCurrentEventTime() / kEventDurationMillisecond ), 
                      (modifiers & controlKey) != 0 );
        }
        WndProc(SCI_SETCURSOR, Window::cursorArrow, 0);
        return noErr;
    }
    return eventNotHandledErr;
}


OSStatus ScintillaMacOSX::MouseDown( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount , TCarbonEvent& inEvent)
{
    ConvertEventRefToEventRecord( inEvent.GetEventRef(), &mouseDownEvent );
    return MouseDown(location, modifiers, button, clickCount);
}

OSStatus ScintillaMacOSX::MouseDown( EventRecord *event )
{
    HIPoint pt = GetLocalPoint(event->where);
    int button = kEventMouseButtonPrimary;
    mouseDownEvent = *event;
  
    if ( event->modifiers & controlKey )
        button = kEventMouseButtonSecondary;
    return MouseDown(pt, event->modifiers, button, 1);
}

OSStatus ScintillaMacOSX::MouseDown( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 /*clickCount*/ )
{
    // We only deal with the first mouse button
    if ( button != kEventMouseButtonPrimary ) return eventNotHandledErr;
    // TODO: Verify that Scintilla wants the time in milliseconds
    if (!HaveMouseCapture() && HIViewGetSuperview(GetViewRef()) != NULL) {
        if (ScrollBarHit(location)) return noErr;
    }
    ButtonDown( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ),
             static_cast<int>( GetCurrentEventTime() / kEventDurationMillisecond ),
             (modifiers & shiftKey) != 0, 
             (modifiers & controlKey) != 0, 
             (modifiers & cmdKey) );
#if !defined(CONTAINER_HANDLES_EVENTS)
    OSStatus err;
    err = SetKeyboardFocus( this->GetOwner(), this->GetViewRef(), 1 );
    ::SetUserFocusWindow(::HIViewGetWindow( this->GetViewRef() ));
    return noErr;
#else
    return eventNotHandledErr; // allow event to go to container
#endif
}

OSStatus ScintillaMacOSX::MouseUp( EventRecord *event )
{
    HIPoint pt = GetLocalPoint(event->where);
    int button = kEventMouseButtonPrimary;
    if ( event->modifiers & controlKey )
        button = kEventMouseButtonSecondary;
    return MouseUp(pt, event->modifiers, button, 1);
}

OSStatus ScintillaMacOSX::MouseUp( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 /*clickCount*/ )
{
    // We only deal with the first mouse button
    if ( button != kEventMouseButtonPrimary ) return eventNotHandledErr;
        ButtonUp( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ),
               static_cast<int>( GetCurrentEventTime() / kEventDurationMillisecond ), 
               (modifiers & controlKey) != 0 );
    
#if !defined(CONTAINER_HANDLES_EVENTS)
    return noErr;
#else
    return eventNotHandledErr; // allow event to go to container
#endif
}

OSStatus ScintillaMacOSX::MouseDragged( EventRecord *event )
{
    HIPoint pt = GetLocalPoint(event->where);
    int button = 0;
    if ( event->modifiers & btnStateBit ) {
        button = kEventMouseButtonPrimary;
        if ( event->modifiers & controlKey )
            button = kEventMouseButtonSecondary;
    }
    return MouseDragged(pt, event->modifiers, button, 1);
}

OSStatus ScintillaMacOSX::MouseDragged( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount )
{
#if !defined(CONTAINER_HANDLES_EVENTS)
    ButtonMove( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ) );
    return noErr;
#else
    if (HaveMouseCapture() && !inDragDrop) {
        MouseTrackingResult mouseStatus = 0;
        ::Point theQDPoint;
        UInt32 outModifiers;
        EventTimeout inTimeout=0.1;
        while (mouseStatus != kMouseTrackingMouseReleased) {
            ButtonMove( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ) );
            TrackMouseLocationWithOptions((GrafPtr)-1,
                                          kTrackMouseLocationOptionDontConsumeMouseUp,
                                          inTimeout,
                                          &theQDPoint,
                                          &outModifiers,
                                          &mouseStatus);
            location = GetLocalPoint(theQDPoint);
        }
        ButtonUp( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ),
                  static_cast<int>( GetCurrentEventTime() / kEventDurationMillisecond ), 
                  (modifiers & controlKey) != 0 );
    } else {
        if (!HaveMouseCapture() && HIViewGetSuperview(GetViewRef()) != NULL) {
            HIViewRef view;
            HIViewGetSubviewHit(reinterpret_cast<ControlRef>(wMain.GetID()), &location, true, &view);
            if (view) {
                // the hit is on a subview (ie. scrollbars)
                WndProc(SCI_SETCURSOR, Window::cursorArrow, 0);
                return eventNotHandledErr;
            } else {
                // reset to normal, buttonmove will change for other area's in the editor
                WndProc(SCI_SETCURSOR, (long int)SC_CURSORNORMAL, 0);
            }
        }
        ButtonMove( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ) );
    }
    return eventNotHandledErr; // allow event to go to container
#endif
}

OSStatus ScintillaMacOSX::MouseWheelMoved( EventMouseWheelAxis axis, SInt32 delta, UInt32 modifiers )
{
    if ( axis != 1 ) return eventNotHandledErr;
    
    if ( modifiers & controlKey ) {
        // Zoom! We play with the font sizes in the styles.
        // Number of steps/line is ignored, we just care if sizing up or down
        if ( delta > 0 ) {
            KeyCommand( SCI_ZOOMIN );
        } else {
            KeyCommand( SCI_ZOOMOUT );
        }
    } else {
        // Decide if this should be optimized?
        ScrollTo( topLine - delta );
    }

    return noErr;
}

OSStatus ScintillaMacOSX::ContextualMenuClick( HIPoint& location )
{
    // convert screen coords to window relative
    Rect bounds;
    OSStatus err;
    err = GetWindowBounds( this->GetOwner(), kWindowContentRgn, &bounds );
    assert( err == noErr );
    location.x += bounds.left;
    location.y += bounds.top;
    ContextMenu( Scintilla::Point( static_cast<int>( location.x ), static_cast<int>( location.y ) ) );
    return noErr;   
}

OSStatus ScintillaMacOSX::ActiveStateChanged()
{
    // If the window is being deactivated, lose the focus and turn off the ticking
    if ( ! this->IsActive() ) {
        DropCaret();
        //SetFocusState( false );
        SetTicking( false );
    } else {
        ShowCaretAtCurrentPosition();
    }
    return noErr;
}

HIViewRef ScintillaMacOSX::Create()
{
    // Register the HIView, if needed
    static bool registered = false;

    if ( not registered ) {
        TView::RegisterSubclass( kScintillaClassID, Construct );
        registered = true;
    }

    OSStatus err = noErr;
    EventRef event = CreateInitializationEvent();
    assert( event != NULL );

    HIViewRef control = NULL;
    err = HIObjectCreate( kScintillaClassID, event, reinterpret_cast<HIObjectRef*>( &control ) );
    ReleaseEvent( event );
    if ( err == noErr ) {
        Platform::DebugPrintf("ScintillaMacOSX::Create control %08X\n",control);
        return control;
    }
    return NULL;    
}

OSStatus ScintillaMacOSX::Construct( HIViewRef inControl, TView** outView )
{
    *outView = new ScintillaMacOSX( inControl );
    Platform::DebugPrintf("ScintillaMacOSX::Construct scintilla %08X\n",*outView);
    if ( *outView != NULL )
        return noErr;
    else
        return memFullErr;  // could be a lie
}

extern "C" {
HIViewRef scintilla_new() {
    return ScintillaMacOSX::Create();
}
}
