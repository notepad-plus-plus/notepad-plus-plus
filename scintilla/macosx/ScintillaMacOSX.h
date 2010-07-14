/*
 *  ScintillaMacOSX.h
 *  tutorial
 *
 *  Original code by Evan Jones on Sun Sep 01 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 */
#include "TView.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include <vector>

#include "Platform.h"
#include "Scintilla.h"
#include "PlatMacOSX.h"


#include "ScintillaWidget.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#include "PropSet.h"
#include "PropSetSimple.h"
#include "Accessor.h"
#include "KeyWords.h"
#endif
#include "SVector.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "AutoComplete.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "ScintillaBase.h"
#include "ScintillaCallTip.h"

static const OSType scintillaMacOSType = 'Scin';

namespace Scintilla {

/**
On the Mac, there is no WM_COMMAND or WM_NOTIFY message that can be sent
back to the parent. Therefore, there must be a callback handler that acts
like a Windows WndProc, where Scintilla can send notifications to. Use
ScintillaMacOSX::registerNotifyHandler() to register such a handler.
Messgae format is:
<br>
WM_COMMAND: HIWORD (wParam) = notification code, LOWORD (wParam) = 0 (no control ID), lParam = ScintillaMacOSX*
<br>
WM_NOTIFY: wParam = 0 (no control ID), lParam = ptr to SCNotification structure, with hwndFrom set to ScintillaMacOSX*
*/
typedef void(*SciNotifyFunc) (intptr_t windowid, unsigned int iMessage, uintptr_t wParam, uintptr_t lParam);

/**
Scintilla sends these two messages to the nofity handler. Please refer
to the Windows API doc for details about the message format.
*/
#define	WM_COMMAND	1001
#define WM_NOTIFY	1002

class ScintillaMacOSX : public ScintillaBase, public TView
{
 public:
    HIViewRef vScrollBar;
    HIViewRef hScrollBar;
    SInt32 scrollBarFixedSize;
    SciNotifyFunc	notifyProc;
    intptr_t		notifyObj;

    bool capturedMouse;
    // true if scintilla initiated the drag session
    bool inDragSession() { return inDragDrop == ddDragging; }; 
    bool isTracking; 

    // Private so ScintillaMacOSX objects can not be copied
    ScintillaMacOSX(const ScintillaMacOSX &) : ScintillaBase(), TView( NULL ) {}
    ScintillaMacOSX &operator=(const ScintillaMacOSX &) { return * this; }

public:
    /** This is the class ID that we've assigned to Scintilla. */
    static const CFStringRef kScintillaClassID;
    static const ControlKind kScintillaKind;

    ScintillaMacOSX( void* windowid );
    virtual ~ScintillaMacOSX();
    //~ static void ClassInit(GtkObjectClass* object_class, GtkWidgetClass *widget_class);

    /** Returns the HIView object kind, needed to subclass TView. */
    virtual ControlKind GetKind() { return kScintillaKind; }

    /// Register the notify callback
    void registerNotifyCallback(intptr_t windowid, SciNotifyFunc callback);
private:
    virtual void Initialise();
    virtual void Finalise();
    
    // pasteboard support
    bool GetPasteboardData(PasteboardRef &pasteBoard,
                           SelectionText *selectedText, bool *isFileURL);
    void SetPasteboardData(PasteboardRef &pasteBoard,
                           const SelectionText &selectedText);
    char *GetStringFromCFString(CFStringRef &textString, int *textLen);

    // Drag and drop
    virtual void StartDrag();
    Scintilla::Point GetDragPoint(DragRef inDrag);
    bool GetDragData(DragRef inDrag, PasteboardRef &pasteBoard,
                         SelectionText *selectedText,
                         bool *isFileURL);
    void SetDragCursor(DragRef inDrag);
    virtual bool DragEnter(DragRef inDrag );
    virtual bool DragWithin(DragRef inDrag );
    virtual bool DragLeave(DragRef inDrag );
    virtual OSStatus DragReceive(DragRef inDrag );
    void DragScroll();
    int scrollSpeed;
    int scrollTicks;

    EventRecord mouseDownEvent;
    MouseTrackingRef mouseTrackingRef;
    MouseTrackingRegionID mouseTrackingID;
    HIPoint GetLocalPoint(::Point pt);

    void InsertCharacters (const UniChar* buf, int len);
    static pascal void IdleTimerEventHandler(EventLoopTimerRef inTimer,
                                             EventLoopIdleTimerMessage inState,
                                             void *scintilla );

public: // Public for scintilla_send_message
    virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

    void SyncPaint( void* gc, PRectangle rc);
    //void FullPaint( void* gc );
    virtual void Draw( RgnHandle rgn, CGContextRef gc );

    virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
    virtual void SetTicking(bool on);
    virtual bool SetIdle(bool on);
    virtual void SetMouseCapture(bool on);
    virtual bool HaveMouseCapture();
    virtual PRectangle GetClientRectangle();

    virtual void ScrollText(int linesToMove);
    virtual void SetVerticalScrollPos();
    virtual void SetHorizontalScrollPos();
    virtual bool ModifyScrollBars(int nMax, int nPage);
    virtual void ReconfigureScrollBars();
    void Resize(int width, int height);
    static pascal void LiveScrollHandler( ControlHandle control, SInt16 part );
    bool ScrollBarHit(HIPoint location);
    
    virtual void NotifyChange();
    virtual void NotifyFocus(bool focus);
    virtual void NotifyParent(SCNotification scn);
    void NotifyKey(int key, int modifiers);
    void NotifyURIDropped(const char *list);
#ifndef EXT_INPUT
    // Extended UTF8-UTF6-conversion to handle surrogate pairs correctly (CL265070)
    virtual int KeyDefault(int key, int modifiers);
#endif
    static pascal OSStatus CommandEventHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* data );

    bool HasSelection();
    bool CanUndo();
    bool CanRedo();
    bool AlwaysTrue();
    virtual void CopyToClipboard(const SelectionText &selectedText);
    virtual void Copy();
    virtual bool CanPaste();
    virtual void Paste();
    virtual void Paste(bool rectangular);
    virtual void CreateCallTipWindow(PRectangle rc);
    virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
    virtual void ClaimSelection();

    static sptr_t DirectFunction(ScintillaMacOSX *sciThis,
                                 unsigned int iMessage, uptr_t wParam, sptr_t lParam);

    /** Timer event handler. Simply turns around and calls Tick. */
    virtual void TimerFired( EventLoopTimerRef );
    virtual OSStatus BoundsChanged( UInt32 inOptions, const HIRect& inOriginalBounds, const HIRect& inCurrentBounds, RgnHandle inInvalRgn );
    virtual ControlPartCode HitTest( const HIPoint& where );
    virtual OSStatus SetFocusPart( ControlPartCode desiredFocus, RgnHandle, Boolean, ControlPartCode* outActualFocus );
    virtual OSStatus TextInput( TCarbonEvent& event );

    // Configure the features of this control
    virtual UInt32 GetBehaviors();

    virtual OSStatus MouseDown( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount, TCarbonEvent& inEvent );
    virtual OSStatus MouseDown( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseDown( EventRecord *rec );
    virtual OSStatus MouseUp( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseUp( EventRecord *rec );
    virtual OSStatus MouseDragged( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseDragged( EventRecord *rec );
    virtual OSStatus MouseEntered( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseExited( HIPoint& location, UInt32 modifiers, EventMouseButton button, UInt32 clickCount );
    virtual OSStatus MouseWheelMoved( EventMouseWheelAxis axis, SInt32 delta, UInt32 modifiers );
    virtual OSStatus ContextualMenuClick( HIPoint& location );

    virtual OSStatus ActiveStateChanged();

    virtual void CallTipClick();


public:
    static HIViewRef Create();
private:
    static OSStatus Construct( HIViewRef inControl, TView** outView );
    
};


}
