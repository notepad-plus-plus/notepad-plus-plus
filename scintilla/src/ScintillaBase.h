// Scintilla source code edit control
/** @file ScintillaBase.h
 ** Defines an enhanced subclass of Editor with calltips, autocomplete and context menu.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCINTILLABASE_H
#define SCINTILLABASE_H

namespace Scintilla::Internal {

// For most platforms (not Cocoa) all IME indicators are drawn in same colour,
// blue, with different patterns.
constexpr ColourRGBA colourIME(0x0, 0x0, 0xffU);

constexpr int IndicatorInput = static_cast<int>(Scintilla::IndicatorNumbers::Ime);
constexpr int IndicatorTarget = IndicatorInput + 1;
constexpr int IndicatorConverted = IndicatorInput + 2;
constexpr int IndicatorUnknown = IndicatorInput + 3;

class LexState;
/**
 */
class ScintillaBase : public Editor, IListBoxDelegate {
protected:
	/** Enumeration of commands and child windows. */
	enum {
		idCallTip=1,
		idAutoComplete=2,

		idcmdUndo=10,
		idcmdRedo=11,
		idcmdCut=12,
		idcmdCopy=13,
		idcmdPaste=14,
		idcmdDelete=15,
		idcmdSelectAll=16
	};

	Scintilla::PopUp displayPopupMenu;
	Menu popup;
	Scintilla::Internal::AutoComplete ac;

	CallTip ct;

	int listType;			///< 0 is an autocomplete list
	int maxListWidth;		/// Maximum width of list, in average character widths
	Scintilla::MultiAutoComplete multiAutoCMode; /// Mode for autocompleting when multiple selections are present

	LexState *DocumentLexState();
	void Colourise(int start, int end);

	ScintillaBase();
	// Deleted so ScintillaBase objects can not be copied.
	ScintillaBase(const ScintillaBase &) = delete;
	ScintillaBase(ScintillaBase &&) = delete;
	ScintillaBase &operator=(const ScintillaBase &) = delete;
	ScintillaBase &operator=(ScintillaBase &&) = delete;
	// ~ScintillaBase() in public section
	void Initialise() override {}
	void Finalise() override;

	void InsertCharacter(std::string_view sv, Scintilla::CharacterSource charSource) override;
	void Command(int cmdId);
	void CancelModes() override;
	int KeyCommand(Scintilla::Message iMessage) override;

	void MoveImeCarets(Sci::Position offset) noexcept;
	void DrawImeIndicator(int indicator, Sci::Position len);

	void AutoCompleteInsert(Sci::Position startPos, Sci::Position removeLen, std::string_view text);
	void AutoCompleteStart(Sci::Position lenEntered, const char *list);
	void AutoCompleteCancel();
	void AutoCompleteMove(int delta);
	int AutoCompleteGetCurrent() const;
	int AutoCompleteGetCurrentText(char *buffer) const;
	void AutoCompleteCharacterAdded(char ch);
	void AutoCompleteCharacterDeleted();
	void AutoCompleteNotifyCompleted(char ch, CompletionMethods completionMethod, Sci::Position firstPos, const char *text);
	void AutoCompleteCompleted(char ch, Scintilla::CompletionMethods completionMethod);
	void AutoCompleteMoveToCurrentWord();
	void AutoCompleteSelection();
	void ListNotify(ListBoxEvent *plbe) override;

	void CallTipClick();
	void CallTipShow(Point pt, const char *defn);
	virtual void CreateCallTipWindow(PRectangle rc) = 0;

	virtual void AddToPopUp(const char *label, int cmd=0, bool enabled=true) = 0;
	bool ShouldDisplayPopup(Point ptInWindowCoordinates) const;
	void ContextMenu(Point pt);

	void ButtonDownWithModifiers(Point pt, unsigned int curTime, Scintilla::KeyMod modifiers) override;
	void RightButtonDownWithModifiers(Point pt, unsigned int curTime, Scintilla::KeyMod modifiers) override;

	void NotifyStyleToNeeded(Sci::Position endStyleNeeded) override;

public:
	~ScintillaBase() override;

	// Public so scintilla_send_message can use it
	Scintilla::sptr_t WndProc(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) override;
};

}

#endif
