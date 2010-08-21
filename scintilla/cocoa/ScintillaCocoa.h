/*
 * ScintillaCocoa.h
 *
 * Mike Lischke <mlischke@sun.com>
 *
 * Based on ScintillaMacOSX.h
 * Original code by Evan Jones on Sun Sep 01 2002.
 *  Contributors:
 *  Shane Caraveo, ActiveState
 *  Bernd Paradies, Adobe
 *
 * Copyright 2009 Sun Microsystems, Inc. All rights reserved.
 * This file is dual licensed under LGPL v2.1 and the Scintilla license (http://www.scintilla.org/License.txt).
 */

#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include <vector>

#include "ILexer.h"

#ifdef SCI_LEXER
#include "SciLexer.h"
#include "PropSetSimple.h"
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
//#include "ScintillaCallTip.h"

#include "ScintillaBase.h"

extern "C" NSString* ScintillaRecPboardType;

@class ScintillaView;

/**
 * Helper class to be used as timer target (NSTimer).
 */
@interface TimerTarget : NSObject
{
  void* mTarget;
  NSNotificationQueue* notificationQueue;
}
- (id) init: (void*) target;
- (void) timerFired: (NSTimer*) timer;
- (void) idleTimerFired: (NSTimer*) timer;
- (void) idleTriggered: (NSNotification*) notification;
@end

namespace Scintilla {

/**
 * On the Mac, there is no WM_COMMAND or WM_NOTIFY message that can be sent
 * back to the parent. Therefore, there must be a callback handler that acts
 * like a Windows WndProc, where Scintilla can send notifications to. Use
 * ScintillaCocoa::RegisterNotifyHandler() to register such a handler.
 * Message format is:
 * <br>
 * WM_COMMAND: HIWORD (wParam) = notification code, LOWORD (wParam) = 0 (no control ID), lParam = ScintillaCocoa*
 * <br>
 * WM_NOTIFY: wParam = 0 (no control ID), lParam = ptr to SCNotification structure, with hwndFrom set to ScintillaCocoa*
 */
typedef void(*SciNotifyFunc) (intptr_t windowid, unsigned int iMessage, uintptr_t wParam, uintptr_t lParam);

/**
 * Scintilla sends these two messages to the nofity handler. Please refer
 * to the Windows API doc for details about the message format.
 */
#define	WM_COMMAND	1001
#define WM_NOTIFY	1002

/**
 * Main scintilla class, implemented for OS X (Cocoa).
 */
class ScintillaCocoa : public ScintillaBase
{
private:
  TimerTarget* timerTarget;
  NSEvent* lastMouseEvent;
  
  SciNotifyFunc	notifyProc;
  intptr_t notifyObj;

  bool capturedMouse;

  // Private so ScintillaCocoa objects can not be copied
  ScintillaCocoa(const ScintillaCocoa &) : ScintillaBase() {}
  ScintillaCocoa &operator=(const ScintillaCocoa &) { return * this; }

  bool GetPasteboardData(NSPasteboard* board, SelectionText* selectedText);
  void SetPasteboardData(NSPasteboard* board, const SelectionText& selectedText);
  
  int scrollSpeed;
  int scrollTicks;
protected:
  NSView* ContentView();
  PRectangle GetClientRectangle();
  Point ConvertPoint(NSPoint point);
  
  virtual void Initialise();
  virtual void Finalise();
  virtual std::string CaseMapString(const std::string &s, int caseMapping);
public:
  ScintillaCocoa(NSView* view);
  virtual ~ScintillaCocoa();

  void RegisterNotifyCallback(intptr_t windowid, SciNotifyFunc callback);
  sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

  ScintillaView* TopContainer();

  void SyncPaint(void* gc, PRectangle rc);
  void Draw(NSRect rect, CGContextRef gc);

  virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
  void SetTicking(bool on);
  bool SetIdle(bool on);
  void SetMouseCapture(bool on);
  bool HaveMouseCapture();
  void SetVerticalScrollPos();
  void SetHorizontalScrollPos();
  bool ModifyScrollBars(int nMax, int nPage);
  void Resize();
  void DoScroll(float position, NSScrollerPart part, bool horizontal);
    
  // Notifications for the owner.
  void NotifyChange();
  void NotifyFocus(bool focus);
  void NotifyParent(SCNotification scn);
  void NotifyURIDropped(const char *uri);

  bool HasSelection();
  bool CanUndo();
  bool CanRedo();
  virtual void CopyToClipboard(const SelectionText &selectedText);
  virtual void Copy();
  virtual bool CanPaste();
  virtual void Paste();
  virtual void Paste(bool rectangular);
  virtual void CreateCallTipWindow(PRectangle rc);
  virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
  virtual void ClaimSelection();

  NSPoint GetCaretPosition();
  
  static sptr_t DirectFunction(ScintillaCocoa *sciThis, unsigned int iMessage, uptr_t wParam, sptr_t lParam);

  void TimerFired(NSTimer* timer);
  void IdleTimerFired();
  int InsertText(NSString* input);
  
  bool KeyboardInput(NSEvent* event);
  void MouseDown(NSEvent* event);
  void MouseMove(NSEvent* event);
  void MouseUp(NSEvent* event);
  void MouseEntered(NSEvent* event);
  void MouseExited(NSEvent* event);
  void MouseWheel(NSEvent* event);

  // Drag and drop
  void StartDrag();
  bool GetDragData(id <NSDraggingInfo> info, NSPasteboard &pasteBoard, SelectionText* selectedText);
  NSDragOperation DraggingEntered(id <NSDraggingInfo> info);
  NSDragOperation DraggingUpdated(id <NSDraggingInfo> info);
  void DraggingExited(id <NSDraggingInfo> info);
  bool PerformDragOperation(id <NSDraggingInfo> info);
  void DragScroll();
  
  // Promote some methods needed for NSResponder actions.
  virtual void SelectAll();
  void DeleteBackward();
  virtual void Cut();
  virtual void Undo();
  virtual void Redo();
  
  virtual NSMenu* CreateContextMenu(NSEvent* event);
  void HandleCommand(NSInteger command);

//    virtual OSStatus ActiveStateChanged();
//
//    virtual void CallTipClick();
 
};


}


