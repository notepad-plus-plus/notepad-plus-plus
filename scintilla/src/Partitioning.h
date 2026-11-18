// Scintilla source code edit control
/** @file Partitioning.h
 ** Data structure used to partition an interval. Used for holding line start/end positions.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PARTITIONING_H
#define PARTITIONING_H

namespace Scintilla::Internal {

/// Divide an interval into multiple partitions.
/// Useful for breaking a document down into sections such as lines.
/// A 0 length interval has a single 0 length partition, numbered 0
/// If interval not 0 length then each partition non-zero length
/// When needed, positions after the interval are considered part of the last partition
/// but the end of the last partition can be found with PositionFromPartition(last+1).

template <typename T>
class Partitioning {
private:
	// To avoid calculating all the partition positions whenever any text is inserted
	// there may be a step somewhere in the list.
	T stepPartition;
	T stepLength;
	SplitVector<T> body;

	// Deleted so SplitVectorWithRangeAdd objects can not be copied.
	void RangeAddDelta(T start, T end, T delta) noexcept {
		// end is 1 past end, so end-start is number of elements to change
		const ptrdiff_t position = start;
		ptrdiff_t i = 0;
		const ptrdiff_t rangeLength = end - position;
		ptrdiff_t range1Length = rangeLength;
		const ptrdiff_t part1Left = body.GapPosition() - position;
		if (range1Length > part1Left)
			range1Length = part1Left;
		T *writer = &body[position];
		while (i < range1Length) {
			*writer += delta;
			writer++;
			i++;
		}
		if (i < rangeLength) {
			T *writer2 = &body[position + i];
			while (i < rangeLength) {
				*writer2 += delta;
				writer2++;
				i++;
			}
		}
	}

	// Move step forward
	void ApplyStep(T partitionUpTo) noexcept {
		if (stepLength != 0) {
			RangeAddDelta(stepPartition+1, partitionUpTo + 1, stepLength);
		}
		stepPartition = partitionUpTo;
		if (stepPartition >= Partitions()) {
			stepPartition = Partitions();
			stepLength = 0;
		}
	}

	// Move step backward
	void BackStep(T partitionDownTo) noexcept {
		if (stepLength != 0) {
			RangeAddDelta(partitionDownTo+1, stepPartition+1, -stepLength);
		}
		stepPartition = partitionDownTo;
	}

public:
	explicit Partitioning(size_t growSize=8) : stepPartition(0), stepLength(0), body(growSize) {
		body.Insert(0, 0);	// This value stays 0 for ever
		body.Insert(1, 0);	// This is the end of the first partition and will be the start of the second
	}

	T Partitions() const noexcept {
		return static_cast<T>(body.Length())-1;
	}

	void ReAllocate(ptrdiff_t newSize) {
		// + 1 accounts for initial element that is always 0.
		body.ReAllocate(newSize + 1);
	}

	T Length() const noexcept {
		return PositionFromPartition(Partitions());
	}

	void InsertPartition(T partition, T pos) {
		if (stepPartition < partition) {
			ApplyStep(partition);
		}
		body.Insert(partition, pos);
		stepPartition++;
	}

	void InsertPartitions(T partition, const T *positions, size_t length) {
		if (stepPartition < partition) {
			ApplyStep(partition);
		}
		body.InsertFromArray(partition, positions, 0, length);
		stepPartition += static_cast<T>(length);
	}

	void InsertPartitionsWithCast(T partition, const ptrdiff_t *positions, size_t length) {
		// Used for 64-bit builds when T is 32-bits
		if (stepPartition < partition) {
			ApplyStep(partition);
		}
		T *pInsertion = body.InsertEmpty(partition, length);
		for (size_t i = 0; i < length; i++) {
			pInsertion[i] = static_cast<T>(positions[i]);
		}
		stepPartition += static_cast<T>(length);
	}

	void SetPartitionStartPosition(T partition, T pos) noexcept {
		ApplyStep(partition+1);
		if ((partition < 0) || (partition >= body.Length())) {
			return;
		}
		body.SetValueAt(partition, pos);
	}

	void InsertText(T partitionInsert, T delta) noexcept {
		// Point all the partitions after the insertion point further along in the buffer
		if (stepLength != 0) {
			if (partitionInsert >= stepPartition) {
				// Fill in up to the new insertion point
				ApplyStep(partitionInsert);
				stepLength += delta;
			} else if (partitionInsert >= (stepPartition - body.Length() / 10)) {
				// Close to step but before so move step back
				BackStep(partitionInsert);
				stepLength += delta;
			} else {
				ApplyStep(Partitions());
				stepPartition = partitionInsert;
				stepLength = delta;
			}
		} else {
			stepPartition = partitionInsert;
			stepLength = delta;
		}
	}

	void RemovePartition(T partition) {
		if (partition > stepPartition) {
			ApplyStep(partition);
			stepPartition--;
		} else {
			stepPartition--;
		}
		body.Delete(partition);
	}

	T PositionFromPartition(T partition) const noexcept {
		PLATFORM_ASSERT(partition >= 0);
		PLATFORM_ASSERT(partition < body.Length());
		const ptrdiff_t lengthBody = body.Length();
		if ((partition < 0) || (partition >= lengthBody)) {
			return 0;
		}
		T pos = body.ValueAt(partition);
		if (partition > stepPartition)
			pos += stepLength;
		return pos;
	}

	/// Return value in range [0 .. Partitions() - 1] even for arguments outside interval
	T PartitionFromPosition(T pos) const noexcept {
		if (body.Length() <= 1)
			return 0;
		if (pos >= (PositionFromPartition(Partitions())))
			return Partitions() - 1;
		T lower = 0;
		T upper = Partitions();
		do {
			const T middle = (upper + lower + 1) / 2; 	// Round high
			T posMiddle = body[middle];
			if (middle > stepPartition)
				posMiddle += stepLength;
			if (pos < posMiddle) {
				upper = middle - 1;
			} else {
				lower = middle;
			}
		} while (lower < upper);
		return lower;
	}

	void DeleteAll() {
		body.DeleteAll();
		stepPartition = 0;
		stepLength = 0;
		body.Insert(0, 0);	// This value stays 0 for ever
		body.Insert(1, 0);	// This is the end of the first partition and will be the start of the second
	}

	void Check() const {
#ifdef CHECK_CORRECTNESS
		if (Length() < 0) {
			throw std::runtime_error("Partitioning: Length can not be negative.");
		}
		if (Partitions() < 1) {
			throw std::runtime_error("Partitioning: Must always have 1 or more partitions.");
		}
		if (Length() == 0) {
			if ((PositionFromPartition(0) != 0) || (PositionFromPartition(1) != 0)) {
				throw std::runtime_error("Partitioning: Invalid empty partitioning.");
			}
		} else {
			// Positions should be a strictly ascending sequence
			for (T i = 0; i < Partitions(); i++) {
				const T pos = PositionFromPartition(i);
				const T posNext = PositionFromPartition(i+1);
				if (pos > posNext) {
					throw std::runtime_error("Partitioning: Negative partition.");
				} else if (pos == posNext) {
					throw std::runtime_error("Partitioning: Empty partition.");
				}
			}
		}
#endif
	}

};


}

#endif
