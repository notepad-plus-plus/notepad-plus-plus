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

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ILoader.h"
#include "ILexer.h"

#include "CharacterCategory.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "CaseConvert.h"
#include "UniConversion.h"
#include "DBCS.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"

#include "AutoComplete.h"
#include "ScintillaBase.h"

extern "C" NSString *ScintillaRecPboardType;

@class SCIContentView;
@class SCIMarginView;
@class ScintillaView;

@class FindHighlightLayer;

/**
 * Helper class to be used as timer target (NSTimer).
 */
@interface TimerTarget : NSObject {
	void *mTarget;
	NSNotificationQueue *notificationQueue;
}
- (id) init: (void *) target;
- (void) timerFired: (NSTimer *) timer;
- (void) idleTimerFired: (NSTimer *) timer;
- (void) idleTriggered: (NSNotification *) notification;
@end

namespace Scintilla {

/**
 * Main scintilla class, implemented for OS X (Cocoa).
 */
class ScintillaCocoa : public ScintillaBase {
private:
	ScintillaView *sciView;
	TimerTarget *timerTarget;
	NSEvent *lastMouseEvent;

	id<ScintillaNotificationProtocol> delegate;

	SciNotifyFunc	notifyProc;
	intptr_t notifyObj;

	bool capturedMouse;

	bool enteredSetScrollingSize;

	bool GetPasteboardData(NSPasteboard *board, SelectionText *selectedText);
	void SetPasteboardData(NSPasteboard *board, const SelectionText &selectedText);
	Sci::Position TargetAsUTF8(char *text) const;
	Sci::Position EncodedFromUTF8(const char *utf8, char *encoded) const;

	int scrollSpeed;
	int scrollTicks;
	CFRunLoopObserverRef observer;

	FindHighlightLayer *layerFindIndicator;

protected:
	Point GetVisibleOriginInMain() const override;
	PRectangle GetClientRectangle() const override;
	PRectangle GetClientDrawingRectangle() override;
	Point ConvertPoint(NSPoint point);
	void RedrawRect(PRectangle rc) override;
	void DiscardOverdraw() override;
	void Redraw() override;

	void Init();
	CaseFolder *CaseFolderForEncoding() override;
	std::string CaseMapString(const std::string &s, int caseMapping) override;
	void CancelModes() override;

public:
	ScintillaCocoa(ScintillaView *sciView_, SCIContentView *viewContent, SCIMarginView *viewMargin);
	// Deleted so ScintillaCocoa objects can not be copied.
	ScintillaCocoa(const ScintillaCocoa &) = delete;
	ScintillaCocoa &operator=(const ScintillaCocoa &) = delete;
	~ScintillaCocoa() override;
	void Finalise() override;

	void SetDelegate(id<ScintillaNotificationProtocol> delegate_);
	void RegisterNotifyCallback(intptr_t windowid, SciNotifyFunc callback);
	sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;

	NSScrollView *ScrollContainer() const;
	SCIContentView *ContentView();

	bool SyncPaint(void *gc, PRectangle rc);
	bool Draw(NSRect rect, CGContextRef gc);
	void PaintMargin(NSRect aRect);

	sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) override;
	void TickFor(TickReason reason) override;
	bool FineTickerRunning(TickReason reason) override;
	void FineTickerStart(TickReason reason, int millis, int tolerance) override;
	void FineTickerCancel(TickReason reason) override;
	bool SetIdle(bool on) override;
	void SetMouseCapture(bool on) override;
	bool HaveMouseCapture() override;
	void WillDraw(NSRect rect);
	void ScrollText(Sci::Line linesToMove) override;
	void SetVerticalScrollPos() override;
	void SetHorizontalScrollPos() override;
	bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override;
	bool SetScrollingSize(void);
	void Resize();
	void UpdateForScroll();

	// Notifications for the owner.
	void NotifyChange() override;
	void NotifyFocus(bool focus) override;
	void NotifyParent(SCNotification scn) override;
	void NotifyURIDropped(const char *uri);

	bool HasSelection();
	bool CanUndo();
	bool CanRedo();
	void CopyToClipboard(const SelectionText &selectedText) override;
	void Copy() override;
	bool CanPaste() override;
	void Paste() override;
	void Paste(bool rectangular);
	void CTPaint(void *gc, NSRect rc);
	void CallTipMouseDown(NSPoint pt);
	void CreateCallTipWindow(PRectangle rc) override;
	void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
	void ClaimSelection() override;

	NSPoint GetCaretPosition();

	static sptr_t DirectFunction(sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam);

	NSTimer *timers[tickPlatform+1];
	void TimerFired(NSTimer *timer);
	void IdleTimerFired();
	static void UpdateObserver(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *sci);
	void ObserverAdd();
	void ObserverRemove();
	void IdleWork() override;
	void QueueIdleWork(WorkNeeded::workItems items, Sci::Position upTo) override;
	ptrdiff_t InsertText(NSString *input, CharacterSource charSource);
	NSRange PositionsFromCharacters(NSRange rangeCharacters) const;
	NSRange CharactersFromPositions(NSRange rangePositions) const;
	NSString *RangeTextAsString(NSRange rangePositions) const;
	NSInteger VisibleLineForIndex(NSInteger index);
	NSRange RangeForVisibleLine(NSInteger lineVisible);
	NSRect FrameForRange(NSRange rangeCharacters);
	NSRect GetBounds() const;
	void SelectOnlyMainSelection();
	void ConvertSelectionVirtualSpace();
	bool ClearAllSelections();
	void CompositionStart();
	void CompositionCommit();
	void CompositionUndo();
	void SetDocPointer(Document *document) override;

	bool KeyboardInput(NSEvent *event);
	void MouseDown(NSEvent *event);
	void RightMouseDown(NSEvent *event);
	void MouseMove(NSEvent *event);
	void MouseUp(NSEvent *event);
	void MouseEntered(NSEvent *event);
	void MouseExited(NSEvent *event);
	void MouseWheel(NSEvent *event);

	// Drag and drop
	void StartDrag() override;
	bool GetDragData(id <NSDraggingInfo> info, NSPasteboard &pasteBoard, SelectionText *selectedText);
	NSDragOperation DraggingEntered(id <NSDraggingInfo> info);
	NSDragOperation DraggingUpdated(id <NSDraggingInfo> info);
	void DraggingExited(id <NSDraggingInfo> info);
	bool PerformDragOperation(id <NSDraggingInfo> info);
	void DragScroll();

	// Promote some methods needed for NSResponder actions.
	void SelectAll() override;
	void DeleteBackward();
	void Cut() override;
	void Undo() override;
	void Redo() override;

	bool ShouldDisplayPopupOnMargin();
	bool ShouldDisplayPopupOnText();
	NSMenu *CreateContextMenu(NSEvent *event);
	void HandleCommand(NSInteger command);

	void ActiveStateChanged(bool isActive);
	void WindowWillMove();

	// Find indicator
	void ShowFindIndicatorForRange(NSRange charRange, BOOL retaining);
	void MoveFindIndicatorWithBounce(BOOL bounce);
	void HideFindIndicator();
};


}


