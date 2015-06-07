// Scintilla source code edit control
/** @file EditModel.h
 ** Defines the editor state that must be visible to EditorView.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITMODEL_H
#define EDITMODEL_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
*/
class Caret {
public:
	bool active;
	bool on;
	int period;

	Caret();
};

class EditModel {
	// Private so EditModel objects can not be copied
	explicit EditModel(const EditModel &);
	EditModel &operator=(const EditModel &);

public:
	bool inOverstrike;
	int xOffset;		///< Horizontal scrolled amount in pixels
	bool trackLineWidth;

	SpecialRepresentations reprs;
	Caret caret;
	SelectionPosition posDrag;
	Position braces[2];
	int bracesMatchStyle;
	int highlightGuideColumn;
	Selection sel;
	bool primarySelection;

	enum IMEInteraction { imeWindowed, imeInline } imeInteraction; 

	int foldFlags;
	ContractionState cs;
	// Hotspot support
	Range hotspot;
	int hoverIndicatorPos;

	// Wrapping support
	int wrapWidth;

	Document *pdoc;

	EditModel();
	virtual ~EditModel();
	virtual int TopLineOfMain() const = 0;
	virtual Point GetVisibleOriginInMain() const = 0;
	virtual int LinesOnScreen() const = 0;
	virtual Range GetHotSpotRange() const = 0;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
