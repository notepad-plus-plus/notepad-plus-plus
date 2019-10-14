// Scintilla source code edit control
/** @file SparseVector.h
 ** Hold data sparsely associated with elements in a range.
 **/
// Copyright 2016 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SPARSEVECTOR_H
#define SPARSEVECTOR_H

namespace Scintilla {

// SparseVector is similar to RunStyles but is more efficient for cases where values occur
// for one position instead of over a range of positions.
template <typename T>
class SparseVector {
private:
	std::unique_ptr<Partitioning<Sci::Position>> starts;
	std::unique_ptr<SplitVector<T>> values;
	T empty;
	void ClearValue(Sci::Position partition) {
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
	Sci::Position Length() const {
		return starts->PositionFromPartition(starts->Partitions());
	}
	Sci::Position Elements() const {
		return starts->Partitions();
	}
	Sci::Position PositionOfElement(int element) const {
		return starts->PositionFromPartition(element);
	}
	const T& ValueAt(Sci::Position position) const {
		assert(position < Length());
		const Sci::Position partition = starts->PartitionFromPosition(position);
		const Sci::Position startPartition = starts->PositionFromPartition(partition);
		if (startPartition == position) {
			return values->ValueAt(partition);
		} else {
			return empty;
		}
	}
	template <typename ParamType>
	void SetValueAt(Sci::Position position, ParamType &&value) {
		assert(position < Length());
		const Sci::Position partition = starts->PartitionFromPosition(position);
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
		assert(position <= Length());	// Only operation that works at end.
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
	}
	void Check() const {
		if (Length() < 0) {
			throw std::runtime_error("SparseVector: Length can not be negative.");
		}
		if (starts->Partitions() < 1) {
			throw std::runtime_error("SparseVector: Must always have 1 or more partitions.");
		}
		if (starts->Partitions() != values->Length() - 1) {
			throw std::runtime_error("SparseVector: Partitions and values different lengths.");
		}
		// The final element can not be set
		if (values->ValueAt(values->Length() - 1) != T()) {
			throw std::runtime_error("SparseVector: Unused style at end changed.");
		}
	}
};

}

#endif
