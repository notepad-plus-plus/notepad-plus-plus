// Scintilla source code edit control
/** @file ContractionState.h
 ** Manages visibility of lines for folding and wrapping.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CONTRACTIONSTATE_H
#define CONTRACTIONSTATE_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */
class ContractionState {
	// These contain 1 element for every document line.
	RunStyles *visible;
	RunStyles *expanded;
	RunStyles *heights;
	Partitioning *displayLines;
	int linesInDocument;

	void EnsureData();

	bool OneToOne() const {
		// True when each document line is exactly one display line so need for
		// complex data structures.
		return visible == 0;
	}

public:
	ContractionState();
	virtual ~ContractionState();

	void Clear();

	int LinesInDoc() const;
	int LinesDisplayed() const;
	int DisplayFromDoc(int lineDoc) const;
	int DisplayLastFromDoc(int lineDoc) const;
	int DocFromDisplay(int lineDisplay) const;

	void InsertLine(int lineDoc);
	void InsertLines(int lineDoc, int lineCount);
	void DeleteLine(int lineDoc);
	void DeleteLines(int lineDoc, int lineCount);

	bool GetVisible(int lineDoc) const;
	bool SetVisible(int lineDocStart, int lineDocEnd, bool isVisible);
	bool HiddenLines() const;

	bool GetExpanded(int lineDoc) const;
	bool SetExpanded(int lineDoc, bool isExpanded);
	int ContractedNext(int lineDocStart) const;

	int GetHeight(int lineDoc) const;
	bool SetHeight(int lineDoc, int height);

	void ShowAll();
	void Check() const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
