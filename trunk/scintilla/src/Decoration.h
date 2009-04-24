/** @file Decoration.h
 ** Visual elements added over text.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DECORATION_H
#define DECORATION_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class Decoration {
public:
	Decoration *next;
	RunStyles rs;
	int indicator;

	Decoration(int indicator_);
	~Decoration();

	bool Empty();
};

class DecorationList {
	int currentIndicator;
	int currentValue;
	Decoration *current;
	int lengthDocument;
	Decoration *DecorationFromIndicator(int indicator);
	Decoration *Create(int indicator, int length);
	void Delete(int indicator);
	void DeleteAnyEmpty();
public:
	Decoration *root;
	bool clickNotified;

	DecorationList();
	~DecorationList();

	void SetCurrentIndicator(int indicator);
	int GetCurrentIndicator() { return currentIndicator; }

	void SetCurrentValue(int value);
	int GetCurrentValue() { return currentValue; }

	// Returns true if some values may have changed
	bool FillRange(int &position, int value, int &fillLength);

	void InsertSpace(int position, int insertLength);
	void DeleteRange(int position, int deleteLength);

	int AllOnFor(int position);
	int ValueAt(int indicator, int position);
	int Start(int indicator, int position);
	int End(int indicator, int position);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
