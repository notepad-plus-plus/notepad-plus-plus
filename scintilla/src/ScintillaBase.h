// Scintilla source code edit control
/** @file ScintillaBase.h
 ** Defines an enhanced subclass of Editor with calltips, autocomplete and context menu.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCINTILLABASE_H
#define SCINTILLABASE_H

/**
 */
class ScintillaBase : public Editor {
	// Private so ScintillaBase objects can not be copied
	ScintillaBase(const ScintillaBase &) : Editor() {}
	ScintillaBase &operator=(const ScintillaBase &) { return *this; }

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

	bool displayPopupMenu;
	Menu popup;
	AutoComplete ac;

	CallTip ct;

	int listType;			///< 0 is an autocomplete list
	SString listSelected;	///< Receives listbox selected string
	int maxListWidth;		/// Maximum width of list, in average character widths

	bool performingStyle;	///< Prevent reentrance

#ifdef SCI_LEXER
	int lexLanguage;
	const LexerModule *lexCurrent;
	PropSet props;
	enum {numWordLists=KEYWORDSET_MAX+1};
	WordList *keyWordLists[numWordLists+1];
	void SetLexer(uptr_t wParam);
	void SetLexerLanguage(const char *languageName);
	void Colourise(int start, int end);
#endif

	ScintillaBase();
	virtual ~ScintillaBase();
	virtual void Initialise() = 0;
	virtual void Finalise() = 0;

	virtual void RefreshColourPalette(Palette &pal, bool want);

	virtual void AddCharUTF(char *s, unsigned int len, bool treatAsDBCS=false);
	void Command(int cmdId);
	virtual void CancelModes();
	virtual int KeyCommand(unsigned int iMessage);

	void AutoCompleteStart(int lenEntered, const char *list);
	void AutoCompleteCancel();
	void AutoCompleteMove(int delta);
	int AutoCompleteGetCurrent();
	void AutoCompleteCharacterAdded(char ch);
	void AutoCompleteCharacterDeleted();
	void AutoCompleteCompleted();
	void AutoCompleteMoveToCurrentWord();
	static void AutoCompleteDoubleClick(void* p);

	void CallTipClick();
	void CallTipShow(Point pt, const char *defn);
	virtual void CreateCallTipWindow(PRectangle rc) = 0;

	virtual void AddToPopUp(const char *label, int cmd=0, bool enabled=true) = 0;
	void ContextMenu(Point pt);

	virtual void ButtonDown(Point pt, unsigned int curTime, bool shift, bool ctrl, bool alt);

	virtual void NotifyStyleToNeeded(int endStyleNeeded);
public:
	// Public so scintilla_send_message can use it
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
};

#endif
