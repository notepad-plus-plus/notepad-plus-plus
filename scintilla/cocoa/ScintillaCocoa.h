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
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>

#include "ILexer.h"

#ifdef SCI_LEXER
#include "SciLexer.h"
#include "PropSetSimple.h"
#endif

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
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "CaseConvert.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"

#include "AutoComplete.h"
#include "ScintillaBase.h"

extern "C" NSString* ScintillaRecPboardType;

@class SCIContentView;
@class SCIMarginView;
@class ScintillaView;

@class FindHighlightLayer;

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
 * Main scintilla class, implemented for OS X (Cocoa).
 */
class ScintillaCocoa : public ScintillaBase
{
private:
  TimerTarget* timerTarget;
  NSEvent* lastMouseEvent;

  id<ScintillaNotificationProtocol> delegate;

  SciNotifyFunc	notifyProc;
  intptr_t notifyObj;

  bool capturedMouse;

  bool enteredSetScrollingSize;

  // Private so ScintillaCocoa objects can not be copied
  ScintillaCocoa(const ScintillaCocoa &) : ScintillaBase() {}
  ScintillaCocoa &operator=(const ScintillaCocoa &) { return * this; }

  bool GetPasteboardData(NSPasteboard* board, SelectionText* selectedText);
  void SetPasteboardData(NSPasteboard* board, const SelectionText& selectedText);
  int TargetAsUTF8(char *text);
  int EncodedFromUTF8(char *utf8, char *encoded) const;

  int scrollSpeed;
  int scrollTicks;
  NSTimer* tickTimer;
  NSTimer* idleTimer;
  CFRunLoopObserverRef observer;

  FindHighlightLayer *layerFindIndicator;

protected:
  Point GetVisibleOriginInMain() const;
  PRectangle GetClientRectangle() const;
  virtual PRectangle GetClientDrawingRectangle();
  Point ConvertPoint(NSPoint point);
  virtual void RedrawRect(PRectangle rc);
  virtual void DiscardOverdraw();
  virtual void Redraw();

  virtual void Initialise();
  virtual void Finalise();
  virtual CaseFolder *CaseFolderForEncoding();
  virtual std::string CaseMapString(const std::string &s, int caseMapping);
  virtual void CancelModes();

public:
  ScintillaCocoa(SCIContentView* view, SCIMarginView* viewMargin);
  virtual ~ScintillaCocoa();

  void SetDelegate(id<ScintillaNotificationProtocol> delegate_);
  void RegisterNotifyCallback(intptr_t windowid, SciNotifyFunc callback);
  sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);

  NSScrollView* ScrollContainer() const;
  SCIContentView* ContentView();

  bool SyncPaint(void* gc, PRectangle rc);
  bool Draw(NSRect rect, CGContextRef gc);
  void PaintMargin(NSRect aRect);

  virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
  void TickFor(TickReason reason);
  bool FineTickerAvailable();
  bool FineTickerRunning(TickReason reason);
  void FineTickerStart(TickReason reason, int millis, int tolerance);
  void FineTickerCancel(TickReason reason);
  bool SetIdle(bool on);
  void SetMouseCapture(bool on);
  bool HaveMouseCapture();
  void WillDraw(NSRect rect);
  void ScrollText(int linesToMove);
  void SetVerticalScrollPos();
  void SetHorizontalScrollPos();
  bool ModifyScrollBars(int nMax, int nPage);
  bool SetScrollingSize(void);
  void Resize();
  void UpdateForScroll();

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
  void CTPaint(void* gc, NSRect rc);
  void CallTipMouseDown(NSPoint pt);
  virtual void CreateCallTipWindow(PRectangle rc);
  virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
  virtual void ClaimSelection();

  NSPoint GetCaretPosition();

  static sptr_t DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam);

  NSTimer *timers[tickPlatform+1];
  void TimerFired(NSTimer* timer);
  void IdleTimerFired();
  static void UpdateObserver(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *sci);
  void ObserverAdd();
  void ObserverRemove();
  virtual void IdleWork();
  virtual void QueueIdleWork(WorkNeeded::workItems items, int upTo);
  int InsertText(NSString* input);
  NSRange PositionsFromCharacters(NSRange range) const;
  NSRange CharactersFromPositions(NSRange range) const;
  void SelectOnlyMainSelection();
  void ConvertSelectionVirtualSpace();
  bool ClearAllSelections();
  void CompositionStart();
  void CompositionCommit();
  void CompositionUndo();
  virtual void SetDocPointer(Document *document);

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

  virtual void ActiveStateChanged(bool isActive);

  // Find indicator
  void ShowFindIndicatorForRange(NSRange charRange, BOOL retaining);
  void MoveFindIndicatorWithBounce(BOOL bounce);
  void HideFindIndicator();
};


}


