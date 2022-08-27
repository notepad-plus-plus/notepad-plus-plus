/** @file RunStyles.h
 ** Data structure used to store sparse styles.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

/// Styling buffer using one element for each run rather than using
/// a filled buffer.

#ifndef RUNSTYLES_H
#define RUNSTYLES_H

namespace Scintilla::Internal {

// Return for RunStyles::FillRange reports if anything was changed and the
// range that was changed. This may be trimmed from the requested range
// when some of the requested range already had the requested value.
template <typename DISTANCE>
struct FillResult {
	bool changed;
	DISTANCE position;
	DISTANCE fillLength;
};

template <typename DISTANCE, typename STYLE>
class RunStyles {
private:
	Partitioning<DISTANCE> starts;
	SplitVector<STYLE> styles;
	DISTANCE RunFromPosition(DISTANCE position) const noexcept;
	DISTANCE SplitRun(DISTANCE position);
	void RemoveRun(DISTANCE run);
	void RemoveRunIfEmpty(DISTANCE run);
	void RemoveRunIfSameAsPrevious(DISTANCE run);
public:
	RunStyles();
	DISTANCE Length() const noexcept;
	STYLE ValueAt(DISTANCE position) const noexcept;
	DISTANCE FindNextChange(DISTANCE position, DISTANCE end) const noexcept;
	DISTANCE StartRun(DISTANCE position) const noexcept;
	DISTANCE EndRun(DISTANCE position) const noexcept;
	// Returns changed=true if some values may have changed
	FillResult<DISTANCE> FillRange(DISTANCE position, STYLE value, DISTANCE fillLength);
	void SetValueAt(DISTANCE position, STYLE value);
	void InsertSpace(DISTANCE position, DISTANCE insertLength);
	void DeleteAll();
	void DeleteRange(DISTANCE position, DISTANCE deleteLength);
	DISTANCE Runs() const noexcept;
	bool AllSame() const noexcept;
	bool AllSameAs(STYLE value) const noexcept;
	DISTANCE Find(STYLE value, DISTANCE start) const noexcept;

	void Check() const;
};

}

#endif
