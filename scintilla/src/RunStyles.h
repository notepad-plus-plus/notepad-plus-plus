/** @file RunStyles.h
 ** Data structure used to store sparse styles.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

/// Styling buffer using one element for each run rather than using
/// a filled buffer.

#ifndef RUNSTYLES_H
#define RUNSTYLES_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class RunStyles {
private:
	Partitioning *starts;
	SplitVector<int> *styles;
	int RunFromPosition(int position) const;
	int SplitRun(int position);
	void RemoveRun(int run);
	void RemoveRunIfEmpty(int run);
	void RemoveRunIfSameAsPrevious(int run);
	// Private so RunStyles objects can not be copied
	RunStyles(const RunStyles &);
public:
	RunStyles();
	~RunStyles();
	int Length() const;
	int ValueAt(int position) const;
	int FindNextChange(int position, int end) const;
	int StartRun(int position) const;
	int EndRun(int position) const;
	// Returns true if some values may have changed
	bool FillRange(int &position, int value, int &fillLength);
	void SetValueAt(int position, int value);
	void InsertSpace(int position, int insertLength);
	void DeleteAll();
	void DeleteRange(int position, int deleteLength);
	int Runs() const;
	bool AllSame() const;
	bool AllSameAs(int value) const;
	int Find(int value, int start) const;

	void Check() const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
