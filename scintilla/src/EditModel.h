// Scintilla source code edit control
/** @file EditModel.h
 ** Defines the editor state that must be visible to EditorView.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITMODEL_H
#define EDITMODEL_H

namespace Scintilla::Internal {

/**
*/
class Caret {
public:
	bool active;
	bool on;
	int period;

	Caret() noexcept;
};

enum class UndoRedo { undo, redo };

// Selection stack is sparse so use a map

struct SelectionWithScroll {
	std::string selection;
	Sci::Line topLine = 0;
};

using SelectionStack = std::map<int, SelectionWithScroll>;

struct SelectionHistory {
	int indexCurrent = 0;
	std::string ssCurrent;
	SelectionStack stack;
};

struct ModelState : ViewState {
	SelectionHistory historyForUndo;
	SelectionHistory historyForRedo;
	void RememberSelectionForUndo(int index, const Selection &sel);
	void ForgetSelectionForUndo() noexcept;
	void RememberSelectionOntoStack(int index, Sci::Line topLine);
	void RememberSelectionForRedoOntoStack(int index, const Selection &sel, Sci::Line topLine);
	SelectionWithScroll SelectionFromStack(int index, UndoRedo history) const;
	virtual void TruncateUndo(int index) final;
};

using ModelStateShared = std::shared_ptr<ModelState>;

class EditModel {
public:
	bool inOverstrike;
	int xOffset;		///< Horizontal scrolled amount in pixels
	bool trackLineWidth;

	std::unique_ptr<SpecialRepresentations> reprs;
	Caret caret;
	SelectionPosition posDrag;
	Sci::Position braces[2];
	int bracesMatchStyle;
	int highlightGuideColumn;
	bool hasFocus;
	Selection sel;
	bool primarySelection;
	std::string copySeparator;

	Scintilla::IMEInteraction imeInteraction;
	Scintilla::Bidirectional bidirectional;

	Scintilla::FoldFlag foldFlags;
	Scintilla::FoldDisplayTextStyle foldDisplayTextStyle;
	UniqueString defaultFoldDisplayText;
	std::unique_ptr<IContractionState> pcs;
	// Hotspot support
	Range hotspot;
	bool hotspotSingleLine;
	Sci::Position hoverIndicatorPos;

	Scintilla::ChangeHistoryOption changeHistoryOption = Scintilla::ChangeHistoryOption::Disabled;

	// Wrapping support
	int wrapWidth;

	Document *pdoc;

	Scintilla::UndoSelectionHistoryOption undoSelectionHistoryOption = UndoSelectionHistoryOption::Disabled;
	bool needRedoRemembered = false;
	ModelStateShared modelState;

	EditModel();
	// Deleted so EditModel objects can not be copied.
	EditModel(const EditModel &) = delete;
	EditModel(EditModel &&) = delete;
	EditModel &operator=(const EditModel &) = delete;
	EditModel &operator=(EditModel &&) = delete;
	virtual ~EditModel();
	virtual Sci::Line TopLineOfMain() const noexcept = 0;
	virtual Point GetVisibleOriginInMain() const = 0;
	virtual Sci::Line LinesOnScreen() const = 0;
	bool BidirectionalEnabled() const noexcept;
	bool BidirectionalR2L() const noexcept;
	SurfaceMode CurrentSurfaceMode() const noexcept;
	void SetDefaultFoldDisplayText(const char *text);
	const char *GetDefaultFoldDisplayText() const noexcept;
	const char *GetFoldDisplayText(Sci::Line lineDoc) const noexcept;
	InSelection LineEndInSelection(Sci::Line lineDoc) const;
	[[nodiscard]] int GetMark(Sci::Line line) const;

	void EnsureModelState();
	void ChangeUndoSelectionHistory(Scintilla::UndoSelectionHistoryOption undoSelectionHistoryOptionNew);
};

}

#endif
