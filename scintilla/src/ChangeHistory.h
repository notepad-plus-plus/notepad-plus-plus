// Scintilla source code edit control
/** @file ChangeHistory.h
 ** Manages a history of changes in a document.
 **/
// Copyright 2022 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef CHANGEHISTORY_H
#define CHANGEHISTORY_H

namespace Scintilla::Internal {

constexpr int changeOriginal = 0;
constexpr int changeRevertedOriginal = 1;
constexpr int changeSaved = 2;
constexpr int changeModified = 3;
constexpr int changeRevertedToChange = 4;

// As bit flags
constexpr unsigned int bitRevertedToOriginal = 1;
constexpr unsigned int bitSaved = 2;
constexpr unsigned int bitModified = 4;
constexpr unsigned int bitRevertedToModified = 8;

struct ChangeSpan {
	Sci::Position start;
	Sci::Position length;
	int edition;
	int count;
	enum class Direction { insertion, deletion } direction;
};

struct EditionCount {
	int edition;
	int count;
	// Used in tests.
	constexpr bool operator==(const EditionCount &other) const noexcept {
		return (edition == other.edition) && (count == other.count);
	}
};

// EditionSet is ordered from oldest to newest, its not really a set
using EditionSet = std::vector<EditionCount>;
using EditionSetOwned = std::unique_ptr<EditionSet>;

class ChangeStack {
	std::vector<int> steps;
	std::vector<ChangeSpan> changes;
public:
	void Clear() noexcept;
	void AddStep();
	void PushDeletion(Sci::Position positionDeletion, const EditionCount &ec);
	void PushInsertion(Sci::Position positionInsertion, Sci::Position length, int edition);
	[[nodiscard]] int PopStep() noexcept;
	[[nodiscard]] ChangeSpan PopSpan(int maxSteps) noexcept;
	void SetSavePoint() noexcept;
	void Check() const noexcept;
};

struct ChangeLog {
	ChangeStack changeStack;
	RunStyles<Sci::Position, int> insertEdition;
	SparseVector<EditionSetOwned> deleteEdition;

	void Clear(Sci::Position length);
	void InsertSpace(Sci::Position position, Sci::Position insertLength);
	void DeleteRange(Sci::Position position, Sci::Position deleteLength);
	void Insert(Sci::Position start, Sci::Position length, int edition);
	void CollapseRange(Sci::Position position, Sci::Position deleteLength);
	void PushDeletionAt(Sci::Position position, EditionCount ec);
	void InsertFrontDeletionAt(Sci::Position position, EditionCount ec);
	void SaveRange(Sci::Position position, Sci::Position length);
	void PopDeletion(Sci::Position position, Sci::Position deleteLength);
	void SaveHistoryForDelete(Sci::Position position, Sci::Position deleteLength);
	void DeleteRangeSavingHistory(Sci::Position position, Sci::Position deleteLength);
	void SetSavePoint();

	Sci::Position Length() const noexcept;
	[[nodiscard]] size_t DeletionCount(Sci::Position start, Sci::Position length) const noexcept;
	void Check() const noexcept;
};

enum class ReversionState { clear, reverting, detached };

class ChangeHistory {
	ChangeLog changeLog;
	std::unique_ptr<ChangeLog> changeLogReversions;
	int historicEpoch = -1;

public:
	ChangeHistory(Sci::Position length=0);

	void Insert(Sci::Position position, Sci::Position insertLength, bool collectingUndo, bool beforeSave);
	void DeleteRange(Sci::Position position, Sci::Position deleteLength, bool reverting);
	void DeleteRangeSavingHistory(Sci::Position position, Sci::Position deleteLength, bool beforeSave, bool isDetached);

	void StartReversion();
	void EndReversion() noexcept;

	void SetSavePoint();

	void UndoDeleteStep(Sci::Position position, Sci::Position deleteLength, bool isDetached);

	[[nodiscard]] Sci::Position Length() const noexcept;

	// Setting up history before this session
	void SetEpoch(int epoch) noexcept;
	void EditionCreateHistory(Sci::Position start, Sci::Position length);

	// Queries for drawing
	[[nodiscard]] int EditionAt(Sci::Position pos) const noexcept;
	[[nodiscard]] Sci::Position EditionEndRun(Sci::Position pos) const noexcept;
	[[nodiscard]] unsigned int EditionDeletesAt(Sci::Position pos) const noexcept;
	[[nodiscard]] Sci::Position EditionNextDelete(Sci::Position pos) const noexcept;

	// Testing - not used by Scintilla
	[[nodiscard]] size_t DeletionCount(Sci::Position start, Sci::Position length) const noexcept;
	EditionSet DeletionsAt(Sci::Position pos) const;
	void Check() noexcept;
};

}

#endif
