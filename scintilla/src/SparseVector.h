// Scintilla source code edit control
/** @file SparseVector.h
 ** Hold data sparsely associated with elements in a range.
 **/
// Copyright 2016 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SPARSEVECTOR_H
#define SPARSEVECTOR_H

namespace Scintilla::Internal {

// SparseVector is similar to RunStyles but is more efficient for cases where values occur
// for one position instead of over a range of positions.
// There are always elements at the start and end, so the element type should have
// a reasonable empty value that will cause no problems.
// The element type should have a noexcept default constructor as that allows methods to
// be noexcept.
template <typename T>
class SparseVector {
private:
	std::unique_ptr<Partitioning<Sci::Position>> starts;
	std::unique_ptr<SplitVector<T>> values;
	T empty;	// Return from ValueAt when no element at a position.
	void ClearValue(Sci::Position partition) noexcept {
		values->SetValueAt(partition, T());
	}
public:
	SparseVector() : empty() {
		starts = std::make_unique<Partitioning<Sci::Position>>(8);
		values = std::make_unique<SplitVector<T>>();
		values->InsertEmpty(0, 2);
	}
	// Deleted so SparseVector objects can not be copied.
	SparseVector(const SparseVector &) = delete;
	SparseVector(SparseVector &&) = delete;
	void operator=(const SparseVector &) = delete;
	void operator=(SparseVector &&) = delete;
	~SparseVector() {
		starts.reset();
		// starts dead here but not used by ClearValue.
		for (Sci::Position part = 0; part < values->Length(); part++) {
			ClearValue(part);
		}
		values.reset();
	}
	Sci::Position Length() const noexcept {
		return starts->Length();
	}
	Sci::Position Elements() const noexcept {
		return starts->Partitions();
	}
	Sci::Position PositionOfElement(Sci::Position element) const noexcept {
		return starts->PositionFromPartition(element);
	}
	Sci::Position ElementFromPosition(Sci::Position position) const noexcept {
		if (position < Length()) {
			return starts->PartitionFromPosition(position);
		} else {
			return starts->Partitions();
		}
	}
	const T& ValueAt(Sci::Position position) const noexcept {
		assert(position <= Length());
		const Sci::Position partition = ElementFromPosition(position);
		const Sci::Position startPartition = starts->PositionFromPartition(partition);
		if (startPartition == position) {
			return values->ValueAt(partition);
		} else {
			return empty;
		}
	}
	template <typename ParamType>
	void SetValueAt(Sci::Position position, ParamType &&value) {
		assert(position <= Length());
		const Sci::Position partition = ElementFromPosition(position);
		const Sci::Position startPartition = starts->PositionFromPartition(partition);
		if (value == T()) {
			// Setting the empty value is equivalent to deleting the position
			if (position == 0) {
				ClearValue(partition);
			} else if (position == startPartition) {
				// Currently an element at this position, so remove
				ClearValue(partition);
				starts->RemovePartition(partition);
				values->Delete(partition);
			}
			// Else element remains empty
		} else {
			if (position == startPartition) {
				// Already a value at this position, so replace
				ClearValue(partition);
				values->SetValueAt(partition, std::forward<ParamType>(value));
			} else {
				// Insert a new element
				starts->InsertPartition(partition + 1, position);
				values->Insert(partition + 1, std::forward<ParamType>(value));
			}
		}
	}
	void InsertSpace(Sci::Position position, Sci::Position insertLength) {
		assert(position <= Length());
		const Sci::Position partition = starts->PartitionFromPosition(position);
		const Sci::Position startPartition = starts->PositionFromPartition(partition);
		if (startPartition == position) {
			const bool positionOccupied = values->ValueAt(partition) != T();
			// Inserting at start of run so make previous longer
			if (partition == 0) {
				// Inserting at start of document so ensure start empty
				if (positionOccupied) {
					starts->InsertPartition(1, 0);
					values->InsertEmpty(0, 1);
				}
				starts->InsertText(partition, insertLength);
			} else {
				if (positionOccupied) {
					starts->InsertText(partition - 1, insertLength);
				} else {
					// Insert at end of run so do not extend style
					starts->InsertText(partition, insertLength);
				}
			}
		} else {
			starts->InsertText(partition, insertLength);
		}
	}
	void DeletePosition(Sci::Position position) {
		assert(position < Length());
		Sci::Position partition = starts->PartitionFromPosition(position);
		const Sci::Position startPartition = starts->PositionFromPartition(partition);
		if (startPartition == position) {
			if (partition == 0) {
				ClearValue(0);
				if (starts->PositionFromPartition(1) == 1) {
					// Removing all space of first partition, so remove next partition
					// and move value if not last
					if (Elements() > 1) {
						starts->RemovePartition(partition + 1);
						values->Delete(partition);
					}
				}
			} else if (partition == starts->Partitions()) {
				// This should not be possible
				ClearValue(partition);
				throw std::runtime_error("SparseVector: deleting end partition.");
			} else {
				ClearValue(partition);
				starts->RemovePartition(partition);
				values->Delete(partition);
				// Its the previous partition now that gets smaller
				partition--;
			}
		}
		starts->InsertText(partition, -1);
		Check();
	}
	void DeleteRange(Sci::Position position, Sci::Position deleteLength) {
		// For now, delete elements in range - may want to leave value at start
		// or combine onto position.
		if (position > Length() || (deleteLength == 0)) {
			return;
		}
		const Sci::Position positionEnd = position + deleteLength;
		assert(positionEnd <= Length());
		if (position == 0) {
			// Remove all partitions in range, moving values to start
			while ((Elements() > 1) && (starts->PositionFromPartition(1) <= deleteLength)) {
				starts->RemovePartition(1);
				values->Delete(0);
			}
			starts->InsertText(0, -deleteLength);
			if (Length() == 0) {
				ClearValue(0);
			}
		} else {
			const Sci::Position partition = starts->PartitionFromPosition(position);
			const bool atPartitionStart = position == starts->PositionFromPartition(partition);
			const Sci::Position partitionDelete = partition + (atPartitionStart ? 0 : 1);
			assert(partitionDelete > 0);
			for (;;) {
				const Sci::Position positionAtIndex = starts->PositionFromPartition(partitionDelete);
				assert(position <= positionAtIndex);
				if (positionAtIndex >= positionEnd) {
					break;
				}
				assert(partitionDelete <= Elements());
				starts->RemovePartition(partitionDelete);
				values->Delete(partitionDelete);
			}
			starts->InsertText(partition - (atPartitionStart ? 1 : 0), -deleteLength);
		}
		Check();
	}
	Sci::Position IndexAfter(Sci::Position position) const noexcept {
		assert(position < Length());
		if (position < 0)
			return 0;
		const Sci::Position partition = starts->PartitionFromPosition(position);
		return partition + 1;
	}
	void Check() const {
#ifdef CHECK_CORRECTNESS
		starts->Check();
		if (starts->Partitions() != values->Length() - 1) {
			throw std::runtime_error("SparseVector: Partitions and values different lengths.");
		}
#endif
	}
};

}

#endif
