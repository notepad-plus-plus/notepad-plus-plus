// Scintilla source code edit control
/** @file ScintillaBase.cxx
 ** An enhanced subclass of Editor with calltips, autocomplete and context menu.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "SciLexer.h"

#include "PropSetSimple.h"
#include "CharacterCategory.h"

#include "LexerModule.h"
#include "Catalogue.h"

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

#include "ExternalLexer.h"

using namespace Scintilla;

ScintillaBase::ScintillaBase() {
	displayPopupMenu = SC_POPUP_ALL;
	listType = 0;
	maxListWidth = 0;
	multiAutoCMode = SC_MULTIAUTOC_ONCE;
#ifdef SCI_LEXER
	Scintilla_LinkLexers();
#endif
}

ScintillaBase::~ScintillaBase() {
}

void ScintillaBase::Finalise() {
	Editor::Finalise();
	popup.Destroy();
}

void ScintillaBase::AddCharUTF(const char *s, unsigned int len, bool /*treatAsDBCS*/) {
	InsertCharacter(std::string_view(s, len), CharacterSource::directInput);
}

void ScintillaBase::InsertCharacter(std::string_view sv, CharacterSource charSource) {
	const bool isFillUp = ac.Active() && ac.IsFillUpChar(sv[0]);
	if (!isFillUp) {
		Editor::InsertCharacter(sv, charSource);
	}
	if (ac.Active()) {
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
		WndProc(SCI_UNDO, 0, 0);
		break;

	case idcmdRedo:
		WndProc(SCI_REDO, 0, 0);
		break;

	case idcmdCut:
		WndProc(SCI_CUT, 0, 0);
		break;

	case idcmdCopy:
		WndProc(SCI_COPY, 0, 0);
		break;

	case idcmdPaste:
		WndProc(SCI_PASTE, 0, 0);
		break;

	case idcmdDelete:
		WndProc(SCI_CLEAR, 0, 0);
		break;

	case idcmdSelectAll:
		WndProc(SCI_SELECTALL, 0, 0);
		break;
	}
}

int ScintillaBase::KeyCommand(unsigned int iMessage) {
	// Most key commands cancel autocompletion mode
	if (ac.Active()) {
		switch (iMessage) {
			// Except for these
		case SCI_LINEDOWN:
			AutoCompleteMove(1);
			return 0;
		case SCI_LINEUP:
			AutoCompleteMove(-1);
			return 0;
		case SCI_PAGEDOWN:
			AutoCompleteMove(ac.lb->GetVisibleRows());
			return 0;
		case SCI_PAGEUP:
			AutoCompleteMove(-ac.lb->GetVisibleRows());
			return 0;
		case SCI_VCHOME:
			AutoCompleteMove(-5000);
			return 0;
		case SCI_LINEEND:
			AutoCompleteMove(5000);
			return 0;
		case SCI_DELETEBACK:
			DelCharBack(true);
			AutoCompleteCharacterDeleted();
			EnsureCaretVisible();
			return 0;
		case SCI_DELETEBACKNOTLINE:
			DelCharBack(false);
			AutoCompleteCharacterDeleted();
			EnsureCaretVisible();
			return 0;
		case SCI_TAB:
			AutoCompleteCompleted(0, SC_AC_TAB);
			return 0;
		case SCI_NEWLINE:
			AutoCompleteCompleted(0, SC_AC_NEWLINE);
			return 0;

		default:
			AutoCompleteCancel();
		}
	}

	if (ct.inCallTipMode) {
		if (
		    (iMessage != SCI_CHARLEFT) &&
		    (iMessage != SCI_CHARLEFTEXTEND) &&
		    (iMessage != SCI_CHARRIGHT) &&
		    (iMessage != SCI_CHARRIGHTEXTEND) &&
		    (iMessage != SCI_EDITTOGGLEOVERTYPE) &&
		    (iMessage != SCI_DELETEBACK) &&
		    (iMessage != SCI_DELETEBACKNOTLINE)
		) {
			ct.CallTipCancel();
		}
		if ((iMessage == SCI_DELETEBACK) || (iMessage == SCI_DELETEBACKNOTLINE)) {
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
		AutoCompleteCompleted(0, SC_AC_DOUBLECLICK);
		break;
	}
}

void ScintillaBase::AutoCompleteInsert(Sci::Position startPos, Sci::Position removeLen, const char *text, Sci::Position textLen) {
	UndoGroup ug(pdoc);
	if (multiAutoCMode == SC_MULTIAUTOC_ONCE) {
		pdoc->DeleteChars(startPos, removeLen);
		const Sci::Position lengthInserted = pdoc->InsertString(startPos, text, textLen);
		SetEmptySelection(startPos + lengthInserted);
	} else {
		// SC_MULTIAUTOC_EACH
		for (size_t r=0; r<sel.Count(); r++) {
			if (!RangeContainsProtected(sel.Range(r).Start().Position(),
				sel.Range(r).End().Position())) {
				Sci::Position positionInsert = sel.Range(r).Start().Position();
				positionInsert = RealizeVirtualSpace(positionInsert, sel.Range(r).caret.VirtualSpace());
				if (positionInsert - removeLen >= 0) {
					positionInsert -= removeLen;
					pdoc->DeleteChars(positionInsert, removeLen);
				}
				const Sci::Position lengthInserted = pdoc->InsertString(positionInsert, text, textLen);
				if (lengthInserted > 0) {
					sel.Range(r).caret.SetPosition(positionInsert + lengthInserted);
					sel.Range(r).anchor.SetPosition(positionInsert + lengthInserted);
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
			const char *typeSep = strchr(list, ac.GetTypesep());
			const Sci::Position lenInsert = typeSep ?
				(typeSep-list) : strlen(list);
			if (ac.ignoreCase) {
				// May need to convert the case before invocation, so remove lenEntered characters
				AutoCompleteInsert(sel.MainCaret() - lenEntered, lenEntered, list, lenInsert);
			} else {
				AutoCompleteInsert(sel.MainCaret(), 0, list + lenEntered, lenInsert - lenEntered);
			}
			ac.Cancel();
			return;
		}
	}
	ac.Start(wMain, idAutoComplete, sel.MainCaret(), PointMainCaret(),
				lenEntered, vs.lineHeight, IsUnicodeMode(), technology);

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
	ac.lb->SetFont(vs.styles[STYLE_DEFAULT].font);
	const unsigned int aveCharWidth = static_cast<unsigned int>(vs.styles[STYLE_DEFAULT].aveCharWidth);
	ac.lb->SetAverageCharWidth(aveCharWidth);
	ac.lb->SetDelegate(this);

	ac.SetList(list ? list : "");

	// Fiddle the position of the list so it is right next to the target and wide enough for all its strings
	PRectangle rcList = ac.lb->GetDesiredRect();
	const int heightAlloced = static_cast<int>(rcList.bottom - rcList.top);
	widthLB = std::max(widthLB, static_cast<int>(rcList.right - rcList.left));
	if (maxListWidth != 0)
		widthLB = std::min(widthLB, static_cast<int>(aveCharWidth)*maxListWidth);
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
		SCNotification scn = {};
		scn.nmhdr.code = SCN_AUTOCCANCELLED;
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
	std::string wordCurrent = RangeText(ac.posStart - ac.startLen, sel.MainCaret());
	ac.Select(wordCurrent.c_str());
}

void ScintillaBase::AutoCompleteSelection() {
	const int item = ac.GetSelection();
	std::string selected;
	if (item != -1) {
		selected = ac.GetValue(item);
	}

	SCNotification scn = {};
	scn.nmhdr.code = SCN_AUTOCSELECTIONCHANGE;
	scn.message = 0;
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
		AutoCompleteCompleted(ch, SC_AC_FILLUP);
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
	SCNotification scn = {};
	scn.nmhdr.code = SCN_AUTOCCHARDELETED;
	scn.wParam = 0;
	scn.listType = 0;
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteCompleted(char ch, unsigned int completionMethod) {
	const int item = ac.GetSelection();
	if (item == -1) {
		AutoCompleteCancel();
		return;
	}
	const std::string selected = ac.GetValue(item);

	ac.Show(false);

	SCNotification scn = {};
	scn.nmhdr.code = listType > 0 ? SCN_USERLISTSELECTION : SCN_AUTOCSELECTION;
	scn.message = 0;
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
	AutoCompleteInsert(firstPos, endPos - firstPos, selected.c_str(), selected.length());
	SetLastXChosen();

	scn.nmhdr.code = SCN_AUTOCCOMPLETED;
	NotifyParent(scn);

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
	// If container knows about STYLE_CALLTIP then use it in place of the
	// STYLE_DEFAULT for the face name, size and character set. Also use it
	// for the foreground and background colour.
	const int ctStyle = ct.UseStyleCallTip() ? STYLE_CALLTIP : STYLE_DEFAULT;
	if (ct.UseStyleCallTip()) {
		ct.SetForeBack(vs.styles[STYLE_CALLTIP].fore, vs.styles[STYLE_CALLTIP].back);
	}
	if (wMargin.Created()) {
		pt = pt + GetVisibleOriginInMain();
	}
	PRectangle rc = ct.CallTipStart(sel.MainCaret(), pt,
		vs.lineHeight,
		defn,
		vs.styles[ctStyle].fontName,
		vs.styles[ctStyle].sizeZoomed,
		CodePage(),
		vs.styles[ctStyle].characterSet,
		vs.technology,
		wMain);
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
}

void ScintillaBase::CallTipClick() {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_CALLTIPCLICK;
	scn.position = ct.clickPlace;
	NotifyParent(scn);
}

bool ScintillaBase::ShouldDisplayPopup(Point ptInWindowCoordinates) const {
	return (displayPopupMenu == SC_POPUP_ALL ||
		(displayPopupMenu == SC_POPUP_TEXT && !PointInSelMargin(ptInWindowCoordinates)));
}

void ScintillaBase::ContextMenu(Point pt) {
	if (displayPopupMenu) {
		const bool writable = !WndProc(SCI_GETREADONLY, 0, 0);
		popup.CreatePopUp();
		AddToPopUp("Undo", idcmdUndo, writable && pdoc->CanUndo());
		AddToPopUp("Redo", idcmdRedo, writable && pdoc->CanRedo());
		AddToPopUp("");
		AddToPopUp("Cut", idcmdCut, writable && !sel.Empty());
		AddToPopUp("Copy", idcmdCopy, !sel.Empty());
		AddToPopUp("Paste", idcmdPaste, writable && WndProc(SCI_CANPASTE, 0, 0));
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

void ScintillaBase::ButtonDownWithModifiers(Point pt, unsigned int curTime, int modifiers) {
	CancelModes();
	Editor::ButtonDownWithModifiers(pt, curTime, modifiers);
}

void ScintillaBase::RightButtonDownWithModifiers(Point pt, unsigned int curTime, int modifiers) {
	CancelModes();
	Editor::RightButtonDownWithModifiers(pt, curTime, modifiers);
}

namespace Scintilla {

class LexState : public LexInterface {
	const LexerModule *lexCurrent;
	void SetLexerModule(const LexerModule *lex);
	PropSetSimple props;
	int interfaceVersion;
public:
	int lexLanguage;

	explicit LexState(Document *pdoc_);
	void SetInstance(ILexer5 *instance_);
	// Deleted so LexState objects can not be copied.
	LexState(const LexState &) = delete;
	LexState(LexState &&) = delete;
	LexState &operator=(const LexState &) = delete;
	LexState &operator=(LexState &&) = delete;
	~LexState() override;
	void SetLexer(uptr_t wParam);
	void SetLexerLanguage(const char *languageName);

	const char *DescribeWordListSets();
	void SetWordList(int n, const char *wl);
	int GetIdentifier() const;
	const char *GetName() const;
	void *PrivateCall(int operation, void *pointer);
	const char *PropertyNames();
	int PropertyType(const char *name);
	const char *DescribeProperty(const char *name);
	void PropSet(const char *key, const char *val);
	const char *PropGet(const char *key) const;
	int PropGetInt(const char *key, int defaultValue=0) const;
	size_t PropGetExpanded(const char *key, char *result) const;

	int LineEndTypesSupported() override;
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

LexState::LexState(Document *pdoc_) : LexInterface(pdoc_) {
	lexCurrent = nullptr;
	performingStyle = false;
	interfaceVersion = lvRelease4;
	lexLanguage = SCLEX_CONTAINER;
}

LexState::~LexState() {
	if (instance) {
		instance->Release();
		instance = nullptr;
	}
}

void LexState::SetInstance(ILexer5 *instance_) {
	if (instance) {
		instance->Release();
		instance = nullptr;
	}
	instance = instance_;
	pdoc->LexerChanged();
}

LexState *ScintillaBase::DocumentLexState() {
	if (!pdoc->GetLexInterface()) {
		pdoc->SetLexInterface(std::make_unique<LexState>(pdoc));
	}
	return dynamic_cast<LexState *>(pdoc->GetLexInterface());
}

void LexState::SetLexerModule(const LexerModule *lex) {
	if (lex != lexCurrent) {
		if (instance) {
			instance->Release();
			instance = nullptr;
		}
		interfaceVersion = lvRelease4;
		lexCurrent = lex;
		if (lexCurrent) {
			instance = lexCurrent->Create();
			interfaceVersion = instance->Version();
		}
		pdoc->LexerChanged();
	}
}

void LexState::SetLexer(uptr_t wParam) {
	lexLanguage = static_cast<int>(wParam);
	if (lexLanguage == SCLEX_CONTAINER) {
		SetLexerModule(nullptr);
	} else {
		const LexerModule *lex = Catalogue::Find(lexLanguage);
		if (!lex)
			lex = Catalogue::Find(SCLEX_NULL);
		SetLexerModule(lex);
	}
}

void LexState::SetLexerLanguage(const char *languageName) {
	const LexerModule *lex = Catalogue::Find(languageName);
	if (!lex)
		lex = Catalogue::Find(SCLEX_NULL);
	if (lex)
		lexLanguage = lex->GetLanguage();
	SetLexerModule(lex);
}

const char *LexState::DescribeWordListSets() {
	if (instance) {
		return instance->DescribeWordListSets();
	} else {
		return nullptr;
	}
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
	if (lexCurrent) {
		return lexCurrent->GetLanguage();
	}
	if (instance) {
		if (instance->Version() >= lvRelease5) {
			return instance->GetIdentifier();
		}
	}
	return SCLEX_CONTAINER;
}

const char *LexState::GetName() const {
	if (lexCurrent) {
		return lexCurrent->languageName;
	}
	if (instance) {
		if (instance->Version() >= lvRelease5) {
			return instance->GetName();
		}
	}
	return "";
}

void *LexState::PrivateCall(int operation, void *pointer) {
	if (pdoc && instance) {
		return instance->PrivateCall(operation, pointer);
	} else {
		return nullptr;
	}
}

const char *LexState::PropertyNames() {
	if (instance) {
		return instance->PropertyNames();
	} else {
		return nullptr;
	}
}

int LexState::PropertyType(const char *name) {
	if (instance) {
		return instance->PropertyType(name);
	} else {
		return SC_TYPE_BOOLEAN;
	}
}

const char *LexState::DescribeProperty(const char *name) {
	if (instance) {
		return instance->DescribeProperty(name);
	} else {
		return nullptr;
	}
}

void LexState::PropSet(const char *key, const char *val) {
	props.Set(key, val, strlen(key), strlen(val));
	if (instance) {
		const Sci_Position firstModification = instance->PropertySet(key, val);
		if (firstModification >= 0) {
			pdoc->ModifiedAt(firstModification);
		}
	}
}

const char *LexState::PropGet(const char *key) const {
	return props.Get(key);
}

int LexState::PropGetInt(const char *key, int defaultValue) const {
	return props.GetInt(key, defaultValue);
}

size_t LexState::PropGetExpanded(const char *key, char *result) const {
	return props.GetExpanded(key, result);
}

int LexState::LineEndTypesSupported() {
	if (instance) {
		return instance->LineEndTypesSupported();
	}
	return 0;
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
	} else {
		return -1;
	}
}

const char *LexState::NameOfStyle(int style) {
	if (instance) {
		return instance->NameOfStyle(style);
	} else {
		return nullptr;
	}
}

const char *LexState::TagsOfStyle(int style) {
	if (instance) {
		return instance->TagsOfStyle(style);
	} else {
		return nullptr;
	}
}

const char *LexState::DescriptionOfStyle(int style) {
	if (instance) {
		return instance->DescriptionOfStyle(style);
	} else {
		return nullptr;
	}
}

void ScintillaBase::NotifyStyleToNeeded(Sci::Position endStyleNeeded) {
	if (DocumentLexState()->GetIdentifier() != SCLEX_CONTAINER) {
		const Sci::Line lineEndStyled =
			pdoc->SciLineFromPosition(pdoc->GetEndStyled());
		const Sci::Position endStyled =
			pdoc->LineStart(lineEndStyled);
		DocumentLexState()->Colourise(endStyled, endStyleNeeded);
		return;
	}
	Editor::NotifyStyleToNeeded(endStyleNeeded);
}

void ScintillaBase::NotifyLexerChanged(Document *, void *) {
	vs.EnsureStyle(0xff);
}

sptr_t ScintillaBase::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case SCI_AUTOCSHOW:
		listType = 0;
		AutoCompleteStart(static_cast<Sci::Position>(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_AUTOCCANCEL:
		ac.Cancel();
		break;

	case SCI_AUTOCACTIVE:
		return ac.Active();

	case SCI_AUTOCPOSSTART:
		return ac.posStart;

	case SCI_AUTOCCOMPLETE:
		AutoCompleteCompleted(0, SC_AC_COMMAND);
		break;

	case SCI_AUTOCSETSEPARATOR:
		ac.SetSeparator(static_cast<char>(wParam));
		break;

	case SCI_AUTOCGETSEPARATOR:
		return ac.GetSeparator();

	case SCI_AUTOCSTOPS:
		ac.SetStopChars(ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_AUTOCSELECT:
		ac.Select(ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_AUTOCGETCURRENT:
		return AutoCompleteGetCurrent();

	case SCI_AUTOCGETCURRENTTEXT:
		return AutoCompleteGetCurrentText(CharPtrFromSPtr(lParam));

	case SCI_AUTOCSETCANCELATSTART:
		ac.cancelAtStartPos = wParam != 0;
		break;

	case SCI_AUTOCGETCANCELATSTART:
		return ac.cancelAtStartPos;

	case SCI_AUTOCSETFILLUPS:
		ac.SetFillUpChars(ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_AUTOCSETCHOOSESINGLE:
		ac.chooseSingle = wParam != 0;
		break;

	case SCI_AUTOCGETCHOOSESINGLE:
		return ac.chooseSingle;

	case SCI_AUTOCSETIGNORECASE:
		ac.ignoreCase = wParam != 0;
		break;

	case SCI_AUTOCGETIGNORECASE:
		return ac.ignoreCase;

	case SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR:
		ac.ignoreCaseBehaviour = static_cast<unsigned int>(wParam);
		break;

	case SCI_AUTOCGETCASEINSENSITIVEBEHAVIOUR:
		return ac.ignoreCaseBehaviour;

	case SCI_AUTOCSETMULTI:
		multiAutoCMode = static_cast<int>(wParam);
		break;

	case SCI_AUTOCGETMULTI:
		return multiAutoCMode;

	case SCI_AUTOCSETORDER:
		ac.autoSort = static_cast<int>(wParam);
		break;

	case SCI_AUTOCGETORDER:
		return ac.autoSort;

	case SCI_USERLISTSHOW:
		listType = static_cast<int>(wParam);
		AutoCompleteStart(0, ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_AUTOCSETAUTOHIDE:
		ac.autoHide = wParam != 0;
		break;

	case SCI_AUTOCGETAUTOHIDE:
		return ac.autoHide;

	case SCI_AUTOCSETDROPRESTOFWORD:
		ac.dropRestOfWord = wParam != 0;
		break;

	case SCI_AUTOCGETDROPRESTOFWORD:
		return ac.dropRestOfWord;

	case SCI_AUTOCSETMAXHEIGHT:
		ac.lb->SetVisibleRows(static_cast<int>(wParam));
		break;

	case SCI_AUTOCGETMAXHEIGHT:
		return ac.lb->GetVisibleRows();

	case SCI_AUTOCSETMAXWIDTH:
		maxListWidth = static_cast<int>(wParam);
		break;

	case SCI_AUTOCGETMAXWIDTH:
		return maxListWidth;

	case SCI_REGISTERIMAGE:
		ac.lb->RegisterImage(static_cast<int>(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_REGISTERRGBAIMAGE:
		ac.lb->RegisterRGBAImage(static_cast<int>(wParam), static_cast<int>(sizeRGBAImage.x), static_cast<int>(sizeRGBAImage.y),
			ConstUCharPtrFromSPtr(lParam));
		break;

	case SCI_CLEARREGISTEREDIMAGES:
		ac.lb->ClearRegisteredImages();
		break;

	case SCI_AUTOCSETTYPESEPARATOR:
		ac.SetTypesep(static_cast<char>(wParam));
		break;

	case SCI_AUTOCGETTYPESEPARATOR:
		return ac.GetTypesep();

	case SCI_CALLTIPSHOW:
		CallTipShow(LocationFromPosition(wParam),
			ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_CALLTIPCANCEL:
		ct.CallTipCancel();
		break;

	case SCI_CALLTIPACTIVE:
		return ct.inCallTipMode;

	case SCI_CALLTIPPOSSTART:
		return ct.posStartCallTip;

	case SCI_CALLTIPSETPOSSTART:
		ct.posStartCallTip = wParam;
		break;

	case SCI_CALLTIPSETHLT:
		ct.SetHighlight(wParam, lParam);
		break;

	case SCI_CALLTIPSETBACK:
		ct.colourBG = ColourDesired(static_cast<int>(wParam));
		vs.styles[STYLE_CALLTIP].back = ct.colourBG;
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETFORE:
		ct.colourUnSel = ColourDesired(static_cast<int>(wParam));
		vs.styles[STYLE_CALLTIP].fore = ct.colourUnSel;
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETFOREHLT:
		ct.colourSel = ColourDesired(static_cast<int>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPUSESTYLE:
		ct.SetTabSize(static_cast<int>(wParam));
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETPOSITION:
		ct.SetPosition(wParam != 0);
		InvalidateStyleRedraw();
		break;

	case SCI_USEPOPUP:
		displayPopupMenu = static_cast<int>(wParam);
		break;

	case SCI_SETLEXER:
		DocumentLexState()->SetLexer(static_cast<int>(wParam));
		break;

	case SCI_GETLEXER:
		return DocumentLexState()->GetIdentifier();

	case SCI_SETILEXER:
		DocumentLexState()->SetInstance(reinterpret_cast<ILexer5 *>(lParam));
		return 0;

	case SCI_COLOURISE:
		if (DocumentLexState()->lexLanguage == SCLEX_CONTAINER) {
			pdoc->ModifiedAt(static_cast<Sci::Position>(wParam));
			NotifyStyleToNeeded((lParam == -1) ? pdoc->Length() : lParam);
		} else {
			DocumentLexState()->Colourise(static_cast<Sci::Position>(wParam), lParam);
		}
		Redraw();
		break;

	case SCI_SETPROPERTY:
		DocumentLexState()->PropSet(ConstCharPtrFromUPtr(wParam),
		          ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_GETPROPERTY:
		return StringResult(lParam, DocumentLexState()->PropGet(ConstCharPtrFromUPtr(wParam)));

	case SCI_GETPROPERTYEXPANDED:
		return DocumentLexState()->PropGetExpanded(ConstCharPtrFromUPtr(wParam),
			CharPtrFromSPtr(lParam));

	case SCI_GETPROPERTYINT:
		return DocumentLexState()->PropGetInt(ConstCharPtrFromUPtr(wParam), static_cast<int>(lParam));

	case SCI_SETKEYWORDS:
		DocumentLexState()->SetWordList(static_cast<int>(wParam), ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_SETLEXERLANGUAGE:
		DocumentLexState()->SetLexerLanguage(ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_GETLEXERLANGUAGE:
		return StringResult(lParam, DocumentLexState()->GetName());

#ifdef SCI_LEXER
	case SCI_LOADLEXERLIBRARY:
		ExternalLexerLoad(ConstCharPtrFromSPtr(lParam));
		break;
#endif

	case SCI_PRIVATELEXERCALL:
		return reinterpret_cast<sptr_t>(
			DocumentLexState()->PrivateCall(static_cast<int>(wParam), reinterpret_cast<void *>(lParam)));

#ifdef INCLUDE_DEPRECATED_FEATURES
	case SCI_GETSTYLEBITSNEEDED:
		return 8;
#endif

	case SCI_PROPERTYNAMES:
		return StringResult(lParam, DocumentLexState()->PropertyNames());

	case SCI_PROPERTYTYPE:
		return DocumentLexState()->PropertyType(ConstCharPtrFromUPtr(wParam));

	case SCI_DESCRIBEPROPERTY:
		return StringResult(lParam,
				    DocumentLexState()->DescribeProperty(ConstCharPtrFromUPtr(wParam)));

	case SCI_DESCRIBEKEYWORDSETS:
		return StringResult(lParam, DocumentLexState()->DescribeWordListSets());

	case SCI_GETLINEENDTYPESSUPPORTED:
		return DocumentLexState()->LineEndTypesSupported();

	case SCI_ALLOCATESUBSTYLES:
		return DocumentLexState()->AllocateSubStyles(static_cast<int>(wParam), static_cast<int>(lParam));

	case SCI_GETSUBSTYLESSTART:
		return DocumentLexState()->SubStylesStart(static_cast<int>(wParam));

	case SCI_GETSUBSTYLESLENGTH:
		return DocumentLexState()->SubStylesLength(static_cast<int>(wParam));

	case SCI_GETSTYLEFROMSUBSTYLE:
		return DocumentLexState()->StyleFromSubStyle(static_cast<int>(wParam));

	case SCI_GETPRIMARYSTYLEFROMSTYLE:
		return DocumentLexState()->PrimaryStyleFromStyle(static_cast<int>(wParam));

	case SCI_FREESUBSTYLES:
		DocumentLexState()->FreeSubStyles();
		break;

	case SCI_SETIDENTIFIERS:
		DocumentLexState()->SetIdentifiers(static_cast<int>(wParam),
						   ConstCharPtrFromSPtr(lParam));
		break;

	case SCI_DISTANCETOSECONDARYSTYLES:
		return DocumentLexState()->DistanceToSecondaryStyles();

	case SCI_GETSUBSTYLEBASES:
		return StringResult(lParam, DocumentLexState()->GetSubStyleBases());

	case SCI_GETNAMEDSTYLES:
		return DocumentLexState()->NamedStyles();

	case SCI_NAMEOFSTYLE:
		return StringResult(lParam, DocumentLexState()->
				    NameOfStyle(static_cast<int>(wParam)));

	case SCI_TAGSOFSTYLE:
		return StringResult(lParam, DocumentLexState()->
				    TagsOfStyle(static_cast<int>(wParam)));

	case SCI_DESCRIPTIONOFSTYLE:
		return StringResult(lParam, DocumentLexState()->
				    DescriptionOfStyle(static_cast<int>(wParam)));

	default:
		return Editor::WndProc(iMessage, wParam, lParam);
	}
	return 0;
}
