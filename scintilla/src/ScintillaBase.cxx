// Scintilla source code edit control
/** @file ScintillaBase.cxx
 ** An enhanced subclass of Editor with calltips, autocomplete and context menu.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterCategoryMap.h"

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
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "ScintillaBase.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaBase::ScintillaBase() {
	displayPopupMenu = PopUp::All;
	listType = 0;
	maxListWidth = 0;
	multiAutoCMode = MultiAutoComplete::Once;
}

ScintillaBase::~ScintillaBase() = default;

void ScintillaBase::Finalise() {
	Editor::Finalise();
	popup.Destroy();
}

void ScintillaBase::InsertCharacter(std::string_view sv, CharacterSource charSource) {
	const bool acActive = ac.Active();
	const bool isFillUp = acActive && ac.IsFillUpChar(sv[0]);
	if (!isFillUp) {
		Editor::InsertCharacter(sv, charSource);
	}
	if (acActive && ac.Active()) { // if it was and still is active
		AutoCompleteCharacterAdded(sv[0]);
		// For fill ups add the character after the autocompletion has
		// triggered so containers see the key so can display a calltip.
		if (isFillUp) {
			Editor::InsertCharacter(sv, charSource);
		}
	}
}

void ScintillaBase::Command(int cmdId) {

	switch (cmdId) {

	case idAutoComplete:  	// Nothing to do

		break;

	case idCallTip:  	// Nothing to do

		break;

	case idcmdUndo:
		WndProc(Message::Undo, 0, 0);
		break;

	case idcmdRedo:
		WndProc(Message::Redo, 0, 0);
		break;

	case idcmdCut:
		WndProc(Message::Cut, 0, 0);
		break;

	case idcmdCopy:
		WndProc(Message::Copy, 0, 0);
		break;

	case idcmdPaste:
		WndProc(Message::Paste, 0, 0);
		break;

	case idcmdDelete:
		WndProc(Message::Clear, 0, 0);
		break;

	case idcmdSelectAll:
		WndProc(Message::SelectAll, 0, 0);
		break;

	default:
		break;
	}
}

int ScintillaBase::KeyCommand(Message iMessage) {
	// Most key commands cancel autocompletion mode
	if (ac.Active()) {
		switch (iMessage) {
			// Except for these
		case Message::LineDown:
			AutoCompleteMove(1);
			return 0;
		case Message::LineUp:
			AutoCompleteMove(-1);
			return 0;
		case Message::PageDown:
			AutoCompleteMove(ac.lb->GetVisibleRows());
			return 0;
		case Message::PageUp:
			AutoCompleteMove(-ac.lb->GetVisibleRows());
			return 0;
		case Message::VCHome:
			AutoCompleteMove(-5000);
			return 0;
		case Message::LineEnd:
			AutoCompleteMove(5000);
			return 0;
		case Message::DeleteBack:
			DelCharBack(true);
			AutoCompleteCharacterDeleted();
			EnsureCaretVisible();
			return 0;
		case Message::DeleteBackNotLine:
			DelCharBack(false);
			AutoCompleteCharacterDeleted();
			EnsureCaretVisible();
			return 0;
		case Message::Tab:
			AutoCompleteCompleted(0, CompletionMethods::Tab);
			return 0;
		case Message::NewLine:
			AutoCompleteCompleted(0, CompletionMethods::Newline);
			return 0;

		default:
			AutoCompleteCancel();
		}
	}

	if (ct.inCallTipMode) {
		if (
		    (iMessage != Message::CharLeft) &&
		    (iMessage != Message::CharLeftExtend) &&
		    (iMessage != Message::CharRight) &&
		    (iMessage != Message::CharRightExtend) &&
		    (iMessage != Message::EditToggleOvertype) &&
		    (iMessage != Message::DeleteBack) &&
		    (iMessage != Message::DeleteBackNotLine)
		) {
			ct.CallTipCancel();
		}
		if ((iMessage == Message::DeleteBack) || (iMessage == Message::DeleteBackNotLine)) {
			if (sel.MainCaret() <= ct.posStartCallTip) {
				ct.CallTipCancel();
			}
		}
	}
	return Editor::KeyCommand(iMessage);
}

void ScintillaBase::ListNotify(ListBoxEvent *plbe) {
	switch (plbe->event) {
	case ListBoxEvent::EventType::selectionChange:
		AutoCompleteSelection();
		break;
	case ListBoxEvent::EventType::doubleClick:
		AutoCompleteCompleted(0, CompletionMethods::DoubleClick);
		break;
	}
}

void ScintillaBase::MoveImeCarets(Sci::Position offset) noexcept {
	// Move carets relatively by bytes.
	for (size_t r = 0; r < sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		sel.Range(r) = SelectionRange(positionInsert + offset);
	}
}

void ScintillaBase::DrawImeIndicator(int indicator, Sci::Position len) {
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after InsertCharacter().
	// It does not affect caret positions.
	const IndicatorNumbers ind = static_cast<IndicatorNumbers>(indicator);
	if (ind < IndicatorNumbers::Container || ind > IndicatorNumbers::Max) {
		return;
	}
	pdoc->DecorationSetCurrentIndicator(indicator);
	for (size_t r = 0; r < sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

void ScintillaBase::AutoCompleteInsert(Sci::Position startPos, Sci::Position removeLen, std::string_view text) {
	UndoGroup ug(pdoc);
	if (multiAutoCMode == MultiAutoComplete::Once) {
		pdoc->DeleteChars(startPos, removeLen);
		const Sci::Position lengthInserted = pdoc->InsertString(startPos, text);
		SetEmptySelection(startPos + lengthInserted);
	} else {
		// MultiAutoComplete::Each
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r))) {
				Sci::Position positionInsert = sel.Range(r).Start().Position();
				positionInsert = RealizeVirtualSpace(positionInsert, sel.Range(r).caret.VirtualSpace());
				if (positionInsert - removeLen >= 0) {
					positionInsert -= removeLen;
					pdoc->DeleteChars(positionInsert, removeLen);
				}
				const Sci::Position lengthInserted = pdoc->InsertString(positionInsert, text);
				if (lengthInserted > 0) {
					sel.Range(r) = SelectionRange(positionInsert + lengthInserted);
				}
				sel.Range(r).ClearVirtualSpace();
			}
		}
	}
}

void ScintillaBase::AutoCompleteStart(Sci::Position lenEntered, const char *list) {
	//Platform::DebugPrintf("AutoComplete %s\n", list);
	ct.CallTipCancel();

	if (ac.chooseSingle && (listType == 0)) {
		if (list && !strchr(list, ac.GetSeparator())) {
			// list contains just one item so choose it
			const std::string_view item(list);
			const std::string_view choice = item.substr(0, item.find_first_of(ac.GetTypesep()));
			if (ac.ignoreCase) {
				// May need to convert the case before invocation, so remove lenEntered characters
				AutoCompleteInsert(sel.MainCaret() - lenEntered, lenEntered, choice);
			} else {
				AutoCompleteInsert(sel.MainCaret(), 0, choice.substr(lenEntered));
			}
			const Sci::Position firstPos = sel.MainCaret() - lenEntered;
			// Construct a string with a NUL at end as that is expected by applications
			const std::string selected(choice);
			AutoCompleteNotifyCompleted('\0', CompletionMethods::SingleChoice, firstPos, selected.c_str());

			ac.Cancel();
			return;
		}
	}

	const ListOptions options{
		vs.ElementColour(Element::List),
		vs.ElementColour(Element::ListBack),
		vs.ElementColour(Element::ListSelected),
		vs.ElementColour(Element::ListSelectedBack),
		ac.options,
		ac.imageScale,
	};

	int lineHeight;
	if (vs.autocStyle != StyleDefault) {
		AutoSurface surfaceMeasure(this);
		lineHeight = static_cast<int>(std::lround(surfaceMeasure->Height(vs.styles[vs.autocStyle].font.get())));
	} else {
		lineHeight = vs.lineHeight;
	}

	ac.Start(wMain, idAutoComplete, sel.MainCaret(), PointMainCaret(),
				lenEntered, lineHeight, IsUnicodeMode(), technology, options);

	const PRectangle rcClient = GetClientRectangle();
	Point pt = LocationFromPosition(sel.MainCaret() - lenEntered);
	PRectangle rcPopupBounds = wMain.GetMonitorRect(pt);
	if (rcPopupBounds.Height() == 0)
		rcPopupBounds = rcClient;

	int heightLB = ac.heightLBDefault;
	int widthLB = ac.widthLBDefault;
	if (pt.x >= rcClient.right - widthLB) {
		HorizontalScrollTo(static_cast<int>(xOffset + pt.x - rcClient.right + widthLB));
		Redraw();
		pt = PointMainCaret();
	}
	if (wMargin.Created()) {
		pt = pt + GetVisibleOriginInMain();
	}
	PRectangle rcac;
	rcac.left = pt.x - ac.lb->CaretFromEdge();
	if (pt.y >= rcPopupBounds.bottom - heightLB &&  // Won't fit below.
	        pt.y >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2) { // and there is more room above.
		rcac.top = pt.y - heightLB;
		if (rcac.top < rcPopupBounds.top) {
			heightLB -= static_cast<int>(rcPopupBounds.top - rcac.top);
			rcac.top = rcPopupBounds.top;
		}
	} else {
		rcac.top = pt.y + vs.lineHeight;
	}
	rcac.right = rcac.left + widthLB;
	rcac.bottom = static_cast<XYPOSITION>(std::min(static_cast<int>(rcac.top) + heightLB, static_cast<int>(rcPopupBounds.bottom)));
	ac.lb->SetPositionRelative(rcac, &wMain);
	ac.lb->SetFont(vs.styles[vs.autocStyle].font.get());
	const int aveCharWidth = static_cast<int>(vs.styles[vs.autocStyle].aveCharWidth);
	ac.lb->SetAverageCharWidth(aveCharWidth);
	ac.lb->SetDelegate(this);

	ac.SetList(list ? list : "");

	// Fiddle the position of the list so it is right next to the target and wide enough for all its strings
	PRectangle rcList = ac.lb->GetDesiredRect();
	const int heightAlloced = static_cast<int>(rcList.bottom - rcList.top);
	widthLB = std::max(widthLB, static_cast<int>(rcList.right - rcList.left));
	if (maxListWidth != 0)
		widthLB = std::min(widthLB, aveCharWidth*maxListWidth);
	// Make an allowance for large strings in list
	rcList.left = pt.x - ac.lb->CaretFromEdge();
	rcList.right = rcList.left + widthLB;
	if (((pt.y + vs.lineHeight) >= (rcPopupBounds.bottom - heightAlloced)) &&  // Won't fit below.
	        ((pt.y + vs.lineHeight / 2) >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2)) { // and there is more room above.
		rcList.top = pt.y - heightAlloced;
	} else {
		rcList.top = pt.y + vs.lineHeight;
	}
	rcList.bottom = rcList.top + heightAlloced;
	ac.lb->SetPositionRelative(rcList, &wMain);
	ac.Show(true);
	if (lenEntered != 0) {
		AutoCompleteMoveToCurrentWord();
	}
}

void ScintillaBase::AutoCompleteCancel() {
	if (ac.Active()) {
		NotificationData scn = {};
		scn.nmhdr.code = Notification::AutoCCancelled;
		scn.wParam = 0;
		scn.listType = 0;
		NotifyParent(scn);
	}
	ac.Cancel();
}

void ScintillaBase::AutoCompleteMove(int delta) {
	ac.Move(delta);
}

void ScintillaBase::AutoCompleteMoveToCurrentWord() {
	if (FlagSet(ac.options, AutoCompleteOption::SelectFirstItem))
		return;
	std::string wordCurrent = RangeText(ac.posStart - ac.startLen, sel.MainCaret());
	ac.Select(wordCurrent.c_str());
}

void ScintillaBase::AutoCompleteSelection() {
	const int item = ac.GetSelection();
	std::string selected;
	if (item != -1) {
		selected = ac.GetValue(item);
	}

	NotificationData scn = {};
	scn.nmhdr.code = Notification::AutoCSelectionChange;
	scn.message = static_cast<Message>(0);
	scn.wParam = listType;
	scn.listType = listType;
	const Sci::Position firstPos = ac.posStart - ac.startLen;
	scn.position = firstPos;
	scn.lParam = firstPos;
	scn.text = selected.c_str();
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteCharacterAdded(char ch) {
	if (ac.IsFillUpChar(ch)) {
		AutoCompleteCompleted(ch, CompletionMethods::FillUp);
	} else if (ac.IsStopChar(ch)) {
		AutoCompleteCancel();
	} else {
		AutoCompleteMoveToCurrentWord();
	}
}

void ScintillaBase::AutoCompleteCharacterDeleted() {
	if (sel.MainCaret() < ac.posStart - ac.startLen) {
		AutoCompleteCancel();
	} else if (ac.cancelAtStartPos && (sel.MainCaret() <= ac.posStart)) {
		AutoCompleteCancel();
	} else {
		AutoCompleteMoveToCurrentWord();
	}
	NotificationData scn = {};
	scn.nmhdr.code = Notification::AutoCCharDeleted;
	scn.wParam = 0;
	scn.listType = 0;
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteNotifyCompleted(char ch, CompletionMethods completionMethod, Sci::Position firstPos, const char *text) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::AutoCCompleted;
	scn.message = static_cast<Message>(0);
	scn.ch = ch;
	scn.listCompletionMethod = completionMethod;
	scn.wParam = listType;
	scn.listType = listType;
	scn.position = firstPos;
	scn.lParam = firstPos;
	scn.text = text;
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteCompleted(char ch, CompletionMethods completionMethod) {
	const int item = ac.GetSelection();
	if (item == -1) {
		AutoCompleteCancel();
		return;
	}
	const std::string selected = ac.GetValue(item);

	ac.Show(false);

	NotificationData scn = {};
	scn.nmhdr.code = listType > 0 ? Notification::UserListSelection : Notification::AutoCSelection;
	scn.message = static_cast<Message>(0);
	scn.ch = ch;
	scn.listCompletionMethod = completionMethod;
	scn.wParam = listType;
	scn.listType = listType;
	const Sci::Position firstPos = ac.posStart - ac.startLen;
	scn.position = firstPos;
	scn.lParam = firstPos;
	scn.text = selected.c_str();
	NotifyParent(scn);

	if (!ac.Active())
		return;
	ac.Cancel();

	if (listType > 0)
		return;

	Sci::Position endPos = sel.MainCaret();
	if (ac.dropRestOfWord)
		endPos = pdoc->ExtendWordSelect(endPos, 1, true);
	if (endPos < firstPos)
		return;
	AutoCompleteInsert(firstPos, endPos - firstPos, selected);
	SetLastXChosen();

	AutoCompleteNotifyCompleted(ch, completionMethod, firstPos, selected.c_str());
}

int ScintillaBase::AutoCompleteGetCurrent() const {
	if (!ac.Active())
		return -1;
	return ac.GetSelection();
}

int ScintillaBase::AutoCompleteGetCurrentText(char *buffer) const {
	if (ac.Active()) {
		const int item = ac.GetSelection();
		if (item != -1) {
			const std::string selected = ac.GetValue(item);
			if (buffer)
				memcpy(buffer, selected.c_str(), selected.length()+1);
			return static_cast<int>(selected.length());
		}
	}
	if (buffer)
		*buffer = '\0';
	return 0;
}

void ScintillaBase::CallTipShow(Point pt, const char *defn) {
	ac.Cancel();
	// If container knows about StyleCallTip then use it in place of the
	// StyleDefault for the face name, size and character set. Also use it
	// for the foreground and background colour.
	const int ctStyle = ct.UseStyleCallTip() ? StyleCallTip : StyleDefault;
	const Style &style = vs.styles[ctStyle];
	if (ct.UseStyleCallTip()) {
		ct.SetForeBack(style.fore, style.back);
	}
	if (wMargin.Created()) {
		pt = pt + GetVisibleOriginInMain();
	}
	AutoSurface surfaceMeasure(this);
	PRectangle rc = ct.CallTipStart(sel.MainCaret(), pt,
		vs.lineHeight,
		defn,
		CodePage(),
		surfaceMeasure,
		style.font);
	// If the call-tip window would be out of the client
	// space
	const PRectangle rcClient = GetClientRectangle();
	const int offset = vs.lineHeight + static_cast<int>(rc.Height());
	// adjust so it displays above the text.
	if (rc.bottom > rcClient.bottom && rc.Height() < rcClient.Height()) {
		rc.top -= offset;
		rc.bottom -= offset;
	}
	// adjust so it displays below the text.
	if (rc.top < rcClient.top && rc.Height() < rcClient.Height()) {
		rc.top += offset;
		rc.bottom += offset;
	}
	// Now display the window.
	CreateCallTipWindow(rc);
	ct.wCallTip.SetPositionRelative(rc, &wMain);
	ct.wCallTip.Show();
	ct.wCallTip.InvalidateAll();
}

void ScintillaBase::CallTipClick() {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::CallTipClick;
	scn.position = ct.clickPlace;
	NotifyParent(scn);
}

bool ScintillaBase::ShouldDisplayPopup(Point ptInWindowCoordinates) const {
	return (displayPopupMenu == PopUp::All ||
		(displayPopupMenu == PopUp::Text && !PointInSelMargin(ptInWindowCoordinates)));
}

void ScintillaBase::ContextMenu(Point pt) {
	if (displayPopupMenu != PopUp::Never) {
		const bool writable = !WndProc(Message::GetReadOnly, 0, 0);
		popup.CreatePopUp();
		AddToPopUp("Undo", idcmdUndo, writable && pdoc->CanUndo());
		AddToPopUp("Redo", idcmdRedo, writable && pdoc->CanRedo());
		AddToPopUp("");
		AddToPopUp("Cut", idcmdCut, writable && !sel.Empty());
		AddToPopUp("Copy", idcmdCopy, !sel.Empty());
		AddToPopUp("Paste", idcmdPaste, writable && WndProc(Message::CanPaste, 0, 0));
		AddToPopUp("Delete", idcmdDelete, writable && !sel.Empty());
		AddToPopUp("");
		AddToPopUp("Select All", idcmdSelectAll);
		popup.Show(pt, wMain);
	}
}

void ScintillaBase::CancelModes() {
	AutoCompleteCancel();
	ct.CallTipCancel();
	Editor::CancelModes();
}

void ScintillaBase::ButtonDownWithModifiers(Point pt, unsigned int curTime, KeyMod modifiers) {
	CancelModes();
	Editor::ButtonDownWithModifiers(pt, curTime, modifiers);
}

void ScintillaBase::RightButtonDownWithModifiers(Point pt, unsigned int curTime, KeyMod modifiers) {
	CancelModes();
	Editor::RightButtonDownWithModifiers(pt, curTime, modifiers);
}

namespace Scintilla::Internal {

class LexState : public LexInterface {
public:
	explicit LexState(Document *pdoc_) noexcept;

	// LexInterface deleted the standard operators and defined the virtual destructor so don't need to here.

	const char *DescribeWordListSets();
	void SetWordList(int n, const char *wl);
	[[nodiscard]] int GetIdentifier() const;
	[[nodiscard]] const char *GetName() const;
	void *PrivateCall(int operation, void *pointer);
	const char *PropertyNames();
	TypeProperty PropertyType(const char *name);
	const char *DescribeProperty(const char *name);
	void PropSet(const char *key, const char *val);
	const char *PropGet(const char *key) const;
	int PropGetInt(const char *key, int defaultValue=0) const;

	LineEndType LineEndTypesSupported() override;
	int AllocateSubStyles(int styleBase, int numberStyles);
	int SubStylesStart(int styleBase);
	int SubStylesLength(int styleBase);
	int StyleFromSubStyle(int subStyle);
	int PrimaryStyleFromStyle(int style);
	void FreeSubStyles();
	void SetIdentifiers(int style, const char *identifiers);
	int DistanceToSecondaryStyles();
	const char *GetSubStyleBases();
	int NamedStyles();
	const char *NameOfStyle(int style);
	const char *TagsOfStyle(int style);
	const char *DescriptionOfStyle(int style);
};

}

LexState::LexState(Document *pdoc_) noexcept : LexInterface(pdoc_) {
}

LexState *ScintillaBase::DocumentLexState() {
	if (!pdoc->GetLexInterface()) {
		pdoc->SetLexInterface(std::make_unique<LexState>(pdoc));
	}
	return dynamic_cast<LexState *>(pdoc->GetLexInterface());
}

const char *LexState::DescribeWordListSets() {
	if (instance) {
		return instance->DescribeWordListSets();
	}
	return nullptr;
}

void LexState::SetWordList(int n, const char *wl) {
	if (instance) {
		const Sci_Position firstModification = instance->WordListSet(n, wl);
		if (firstModification >= 0) {
			pdoc->ModifiedAt(firstModification);
		}
	}
}

int LexState::GetIdentifier() const {
	if (instance) {
		return instance->GetIdentifier();
	}
	return 0;
}

const char *LexState::GetName() const {
	if (instance) {
		return instance->GetName();
	}
	return "";
}

void *LexState::PrivateCall(int operation, void *pointer) {
	if (instance) {
		return instance->PrivateCall(operation, pointer);
	}
	return nullptr;
}

const char *LexState::PropertyNames() {
	if (instance) {
		return instance->PropertyNames();
	}
	return nullptr;
}

TypeProperty LexState::PropertyType(const char *name) {
	if (instance) {
		return static_cast<TypeProperty>(instance->PropertyType(name));
	}
	return TypeProperty::Boolean;
}

const char *LexState::DescribeProperty(const char *name) {
	if (instance) {
		return instance->DescribeProperty(name);
	}
	return nullptr;
}

void LexState::PropSet(const char *key, const char *val) {
	if (instance) {
		const Sci_Position firstModification = instance->PropertySet(key, val);
		if (firstModification >= 0) {
			pdoc->ModifiedAt(firstModification);
		}
	}
}

const char *LexState::PropGet(const char *key) const {
	if (instance) {
		return instance->PropertyGet(key);
	}
	return nullptr;
}

int LexState::PropGetInt(const char *key, int defaultValue) const {
	if (instance) {
		const char *value = instance->PropertyGet(key);
		if (value && *value) {
			return atoi(value);
		}
	}
	return defaultValue;
}

LineEndType LexState::LineEndTypesSupported() {
	if (instance) {
		return static_cast<LineEndType>(instance->LineEndTypesSupported());
	}
	return LineEndType::Default;
}

int LexState::AllocateSubStyles(int styleBase, int numberStyles) {
	if (instance) {
		return instance->AllocateSubStyles(styleBase, numberStyles);
	}
	return -1;
}

int LexState::SubStylesStart(int styleBase) {
	if (instance) {
		return instance->SubStylesStart(styleBase);
	}
	return -1;
}

int LexState::SubStylesLength(int styleBase) {
	if (instance) {
		return instance->SubStylesLength(styleBase);
	}
	return 0;
}

int LexState::StyleFromSubStyle(int subStyle) {
	if (instance) {
		return instance->StyleFromSubStyle(subStyle);
	}
	return 0;
}

int LexState::PrimaryStyleFromStyle(int style) {
	if (instance) {
		return instance->PrimaryStyleFromStyle(style);
	}
	return 0;
}

void LexState::FreeSubStyles() {
	if (instance) {
		instance->FreeSubStyles();
	}
}

void LexState::SetIdentifiers(int style, const char *identifiers) {
	if (instance) {
		instance->SetIdentifiers(style, identifiers);
		pdoc->ModifiedAt(0);
	}
}

int LexState::DistanceToSecondaryStyles() {
	if (instance) {
		return instance->DistanceToSecondaryStyles();
	}
	return 0;
}

const char *LexState::GetSubStyleBases() {
	if (instance) {
		return instance->GetSubStyleBases();
	}
	return "";
}

int LexState::NamedStyles() {
	if (instance) {
		return instance->NamedStyles();
	}
	return -1;
}

const char *LexState::NameOfStyle(int style) {
	if (instance) {
		return instance->NameOfStyle(style);
	}
	return nullptr;
}

const char *LexState::TagsOfStyle(int style) {
	if (instance) {
		return instance->TagsOfStyle(style);
	}
	return nullptr;
}

const char *LexState::DescriptionOfStyle(int style) {
	if (instance) {
		return instance->DescriptionOfStyle(style);
	}
	return nullptr;
}

void ScintillaBase::NotifyStyleToNeeded(Sci::Position endStyleNeeded) {
	if (!DocumentLexState()->UseContainerLexing()) {
		const Sci::Position startStyling = pdoc->LineStartPosition(pdoc->GetEndStyled());
		DocumentLexState()->Colourise(startStyling, endStyleNeeded);
		return;
	}
	Editor::NotifyStyleToNeeded(endStyleNeeded);
}

sptr_t ScintillaBase::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case Message::AutoCShow:
		listType = 0;
		AutoCompleteStart(PositionFromUPtr(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::AutoCCancel:
		ac.Cancel();
		break;

	case Message::AutoCActive:
		return ac.Active();

	case Message::AutoCPosStart:
		return ac.posStart;

	case Message::AutoCComplete:
		AutoCompleteCompleted(0, CompletionMethods::Command);
		break;

	case Message::AutoCSetSeparator:
		ac.SetSeparator(static_cast<char>(wParam));
		break;

	case Message::AutoCGetSeparator:
		return ac.GetSeparator();

	case Message::AutoCStops:
		ac.SetStopChars(ConstCharPtrFromSPtr(lParam));
		break;

	case Message::AutoCSelect:
		ac.Select(ConstCharPtrFromSPtr(lParam));
		break;

	case Message::AutoCGetCurrent:
		return AutoCompleteGetCurrent();

	case Message::AutoCGetCurrentText:
		return AutoCompleteGetCurrentText(CharPtrFromSPtr(lParam));

	case Message::AutoCSetCancelAtStart:
		ac.cancelAtStartPos = wParam != 0;
		break;

	case Message::AutoCGetCancelAtStart:
		return ac.cancelAtStartPos;

	case Message::AutoCSetFillUps:
		ac.SetFillUpChars(ConstCharPtrFromSPtr(lParam));
		break;

	case Message::AutoCSetChooseSingle:
		ac.chooseSingle = wParam != 0;
		break;

	case Message::AutoCGetChooseSingle:
		return ac.chooseSingle;

	case Message::AutoCSetIgnoreCase:
		ac.ignoreCase = wParam != 0;
		break;

	case Message::AutoCGetIgnoreCase:
		return ac.ignoreCase;

	case Message::AutoCSetCaseInsensitiveBehaviour:
		ac.ignoreCaseBehaviour = static_cast<CaseInsensitiveBehaviour>(wParam);
		break;

	case Message::AutoCGetCaseInsensitiveBehaviour:
		return static_cast<sptr_t>(ac.ignoreCaseBehaviour);

	case Message::AutoCSetMulti:
		multiAutoCMode = static_cast<MultiAutoComplete>(wParam);
		break;

	case Message::AutoCGetMulti:
		return static_cast<sptr_t>(multiAutoCMode);

	case Message::AutoCSetOrder:
		ac.autoSort = static_cast<Ordering>(wParam);
		break;

	case Message::AutoCGetOrder:
		return static_cast<sptr_t>(ac.autoSort);

	case Message::UserListShow:
		listType = static_cast<int>(wParam);
		AutoCompleteStart(0, ConstCharPtrFromSPtr(lParam));
		break;

	case Message::AutoCSetAutoHide:
		ac.autoHide = wParam != 0;
		break;

	case Message::AutoCGetAutoHide:
		return ac.autoHide;

	case Message::AutoCSetOptions:
		ac.options = static_cast<AutoCompleteOption>(wParam);
		break;

	case Message::AutoCGetOptions:
		return static_cast<sptr_t>(ac.options);

	case Message::AutoCSetDropRestOfWord:
		ac.dropRestOfWord = wParam != 0;
		break;

	case Message::AutoCGetDropRestOfWord:
		return ac.dropRestOfWord;

	case Message::AutoCSetMaxHeight:
		ac.lb->SetVisibleRows(static_cast<int>(wParam));
		break;

	case Message::AutoCGetMaxHeight:
		return ac.lb->GetVisibleRows();

	case Message::AutoCSetMaxWidth:
		maxListWidth = static_cast<int>(wParam);
		break;

	case Message::AutoCGetMaxWidth:
		return maxListWidth;

	case Message::AutoCSetStyle:
		vs.autocStyle = static_cast<int>(wParam);
		InvalidateStyleRedraw();
		break;

	case Message::AutoCGetStyle:
		return vs.autocStyle;

	case Message::AutoCSetImageScale:
		ac.imageScale = static_cast<float>(wParam) / 100.0f;
		break;

	case Message::AutoCGetImageScale:
		return static_cast<int>(ac.imageScale * 100);

	case Message::RegisterImage:
		ac.lb->RegisterImage(static_cast<int>(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::RegisterRGBAImage:
		ac.lb->RegisterRGBAImage(static_cast<int>(wParam), static_cast<int>(sizeRGBAImage.x), static_cast<int>(sizeRGBAImage.y),
			ConstUCharPtrFromSPtr(lParam));
		break;

	case Message::ClearRegisteredImages:
		ac.lb->ClearRegisteredImages();
		break;

	case Message::AutoCSetTypeSeparator:
		ac.SetTypesep(static_cast<char>(wParam));
		break;

	case Message::AutoCGetTypeSeparator:
		return ac.GetTypesep();

	case Message::CallTipShow:
		CallTipShow(LocationFromPosition(wParam),
			ConstCharPtrFromSPtr(lParam));
		break;

	case Message::CallTipCancel:
		ct.CallTipCancel();
		break;

	case Message::CallTipActive:
		return ct.inCallTipMode;

	case Message::CallTipPosStart:
		return ct.posStartCallTip;

	case Message::CallTipSetPosStart:
		ct.posStartCallTip = wParam;
		break;

	case Message::CallTipSetHlt:
		ct.SetHighlight(wParam, lParam);
		break;

	case Message::CallTipSetBack:
		ct.colourBG = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		vs.styles[StyleCallTip].back = ct.colourBG;
		InvalidateStyleRedraw();
		break;

	case Message::CallTipSetFore:
		ct.colourUnSel = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		vs.styles[StyleCallTip].fore = ct.colourUnSel;
		InvalidateStyleRedraw();
		break;

	case Message::CallTipSetForeHlt:
		ct.colourSel = ColourRGBA::FromIpRGB(SPtrFromUPtr(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::CallTipUseStyle:
		ct.SetTabSize(static_cast<int>(wParam));
		InvalidateStyleRedraw();
		break;

	case Message::CallTipSetPosition:
		ct.SetPosition(wParam != 0);
		InvalidateStyleRedraw();
		break;

	case Message::UsePopUp:
		displayPopupMenu = static_cast<PopUp>(wParam);
		break;

	case Message::GetLexer:
		return DocumentLexState()->GetIdentifier();

	case Message::SetILexer:
		DocumentLexState()->SetInstance(static_cast<ILexer5 *>(PtrFromSPtr(lParam)));
		return 0;

	case Message::Colourise:
		if (DocumentLexState()->UseContainerLexing()) {
			pdoc->ModifiedAt(PositionFromUPtr(wParam));
			NotifyStyleToNeeded((lParam == -1) ? pdoc->Length() : lParam);
		} else {
			DocumentLexState()->Colourise(PositionFromUPtr(wParam), lParam);
		}
		Redraw();
		break;

	case Message::SetProperty:
		DocumentLexState()->PropSet(ConstCharPtrFromUPtr(wParam),
		          ConstCharPtrFromSPtr(lParam));
		break;

	case Message::GetProperty:
		return StringResult(lParam, DocumentLexState()->PropGet(ConstCharPtrFromUPtr(wParam)));

	case Message::GetPropertyExpanded:
		return StringResult(lParam, DocumentLexState()->PropGet(ConstCharPtrFromUPtr(wParam)));

	case Message::GetPropertyInt:
		return DocumentLexState()->PropGetInt(ConstCharPtrFromUPtr(wParam), static_cast<int>(lParam));

	case Message::SetKeyWords:
		DocumentLexState()->SetWordList(static_cast<int>(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case Message::GetLexerLanguage:
		return StringResult(lParam, DocumentLexState()->GetName());

	case Message::PrivateLexerCall:
		return reinterpret_cast<sptr_t>(
			DocumentLexState()->PrivateCall(static_cast<int>(wParam), PtrFromSPtr(lParam)));

#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_GETSTYLEBITSNEEDED:
		return 8;
#endif

	case Message::PropertyNames:
		return StringResult(lParam, DocumentLexState()->PropertyNames());

	case Message::PropertyType:
		return static_cast<sptr_t>(DocumentLexState()->PropertyType(ConstCharPtrFromUPtr(wParam)));

	case Message::DescribeProperty:
		return StringResult(lParam,
				    DocumentLexState()->DescribeProperty(ConstCharPtrFromUPtr(wParam)));

	case Message::DescribeKeyWordSets:
		return StringResult(lParam, DocumentLexState()->DescribeWordListSets());

	case Message::GetLineEndTypesSupported:
		return static_cast<sptr_t>(DocumentLexState()->LineEndTypesSupported());

	case Message::AllocateSubStyles:
		return DocumentLexState()->AllocateSubStyles(static_cast<int>(wParam), static_cast<int>(lParam));

	case Message::GetSubStylesStart:
		return DocumentLexState()->SubStylesStart(static_cast<int>(wParam));

	case Message::GetSubStylesLength:
		return DocumentLexState()->SubStylesLength(static_cast<int>(wParam));

	case Message::GetStyleFromSubStyle:
		return DocumentLexState()->StyleFromSubStyle(static_cast<int>(wParam));

	case Message::GetPrimaryStyleFromStyle:
		return DocumentLexState()->PrimaryStyleFromStyle(static_cast<int>(wParam));

	case Message::FreeSubStyles:
		DocumentLexState()->FreeSubStyles();
		break;

	case Message::SetIdentifiers:
		DocumentLexState()->SetIdentifiers(static_cast<int>(wParam),
						   ConstCharPtrFromSPtr(lParam));
		break;

	case Message::DistanceToSecondaryStyles:
		return DocumentLexState()->DistanceToSecondaryStyles();

	case Message::GetSubStyleBases:
		return StringResult(lParam, DocumentLexState()->GetSubStyleBases());

	case Message::GetNamedStyles:
		return DocumentLexState()->NamedStyles();

	case Message::NameOfStyle:
		return StringResult(lParam, DocumentLexState()->
				    NameOfStyle(static_cast<int>(wParam)));

	case Message::TagsOfStyle:
		return StringResult(lParam, DocumentLexState()->
				    TagsOfStyle(static_cast<int>(wParam)));

	case Message::DescriptionOfStyle:
		return StringResult(lParam, DocumentLexState()->
				    DescriptionOfStyle(static_cast<int>(wParam)));

	default:
		return Editor::WndProc(iMessage, wParam, lParam);
	}
	return 0;
}
