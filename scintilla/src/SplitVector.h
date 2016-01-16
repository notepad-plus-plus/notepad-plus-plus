// Scintilla source code edit control
/** @file SplitVector.h
 ** Main data structure for holding arrays that handle insertions
 ** and deletions efficiently.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SPLITVECTOR_H
#define SPLITVECTOR_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

template <typename T>
class SplitVector {
protected:
	T *body;
	int size;
	int lengthBody;
	int part1Length;
	int gapLength;	/// invariant: gapLength == size - lengthBody
	int growSize;

	/// Move the gap to a particular position so that insertion and
	/// deletion at that point will not require much copying and
	/// hence be fast.
	void GapTo(int position) {
		if (position != part1Length) {
			if (position < part1Length) {
				memmove(
					body + position + gapLength,
					body + position,
					sizeof(T) * (part1Length - position));
			} else {	// position > part1Length
				memmove(
					body + part1Length,
					body + part1Length + gapLength,
					sizeof(T) * (position - part1Length));
			}
			part1Length = position;
		}
	}

	/// Check that there is room in the buffer for an insertion,
	/// reallocating if more space needed.
	void RoomFor(int insertionLength) {
		if (gapLength <= insertionLength) {
			while (growSize < size / 6)
				growSize *= 2;
			ReAllocate(size + insertionLength + growSize);
		}
	}

	void Init() {
		body = NULL;
		growSize = 8;
		size = 0;
		lengthBody = 0;
		part1Length = 0;
		gapLength = 0;
	}

public:
	/// Construct a split buffer.
	SplitVector() {
		Init();
	}

	~SplitVector() {
		delete []body;
		body = 0;
	}

	int GetGrowSize() const {
		return growSize;
	}

	void SetGrowSize(int growSize_) {
		growSize = growSize_;
	}

	/// Reallocate the storage for the buffer to be newSize and
	/// copy exisiting contents to the new buffer.
	/// Must not be used to decrease the size of the buffer.
	void ReAllocate(int newSize) {
		if (newSize < 0)
			throw std::runtime_error("SplitVector::ReAllocate: negative size.");

		if (newSize > size) {
			// Move the gap to the end
			GapTo(lengthBody);
			T *newBody = new T[newSize];
			if ((size != 0) && (body != 0)) {
				memmove(newBody, body, sizeof(T) * lengthBody);
				delete []body;
			}
			body = newBody;
			gapLength += newSize - size;
			size = newSize;
		}
	}

	/// Retrieve the character at a particular position.
	/// Retrieving positions outside the range of the buffer returns 0.
	/// The assertions here are disabled since calling code can be
	/// simpler if out of range access works and returns 0.
	T ValueAt(int position) const {
		if (position < part1Length) {
			//PLATFORM_ASSERT(position >= 0);
			if (position < 0) {
				return 0;
			} else {
				return body[position];
			}
		} else {
			//PLATFORM_ASSERT(position < lengthBody);
			if (position >= lengthBody) {
				return 0;
			} else {
				return body[gapLength + position];
			}
		}
	}

	void SetValueAt(int position, T v) {
		if (position < part1Length) {
			PLATFORM_ASSERT(position >= 0);
			if (position < 0) {
				;
			} else {
				body[position] = v;
			}
		} else {
			PLATFORM_ASSERT(position < lengthBody);
			if (position >= lengthBody) {
				;
			} else {
				body[gapLength + position] = v;
			}
		}
	}

	T &operator[](int position) const {
		PLATFORM_ASSERT(position >= 0 && position < lengthBody);
		if (position < part1Length) {
			return body[position];
		} else {
			return body[gapLength + position];
		}
	}

	/// Retrieve the length of the buffer.
	int Length() const {
		return lengthBody;
	}

	/// Insert a single value into the buffer.
	/// Inserting at positions outside the current range fails.
	void Insert(int position, T v) {
		PLATFORM_ASSERT((position >= 0) && (position <= lengthBody));
		if ((position < 0) || (position > lengthBody)) {
			return;
		}
		RoomFor(1);
		GapTo(position);
		body[part1Length] = v;
		lengthBody++;
		part1Length++;
		gapLength--;
	}

	/// Insert a number of elements into the buffer setting their value.
	/// Inserting at positions outside the current range fails.
	void InsertValue(int position, int insertLength, T v) {
		PLATFORM_ASSERT((position >= 0) && (position <= lengthBody));
		if (insertLength > 0) {
			if ((position < 0) || (position > lengthBody)) {
				return;
			}
			RoomFor(insertLength);
			GapTo(position);
			std::fill(&body[part1Length], &body[part1Length + insertLength], v);
			lengthBody += insertLength;
			part1Length += insertLength;
			gapLength -= insertLength;
		}
	}

	/// Ensure at least length elements allocated,
	/// appending zero valued elements if needed.
	void EnsureLength(int wantedLength) {
		if (Length() < wantedLength) {
			InsertValue(Length(), wantedLength - Length(), 0);
		}
	}

	/// Insert text into the buffer from an array.
	void InsertFromArray(int positionToInsert, const T s[], int positionFrom, int insertLength) {
		PLATFORM_ASSERT((positionToInsert >= 0) && (positionToInsert <= lengthBody));
		if (insertLength > 0) {
			if ((positionToInsert < 0) || (positionToInsert > lengthBody)) {
				return;
			}
			RoomFor(insertLength);
			GapTo(positionToInsert);
			memmove(body + part1Length, s + positionFrom, sizeof(T) * insertLength);
			lengthBody += insertLength;
			part1Length += insertLength;
			gapLength -= insertLength;
		}
	}

	/// Delete one element from the buffer.
	void Delete(int position) {
		PLATFORM_ASSERT((position >= 0) && (position < lengthBody));
		if ((position < 0) || (position >= lengthBody)) {
			return;
		}
		DeleteRange(position, 1);
	}

	/// Delete a range from the buffer.
	/// Deleting positions outside the current range fails.
	void DeleteRange(int position, int deleteLength) {
		PLATFORM_ASSERT((position >= 0) && (position + deleteLength <= lengthBody));
		if ((position < 0) || ((position + deleteLength) > lengthBody)) {
			return;
		}
		if ((position == 0) && (deleteLength == lengthBody)) {
			// Full deallocation returns storage and is faster
			delete []body;
			Init();
		} else if (deleteLength > 0) {
			GapTo(position);
			lengthBody -= deleteLength;
			gapLength += deleteLength;
		}
	}

	/// Delete all the buffer contents.
	void DeleteAll() {
		DeleteRange(0, lengthBody);
	}

	// Retrieve a range of elements into an array
	void GetRange(T *buffer, int position, int retrieveLength) const {
		// Split into up to 2 ranges, before and after the split then use memcpy on each.
		int range1Length = 0;
		if (position < part1Length) {
			int part1AfterPosition = part1Length - position;
			range1Length = retrieveLength;
			if (range1Length > part1AfterPosition)
				range1Length = part1AfterPosition;
		}
		memcpy(buffer, body + position, range1Length * sizeof(T));
		buffer += range1Length;
		position = position + range1Length + gapLength;
		int range2Length = retrieveLength - range1Length;
		memcpy(buffer, body + position, range2Length * sizeof(T));
	}

	T *BufferPointer() {
		RoomFor(1);
		GapTo(lengthBody);
		body[lengthBody] = 0;
		return body;
	}

	T *RangePointer(int position, int rangeLength) {
		if (position < part1Length) {
			if ((position + rangeLength) > part1Length) {
				// Range overlaps gap, so move gap to start of range.
				GapTo(position);
				return body + position + gapLength;
			} else {
				return body + position;
			}
		} else {
			return body + position + gapLength;
		}
	}

	int GapPosition() const {
		return part1Length;
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
