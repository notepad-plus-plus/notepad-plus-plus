// Scintilla source code edit control
/** @file ScintillaBase.cxx
 ** An enhanced subclass of Editor with calltips, autocomplete and context menu.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <string>
#include <vector>

#include "Platform.h"

#include "Scintilla.h"
#include "PropSet.h"
#include "PropSetSimple.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#include "Accessor.h"
#include "DocumentAccessor.h"
#include "KeyWords.h"
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
#include "AutoComplete.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "ScintillaBase.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

ScintillaBase::ScintillaBase() {
	displayPopupMenu = true;
	listType = 0;
	maxListWidth = 0;
#ifdef SCI_LEXER
	lexLanguage = SCLEX_CONTAINER;
	performingStyle = false;
	lexCurrent = 0;
	for (int wl = 0; wl < numWordLists; wl++)
		keyWordLists[wl] = new WordList;
	keyWordLists[numWordLists] = 0;
#endif
}

ScintillaBase::~ScintillaBase() {
#ifdef SCI_LEXER
	for (int wl = 0; wl < numWordLists; wl++)
		delete keyWordLists[wl];
#endif
}

void ScintillaBase::Finalise() {
	Editor::Finalise();
	popup.Destroy();
}

void ScintillaBase::RefreshColourPalette(Palette &pal, bool want) {
	Editor::RefreshColourPalette(pal, want);
	ct.RefreshColourPalette(pal, want);
}

void ScintillaBase::AddCharUTF(char *s, unsigned int len, bool treatAsDBCS) {
	bool isFillUp = ac.Active() && ac.IsFillUpChar(*s);
	if (!isFillUp) {
		Editor::AddCharUTF(s, len, treatAsDBCS);
	}
	if (ac.Active()) {
		AutoCompleteCharacterAdded(s[0]);
		// For fill ups add the character after the autocompletion has
		// triggered so containers see the key so can display a calltip.
		if (isFillUp) {
			Editor::AddCharUTF(s, len, treatAsDBCS);
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
			AutoCompleteMove( -1);
			return 0;
		case SCI_PAGEDOWN:
			AutoCompleteMove(5);
			return 0;
		case SCI_PAGEUP:
			AutoCompleteMove( -5);
			return 0;
		case SCI_VCHOME:
			AutoCompleteMove( -5000);
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
			AutoCompleteCompleted();
			return 0;
		case SCI_NEWLINE:
			AutoCompleteCompleted();
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

void ScintillaBase::AutoCompleteDoubleClick(void *p) {
	ScintillaBase *sci = reinterpret_cast<ScintillaBase *>(p);
	sci->AutoCompleteCompleted();
}

void ScintillaBase::AutoCompleteStart(int lenEntered, const char *list) {
	//Platform::DebugPrintf("AutoComplete %s\n", list);
	ct.CallTipCancel();

	if (ac.chooseSingle && (listType == 0)) {
		if (list && !strchr(list, ac.GetSeparator())) {
			const char *typeSep = strchr(list, ac.GetTypesep());
			size_t lenInsert = (typeSep) ? (typeSep-list) : strlen(list);
			if (ac.ignoreCase) {
				SetEmptySelection(sel.MainCaret() - lenEntered);
				pdoc->DeleteChars(sel.MainCaret(), lenEntered);
				SetEmptySelection(sel.MainCaret());
				pdoc->InsertString(sel.MainCaret(), list, lenInsert);
				SetEmptySelection(sel.MainCaret() + lenInsert);
			} else {
				SetEmptySelection(sel.MainCaret());
				pdoc->InsertString(sel.MainCaret(), list + lenEntered, lenInsert - lenEntered);
				SetEmptySelection(sel.MainCaret() + lenInsert - lenEntered);
			}
			return;
		}
	}
	ac.Start(wMain, idAutoComplete, sel.MainCaret(), PointMainCaret(),
				lenEntered, vs.lineHeight, IsUnicodeMode());

	PRectangle rcClient = GetClientRectangle();
	Point pt = LocationFromPosition(sel.MainCaret() - lenEntered);
	PRectangle rcPopupBounds = wMain.GetMonitorRect(pt);
	if (rcPopupBounds.Height() == 0)
		rcPopupBounds = rcClient;

	int heightLB = 100;
	int widthLB = 100;
	if (pt.x >= rcClient.right - widthLB) {
		HorizontalScrollTo(xOffset + pt.x - rcClient.right + widthLB);
		Redraw();
		pt = PointMainCaret();
	}
	PRectangle rcac;
	rcac.left = pt.x - ac.lb->CaretFromEdge();
	if (pt.y >= rcPopupBounds.bottom - heightLB &&  // Wont fit below.
	        pt.y >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2) { // and there is more room above.
		rcac.top = pt.y - heightLB;
		if (rcac.top < rcPopupBounds.top) {
			heightLB -= (rcPopupBounds.top - rcac.top);
			rcac.top = rcPopupBounds.top;
		}
	} else {
		rcac.top = pt.y + vs.lineHeight;
	}
	rcac.right = rcac.left + widthLB;
	rcac.bottom = Platform::Minimum(rcac.top + heightLB, rcPopupBounds.bottom);
	ac.lb->SetPositionRelative(rcac, wMain);
	ac.lb->SetFont(vs.styles[STYLE_DEFAULT].font);
	unsigned int aveCharWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
	ac.lb->SetAverageCharWidth(aveCharWidth);
	ac.lb->SetDoubleClickAction(AutoCompleteDoubleClick, this);

	ac.SetList(list);

	// Fiddle the position of the list so it is right next to the target and wide enough for all its strings
	PRectangle rcList = ac.lb->GetDesiredRect();
	int heightAlloced = rcList.bottom - rcList.top;
	widthLB = Platform::Maximum(widthLB, rcList.right - rcList.left);
	if (maxListWidth != 0)
		widthLB = Platform::Minimum(widthLB, aveCharWidth*maxListWidth);
	// Make an allowance for large strings in list
	rcList.left = pt.x - ac.lb->CaretFromEdge();
	rcList.right = rcList.left + widthLB;
	if (((pt.y + vs.lineHeight) >= (rcPopupBounds.bottom - heightAlloced)) &&  // Wont fit below.
	        ((pt.y + vs.lineHeight / 2) >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2)) { // and there is more room above.
		rcList.top = pt.y - heightAlloced;
	} else {
		rcList.top = pt.y + vs.lineHeight;
	}
	rcList.bottom = rcList.top + heightAlloced;
	ac.lb->SetPositionRelative(rcList, wMain);
	ac.Show(true);
	if (lenEntered != 0) {
		AutoCompleteMoveToCurrentWord();
	}
}

void ScintillaBase::AutoCompleteCancel() {
	if (ac.Active()) {
		SCNotification scn = {0};
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
	char wordCurrent[1000];
	int i;
	int startWord = ac.posStart - ac.startLen;
	for (i = startWord; i < sel.MainCaret() && i - startWord < 1000; i++)
		wordCurrent[i - startWord] = pdoc->CharAt(i);
	wordCurrent[Platform::Minimum(i - startWord, 999)] = '\0';
	ac.Select(wordCurrent);
}

void ScintillaBase::AutoCompleteCharacterAdded(char ch) {
	if (ac.IsFillUpChar(ch)) {
		AutoCompleteCompleted();
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
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_AUTOCCHARDELETED;
	scn.wParam = 0;
	scn.listType = 0;
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteCompleted() {
	int item = ac.lb->GetSelection();
	char selected[1000];
	selected[0] = '\0';
	if (item != -1) {
		ac.lb->GetValue(item, selected, sizeof(selected));
	} else {
		AutoCompleteCancel();
		return;
	}

	ac.Show(false);

	SCNotification scn = {0};
	scn.nmhdr.code = listType > 0 ? SCN_USERLISTSELECTION : SCN_AUTOCSELECTION;
	scn.message = 0;
	scn.wParam = listType;
	scn.listType = listType;
	Position firstPos = ac.posStart - ac.startLen;
	scn.lParam = firstPos;
	scn.text = selected;
	NotifyParent(scn);

	if (!ac.Active())
		return;
	ac.Cancel();

	if (listType > 0)
		return;

	Position endPos = sel.MainCaret();
	if (ac.dropRestOfWord)
		endPos = pdoc->ExtendWordSelect(endPos, 1, true);
	if (endPos < firstPos)
		return;
	UndoGroup ug(pdoc);
	if (endPos != firstPos) {
		pdoc->DeleteChars(firstPos, endPos - firstPos);
	}
	SetEmptySelection(ac.posStart);
	if (item != -1) {
		pdoc->InsertCString(firstPos, selected);
		SetEmptySelection(firstPos + static_cast<int>(strlen(selected)));
	}
}

int ScintillaBase::AutoCompleteGetCurrent() {
	if (!ac.Active())
		return -1;
	return ac.lb->GetSelection();
}

int ScintillaBase::AutoCompleteGetCurrentText(char *buffer) {
	if (ac.Active()) {
		int item = ac.lb->GetSelection();
		char selected[1000];
		selected[0] = '\0';
		if (item != -1) {
			ac.lb->GetValue(item, selected, sizeof(selected));
			if (buffer != NULL)
				strcpy(buffer, selected);
			return strlen(selected);
		}
	}
	if (buffer != NULL)
		*buffer = '\0';
	return 0;
}

void ScintillaBase::CallTipShow(Point pt, const char *defn) {
	ac.Cancel();
	pt.y += vs.lineHeight;
	// If container knows about STYLE_CALLTIP then use it in place of the
	// STYLE_DEFAULT for the face name, size and character set. Also use it
	// for the foreground and background colour.
	int ctStyle = ct.UseStyleCallTip() ? STYLE_CALLTIP : STYLE_DEFAULT;
	if (ct.UseStyleCallTip()) {
		ct.SetForeBack(vs.styles[STYLE_CALLTIP].fore, vs.styles[STYLE_CALLTIP].back);
	}
	PRectangle rc = ct.CallTipStart(sel.MainCaret(), pt,
		defn,
		vs.styles[ctStyle].fontName,
		vs.styles[ctStyle].sizeZoomed,
		CodePage(),
		vs.styles[ctStyle].characterSet,
		wMain);
	// If the call-tip window would be out of the client
	// space, adjust so it displays above the text.
	PRectangle rcClient = GetClientRectangle();
	if (rc.bottom > rcClient.bottom) {
		int offset = vs.lineHeight + rc.Height();
		rc.top -= offset;
		rc.bottom -= offset;
	}
	// Now display the window.
	CreateCallTipWindow(rc);
	ct.wCallTip.SetPositionRelative(rc, wMain);
	ct.wCallTip.Show();
}

void ScintillaBase::CallTipClick() {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_CALLTIPCLICK;
	scn.position = ct.clickPlace;
	NotifyParent(scn);
}

void ScintillaBase::ContextMenu(Point pt) {
	if (displayPopupMenu) {
		bool writable = !WndProc(SCI_GETREADONLY, 0, 0);
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

void ScintillaBase::ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt) {
	CancelModes();
	Editor::ButtonDown(pt, curTime, shift, ctrl, alt);
}

#ifdef SCI_LEXER
void ScintillaBase::SetLexer(uptr_t wParam) {
	lexLanguage = wParam;
	lexCurrent = LexerModule::Find(lexLanguage);
	if (!lexCurrent)
		lexCurrent = LexerModule::Find(SCLEX_NULL);
	int bits = lexCurrent ? lexCurrent->GetStyleBitsNeeded() : 5;
	vs.EnsureStyle((1 << bits) - 1);
}

void ScintillaBase::SetLexerLanguage(const char *languageName) {
	lexLanguage = SCLEX_CONTAINER;
	lexCurrent = LexerModule::Find(languageName);
	if (!lexCurrent)
		lexCurrent = LexerModule::Find(SCLEX_NULL);
	if (lexCurrent)
		lexLanguage = lexCurrent->GetLanguage();
	int bits = lexCurrent ? lexCurrent->GetStyleBitsNeeded() : 5;
	vs.EnsureStyle((1 << bits) - 1);
}

void ScintillaBase::Colourise(int start, int end) {
	if (!performingStyle) {
		// Protect against reentrance, which may occur, for example, when
		// fold points are discovered while performing styling and the folding
		// code looks for child lines which may trigger styling.
		performingStyle = true;

		int lengthDoc = pdoc->Length();
		if (end == -1)
			end = lengthDoc;
		int len = end - start;

		PLATFORM_ASSERT(len >= 0);
		PLATFORM_ASSERT(start + len <= lengthDoc);

		//WindowAccessor styler(wMain.GetID(), props);
		DocumentAccessor styler(pdoc, props, wMain.GetID());

		int styleStart = 0;
		if (start > 0)
			styleStart = styler.StyleAt(start - 1) & pdoc->stylingBitsMask;
		styler.SetCodePage(pdoc->dbcsCodePage);

		if (lexCurrent && (len > 0)) {	// Should always succeed as null lexer should always be available
			lexCurrent->Lex(start, len, styleStart, keyWordLists, styler);
			styler.Flush();
			if (styler.GetPropertyInt("fold")) {
				lexCurrent->Fold(start, len, styleStart, keyWordLists, styler);
				styler.Flush();
			}
		}

		performingStyle = false;
	}
}
#endif

void ScintillaBase::NotifyStyleToNeeded(int endStyleNeeded) {
#ifdef SCI_LEXER
	if (lexLanguage != SCLEX_CONTAINER) {
		int endStyled = WndProc(SCI_GETENDSTYLED, 0, 0);
		int lineEndStyled = WndProc(SCI_LINEFROMPOSITION, endStyled, 0);
		endStyled = WndProc(SCI_POSITIONFROMLINE, lineEndStyled, 0);
		Colourise(endStyled, endStyleNeeded);
		return;
	}
#endif
	Editor::NotifyStyleToNeeded(endStyleNeeded);
}

sptr_t ScintillaBase::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch (iMessage) {
	case SCI_AUTOCSHOW:
		listType = 0;
		AutoCompleteStart(wParam, reinterpret_cast<const char *>(lParam));
		break;

	case SCI_AUTOCCANCEL:
		ac.Cancel();
		break;

	case SCI_AUTOCACTIVE:
		return ac.Active();

	case SCI_AUTOCPOSSTART:
		return ac.posStart;

	case SCI_AUTOCCOMPLETE:
		AutoCompleteCompleted();
		break;

	case SCI_AUTOCSETSEPARATOR:
		ac.SetSeparator(static_cast<char>(wParam));
		break;

	case SCI_AUTOCGETSEPARATOR:
		return ac.GetSeparator();

	case SCI_AUTOCSTOPS:
		ac.SetStopChars(reinterpret_cast<char *>(lParam));
		break;

	case SCI_AUTOCSELECT:
		ac.Select(reinterpret_cast<char *>(lParam));
		break;

	case SCI_AUTOCGETCURRENT:
		return AutoCompleteGetCurrent();

	case SCI_AUTOCGETCURRENTTEXT:
		return AutoCompleteGetCurrentText(reinterpret_cast<char *>(lParam));

	case SCI_AUTOCSETCANCELATSTART:
		ac.cancelAtStartPos = wParam != 0;
		break;

	case SCI_AUTOCGETCANCELATSTART:
		return ac.cancelAtStartPos;

	case SCI_AUTOCSETFILLUPS:
		ac.SetFillUpChars(reinterpret_cast<char *>(lParam));
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

	case SCI_USERLISTSHOW:
		listType = wParam;
		AutoCompleteStart(0, reinterpret_cast<const char *>(lParam));
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
		ac.lb->SetVisibleRows(wParam);
		break;

	case SCI_AUTOCGETMAXHEIGHT:
		return ac.lb->GetVisibleRows();

	case SCI_AUTOCSETMAXWIDTH:
		maxListWidth = wParam;
		break;

	case SCI_AUTOCGETMAXWIDTH:
		return maxListWidth;

	case SCI_REGISTERIMAGE:
		ac.lb->RegisterImage(wParam, reinterpret_cast<const char *>(lParam));
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
			reinterpret_cast<const char *>(lParam));
		break;

	case SCI_CALLTIPCANCEL:
		ct.CallTipCancel();
		break;

	case SCI_CALLTIPACTIVE:
		return ct.inCallTipMode;

	case SCI_CALLTIPPOSSTART:
		return ct.posStartCallTip;

	case SCI_CALLTIPSETHLT:
		ct.SetHighlight(wParam, lParam);
		break;

	case SCI_CALLTIPSETBACK:
		ct.colourBG = ColourDesired(wParam);
		vs.styles[STYLE_CALLTIP].back = ct.colourBG;
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETFORE:
		ct.colourUnSel = ColourDesired(wParam);
		vs.styles[STYLE_CALLTIP].fore = ct.colourUnSel;
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPSETFOREHLT:
		ct.colourSel = ColourDesired(wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_CALLTIPUSESTYLE:
		ct.SetTabSize((int)wParam);
		InvalidateStyleRedraw();
		break;

	case SCI_USEPOPUP:
		displayPopupMenu = wParam != 0;
		break;

#ifdef SCI_LEXER
	case SCI_SETLEXER:
		SetLexer(wParam);
		lexLanguage = wParam;
		break;

	case SCI_GETLEXER:
		return lexLanguage;

	case SCI_COLOURISE:
		if (lexLanguage == SCLEX_CONTAINER) {
			pdoc->ModifiedAt(wParam);
			NotifyStyleToNeeded((lParam == -1) ? pdoc->Length() : lParam);
		} else {
			Colourise(wParam, lParam);
		}
		Redraw();
		break;

	case SCI_SETPROPERTY:
		props.Set(reinterpret_cast<const char *>(wParam),
		          reinterpret_cast<const char *>(lParam));
		break;

	case SCI_GETPROPERTY:
			return StringResult(lParam, props.Get(reinterpret_cast<const char *>(wParam)));

	case SCI_GETPROPERTYEXPANDED: {
			char *val = props.Expanded(reinterpret_cast<const char *>(wParam));
			const int n = strlen(val);
			if (lParam != 0) {
				char *ptr = reinterpret_cast<char *>(lParam);
				strcpy(ptr, val);
			}
			delete []val;
			return n;	// Not including NUL
		}

	case SCI_GETPROPERTYINT:
		return props.GetInt(reinterpret_cast<const char *>(wParam), lParam);

	case SCI_SETKEYWORDS:
		if (wParam < numWordLists) {
			keyWordLists[wParam]->Clear();
			keyWordLists[wParam]->Set(reinterpret_cast<const char *>(lParam));
		}
		break;

	case SCI_SETLEXERLANGUAGE:
		SetLexerLanguage(reinterpret_cast<const char *>(lParam));
		break;

	case SCI_GETLEXERLANGUAGE:
		return StringResult(lParam, lexCurrent ? lexCurrent->languageName : "");

	case SCI_GETSTYLEBITSNEEDED:
		return lexCurrent ? lexCurrent->GetStyleBitsNeeded() : 5;

#endif

	default:
		return Editor::WndProc(iMessage, wParam, lParam);
	}
	return 0l;
}
