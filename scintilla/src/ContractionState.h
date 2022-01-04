// Scintilla source code edit control
/** @file ContractionState.h
 ** Manages visibility of lines for folding and wrapping.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CONTRACTIONSTATE_H
#define CONTRACTIONSTATE_H

namespace Scintilla::Internal {

/**
*/
class IContractionState {
public:
	virtual ~IContractionState() {};

	virtual void Clear()=0;

	virtual Sci::Line LinesInDoc() const noexcept=0;
	virtual Sci::Line LinesDisplayed() const noexcept=0;
	virtual Sci::Line DisplayFromDoc(Sci::Line lineDoc) const noexcept=0;
	virtual Sci::Line DisplayLastFromDoc(Sci::Line lineDoc) const noexcept=0;
	virtual Sci::Line DocFromDisplay(Sci::Line lineDisplay) const noexcept=0;

	virtual void InsertLines(Sci::Line lineDoc, Sci::Line lineCount)=0;
	virtual void DeleteLines(Sci::Line lineDoc, Sci::Line lineCount)=0;

	virtual bool GetVisible(Sci::Line lineDoc) const noexcept=0;
	virtual bool SetVisible(Sci::Line lineDocStart, Sci::Line lineDocEnd, bool isVisible)=0;
	virtual bool HiddenLines() const noexcept=0;

	virtual const char *GetFoldDisplayText(Sci::Line lineDoc) const noexcept=0;
	virtual bool SetFoldDisplayText(Sci::Line lineDoc, const char *text)=0;

	virtual bool GetExpanded(Sci::Line lineDoc) const noexcept=0;
	virtual bool SetExpanded(Sci::Line lineDoc, bool isExpanded)=0;
	virtual Sci::Line ContractedNext(Sci::Line lineDocStart) const noexcept =0;

	virtual int GetHeight(Sci::Line lineDoc) const noexcept=0;
	virtual bool SetHeight(Sci::Line lineDoc, int height)=0;

	virtual void ShowAll() noexcept=0;
};

std::unique_ptr<IContractionState> ContractionStateCreate(bool largeDocument);

}

#endif
