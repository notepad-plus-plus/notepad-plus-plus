// Scintilla source code edit control
/** @file PerLine.cxx
 ** Manages data associated with each line of the document
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <forward_list>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "Scintilla.h"
#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "CellBuffer.h"
#include "PerLine.h"

using namespace Scintilla;

MarkerHandleSet::MarkerHandleSet() {
}

MarkerHandleSet::~MarkerHandleSet() {
	mhList.clear();
}

bool MarkerHandleSet::Empty() const noexcept {
	return mhList.empty();
}

int MarkerHandleSet::MarkValue() const noexcept {
	unsigned int m = 0;
	for (const MarkerHandleNumber &mhn : mhList) {
		m |= (1 << mhn.number);
	}
	return m;
}

bool MarkerHandleSet::Contains(int handle) const noexcept {
	for (const MarkerHandleNumber &mhn : mhList) {
		if (mhn.handle == handle) {
			return true;
		}
	}
	return false;
}

bool MarkerHandleSet::InsertHandle(int handle, int markerNum) {
	mhList.push_front(MarkerHandleNumber(handle, markerNum));
	return true;
}

void MarkerHandleSet::RemoveHandle(int handle) {
	mhList.remove_if([handle](const MarkerHandleNumber &mhn) { return mhn.handle == handle; });
}

bool MarkerHandleSet::RemoveNumber(int markerNum, bool all) {
	bool performedDeletion = false;
	mhList.remove_if([&](const MarkerHandleNumber &mhn) {
		if ((all || !performedDeletion) && (mhn.number == markerNum)) {
			performedDeletion = true;
			return true;
		}
		return false;
	});
	return performedDeletion;
}

void MarkerHandleSet::CombineWith(MarkerHandleSet *other) {
	mhList.splice_after(mhList.before_begin(), other->mhList);
}

LineMarkers::~LineMarkers() {
	markers.DeleteAll();
}

void LineMarkers::Init() {
	markers.DeleteAll();
}

void LineMarkers::InsertLine(Sci::Line line) {
	if (markers.Length()) {
		markers.Insert(line, 0);
	}
}

void LineMarkers::RemoveLine(Sci::Line line) {
	// Retain the markers from the deleted line by oring them into the previous line
	if (markers.Length()) {
		if (line > 0) {
			MergeMarkers(line - 1);
		}
		markers.Delete(line);
	}
}

Sci::Line LineMarkers::LineFromHandle(int markerHandle) {
	if (markers.Length()) {
		for (Sci::Line line = 0; line < markers.Length(); line++) {
			if (markers[line]) {
				if (markers[line]->Contains(markerHandle)) {
					return line;
				}
			}
		}
	}
	return -1;
}

void LineMarkers::MergeMarkers(Sci::Line line) {
	if (markers[line + 1]) {
		if (!markers[line])
			markers[line] = std::make_unique<MarkerHandleSet>();
		markers[line]->CombineWith(markers[line + 1].get());
		markers[line + 1].reset();
	}
}

int LineMarkers::MarkValue(Sci::Line line) noexcept {
	if (markers.Length() && (line >= 0) && (line < markers.Length()) && markers[line])
		return markers[line]->MarkValue();
	else
		return 0;
}

Sci::Line LineMarkers::MarkerNext(Sci::Line lineStart, int mask) const {
	if (lineStart < 0)
		lineStart = 0;
	const Sci::Line length = static_cast<Sci::Line>(markers.Length());
	for (Sci::Line iLine = lineStart; iLine < length; iLine++) {
		const MarkerHandleSet *onLine = markers[iLine].get();
		if (onLine && ((onLine->MarkValue() & mask) != 0))
			return iLine;
	}
	return -1;
}

int LineMarkers::AddMark(Sci::Line line, int markerNum, Sci::Line lines) {
	handleCurrent++;
	if (!markers.Length()) {
		// No existing markers so allocate one element per line
		markers.InsertEmpty(0, lines);
	}
	if (line >= markers.Length()) {
		return -1;
	}
	if (!markers[line]) {
		// Need new structure to hold marker handle
		markers[line] = std::make_unique<MarkerHandleSet>();
	}
	markers[line]->InsertHandle(handleCurrent, markerNum);

	return handleCurrent;
}

bool LineMarkers::DeleteMark(Sci::Line line, int markerNum, bool all) {
	bool someChanges = false;
	if (markers.Length() && (line >= 0) && (line < markers.Length()) && markers[line]) {
		if (markerNum == -1) {
			someChanges = true;
			markers[line].reset();
		} else {
			someChanges = markers[line]->RemoveNumber(markerNum, all);
			if (markers[line]->Empty()) {
				markers[line].reset();
			}
		}
	}
	return someChanges;
}

void LineMarkers::DeleteMarkFromHandle(int markerHandle) {
	const Sci::Line line = LineFromHandle(markerHandle);
	if (line >= 0) {
		markers[line]->RemoveHandle(markerHandle);
		if (markers[line]->Empty()) {
			markers[line].reset();
		}
	}
}

LineLevels::~LineLevels() {
}

void LineLevels::Init() {
	levels.DeleteAll();
}

void LineLevels::InsertLine(Sci::Line line) {
	if (levels.Length()) {
		const int level = (line < levels.Length()) ? levels[line] : SC_FOLDLEVELBASE;
		levels.InsertValue(line, 1, level);
	}
}

void LineLevels::RemoveLine(Sci::Line line) {
	if (levels.Length()) {
		// Move up following lines but merge header flag from this line
		// to line before to avoid a temporary disappearence causing expansion.
		int firstHeader = levels[line] & SC_FOLDLEVELHEADERFLAG;
		levels.Delete(line);
		if (line == levels.Length()-1) // Last line loses the header flag
			levels[line-1] &= ~SC_FOLDLEVELHEADERFLAG;
		else if (line > 0)
			levels[line-1] |= firstHeader;
	}
}

void LineLevels::ExpandLevels(Sci::Line sizeNew) {
	levels.InsertValue(levels.Length(), sizeNew - levels.Length(), SC_FOLDLEVELBASE);
}

void LineLevels::ClearLevels() {
	levels.DeleteAll();
}

int LineLevels::SetLevel(Sci::Line line, int level, Sci::Line lines) {
	int prev = 0;
	if ((line >= 0) && (line < lines)) {
		if (!levels.Length()) {
			ExpandLevels(lines + 1);
		}
		prev = levels[line];
		if (prev != level) {
			levels[line] = level;
		}
	}
	return prev;
}

int LineLevels::GetLevel(Sci::Line line) const {
	if (levels.Length() && (line >= 0) && (line < levels.Length())) {
		return levels[line];
	} else {
		return SC_FOLDLEVELBASE;
	}
}

LineState::~LineState() {
}

void LineState::Init() {
	lineStates.DeleteAll();
}

void LineState::InsertLine(Sci::Line line) {
	if (lineStates.Length()) {
		lineStates.EnsureLength(line);
		const int val = (line < lineStates.Length()) ? lineStates[line] : 0;
		lineStates.Insert(line, val);
	}
}

void LineState::RemoveLine(Sci::Line line) {
	if (lineStates.Length() > line) {
		lineStates.Delete(line);
	}
}

int LineState::SetLineState(Sci::Line line, int state) {
	lineStates.EnsureLength(line + 1);
	const int stateOld = lineStates[line];
	lineStates[line] = state;
	return stateOld;
}

int LineState::GetLineState(Sci::Line line) {
	if (line < 0)
		return 0;
	lineStates.EnsureLength(line + 1);
	return lineStates[line];
}

Sci::Line LineState::GetMaxLineState() const {
	return static_cast<Sci::Line>(lineStates.Length());
}

static int NumberLines(const char *text) noexcept {
	if (text) {
		int newLines = 0;
		while (*text) {
			if (*text == '\n')
				newLines++;
			text++;
		}
		return newLines+1;
	} else {
		return 0;
	}
}

// Each allocated LineAnnotation is a char array which starts with an AnnotationHeader
// and then has text and optional styles.

static const int IndividualStyles = 0x100;

struct AnnotationHeader {
	short style;	// Style IndividualStyles implies array of styles
	short lines;
	int length;
};

LineAnnotation::~LineAnnotation() {
	ClearAll();
}

void LineAnnotation::Init() {
	ClearAll();
}

void LineAnnotation::InsertLine(Sci::Line line) {
	if (annotations.Length()) {
		annotations.EnsureLength(line);
		annotations.Insert(line, std::unique_ptr<char []>());
	}
}

void LineAnnotation::RemoveLine(Sci::Line line) {
	if (annotations.Length() && (line > 0) && (line <= annotations.Length())) {
		annotations[line-1].reset();
		annotations.Delete(line-1);
	}
}

bool LineAnnotation::MultipleStyles(Sci::Line line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line].get())->style == IndividualStyles;
	else
		return false;
}

int LineAnnotation::Style(Sci::Line line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line].get())->style;
	else
		return 0;
}

const char *LineAnnotation::Text(Sci::Line line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return annotations[line].get()+sizeof(AnnotationHeader);
	else
		return nullptr;
}

const unsigned char *LineAnnotation::Styles(Sci::Line line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line] && MultipleStyles(line))
		return reinterpret_cast<unsigned char *>(annotations[line].get() + sizeof(AnnotationHeader) + Length(line));
	else
		return nullptr;
}

static std::unique_ptr<char[]>AllocateAnnotation(int length, int style) {
	const size_t len = sizeof(AnnotationHeader) + length + ((style == IndividualStyles) ? length : 0);
	return std::make_unique<char[]>(len);
}

void LineAnnotation::SetText(Sci::Line line, const char *text) {
	if (text && (line >= 0)) {
		annotations.EnsureLength(line+1);
		const int style = Style(line);
		annotations[line] = AllocateAnnotation(static_cast<int>(strlen(text)), style);
		char *pa = annotations[line].get();
		assert(pa);
		AnnotationHeader *pah = reinterpret_cast<AnnotationHeader *>(pa);
		pah->style = static_cast<short>(style);
		pah->length = static_cast<int>(strlen(text));
		pah->lines = static_cast<short>(NumberLines(text));
		memcpy(pa+sizeof(AnnotationHeader), text, pah->length);
	} else {
		if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line]) {
			annotations[line].reset();
		}
	}
}

void LineAnnotation::ClearAll() {
	annotations.DeleteAll();
}

void LineAnnotation::SetStyle(Sci::Line line, int style) {
	annotations.EnsureLength(line+1);
	if (!annotations[line]) {
		annotations[line] = AllocateAnnotation(0, style);
	}
	reinterpret_cast<AnnotationHeader *>(annotations[line].get())->style = static_cast<short>(style);
}

void LineAnnotation::SetStyles(Sci::Line line, const unsigned char *styles) {
	if (line >= 0) {
		annotations.EnsureLength(line+1);
		if (!annotations[line]) {
			annotations[line] = AllocateAnnotation(0, IndividualStyles);
		} else {
			const AnnotationHeader *pahSource = reinterpret_cast<AnnotationHeader *>(annotations[line].get());
			if (pahSource->style != IndividualStyles) {
				std::unique_ptr<char[]>allocation = AllocateAnnotation(pahSource->length, IndividualStyles);
				AnnotationHeader *pahAlloc = reinterpret_cast<AnnotationHeader *>(allocation.get());
				pahAlloc->length = pahSource->length;
				pahAlloc->lines = pahSource->lines;
				memcpy(allocation.get() + sizeof(AnnotationHeader), annotations[line].get() + sizeof(AnnotationHeader), pahSource->length);
				annotations[line] = std::move(allocation);
			}
		}
		AnnotationHeader *pah = reinterpret_cast<AnnotationHeader *>(annotations[line].get());
		pah->style = IndividualStyles;
		memcpy(annotations[line].get() + sizeof(AnnotationHeader) + pah->length, styles, pah->length);
	}
}

int LineAnnotation::Length(Sci::Line line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line].get())->length;
	else
		return 0;
}

int LineAnnotation::Lines(Sci::Line line) const {
	if (annotations.Length() && (line >= 0) && (line < annotations.Length()) && annotations[line])
		return reinterpret_cast<AnnotationHeader *>(annotations[line].get())->lines;
	else
		return 0;
}

LineTabstops::~LineTabstops() {
	tabstops.DeleteAll();
}

void LineTabstops::Init() {
	tabstops.DeleteAll();
}

void LineTabstops::InsertLine(Sci::Line line) {
	if (tabstops.Length()) {
		tabstops.EnsureLength(line);
		tabstops.Insert(line, nullptr);
	}
}

void LineTabstops::RemoveLine(Sci::Line line) {
	if (tabstops.Length() > line) {
		tabstops[line].reset();
		tabstops.Delete(line);
	}
}

bool LineTabstops::ClearTabstops(Sci::Line line) {
	if (line < tabstops.Length()) {
		TabstopList *tl = tabstops[line].get();
		if (tl) {
			tl->clear();
			return true;
		}
	}
	return false;
}

bool LineTabstops::AddTabstop(Sci::Line line, int x) {
	tabstops.EnsureLength(line + 1);
	if (!tabstops[line]) {
		tabstops[line] = std::make_unique<TabstopList>();
	}

	TabstopList *tl = tabstops[line].get();
	if (tl) {
		// tabstop positions are kept in order - insert in the right place
		std::vector<int>::iterator it = std::lower_bound(tl->begin(), tl->end(), x);
		// don't insert duplicates
		if (it == tl->end() || *it != x) {
			tl->insert(it, x);
			return true;
		}
	}
	return false;
}

int LineTabstops::GetNextTabstop(Sci::Line line, int x) const {
	if (line < tabstops.Length()) {
		TabstopList *tl = tabstops[line].get();
		if (tl) {
			for (const int i : *tl) {
				if (i > x) {
					return i;
				}
			}
		}
	}
	return 0;
}
