/** @file Decoration.h
 ** Visual elements added over text.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DECORATION_H
#define DECORATION_H

namespace Scintilla {

class IDecoration {
public:
	virtual ~IDecoration() {}
	virtual bool Empty() const = 0;
	virtual int Indicator() const = 0;
	virtual Sci::Position Length() const = 0;
	virtual int ValueAt(Sci::Position position) const = 0;
	virtual Sci::Position StartRun(Sci::Position position) const = 0;
	virtual Sci::Position EndRun(Sci::Position position) const = 0;
	virtual void SetValueAt(Sci::Position position, int value) = 0;
	virtual void InsertSpace(Sci::Position position, Sci::Position insertLength) = 0;
	virtual Sci::Position Runs() const = 0;
};

class IDecorationList {
public:
	virtual ~IDecorationList() {}

	virtual const std::vector<const IDecoration*> &View() const =0;

	virtual void SetCurrentIndicator(int indicator) = 0;
	virtual int GetCurrentIndicator() const = 0;

	virtual void SetCurrentValue(int value) = 0;
	virtual int GetCurrentValue() const = 0;

	// Returns with changed=true if some values may have changed
	virtual FillResult<Sci::Position> FillRange(Sci::Position position, int value, Sci::Position fillLength) = 0;
	virtual void InsertSpace(Sci::Position position, Sci::Position insertLength) = 0;
	virtual void DeleteRange(Sci::Position position, Sci::Position deleteLength) = 0;
	virtual void DeleteLexerDecorations() = 0;

	virtual int AllOnFor(Sci::Position position) const = 0;
	virtual int ValueAt(int indicator, Sci::Position position) = 0;
	virtual Sci::Position Start(int indicator, Sci::Position position) = 0;
	virtual Sci::Position End(int indicator, Sci::Position position) = 0;

	virtual bool ClickNotified() const = 0;
	virtual void SetClickNotified(bool notified) = 0;
};

std::unique_ptr<IDecoration> DecorationCreate(bool largeDocument, int indicator);

std::unique_ptr<IDecorationList> DecorationListCreate(bool largeDocument);

}

#endif
