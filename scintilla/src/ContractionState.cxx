// Scintilla source code edit control
/** @file ContractionState.cxx
 ** Manages visibility of lines for folding.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "Platform.h"

#include "ContractionState.h"

OneLine::OneLine() {
	displayLine = 0;
	//docLine = 0;
	visible = true;
	height = 1;
	expanded = true;
}

ContractionState::ContractionState() {
	lines = 0;
	size = 0;
	linesInDoc = 1;
	linesInDisplay = 1;
	valid = false;
	docLines = 0;
	sizeDocLines = 0;
}

ContractionState::~ContractionState() {
	Clear();
}

void ContractionState::MakeValid() const {
	if (!valid) {
		// Could be cleverer by keeping the index of the last still valid entry
		// rather than invalidating all.
		linesInDisplay = 0;
		for (int lineInDoc=0; lineInDoc<linesInDoc; lineInDoc++) {
			lines[lineInDoc].displayLine = linesInDisplay;
			if (lines[lineInDoc].visible) {
				linesInDisplay += lines[lineInDoc].height;
			}
		}
		if (sizeDocLines < linesInDisplay) {
			delete []docLines;
			int *docLinesNew = new int[linesInDisplay + growSize];
			if (!docLinesNew) {
				docLines = 0;
				sizeDocLines = 0;
				return;
			}
			docLines = docLinesNew;
			sizeDocLines = linesInDisplay + growSize;
		}

		int lineInDisplay=0;
		for (int line=0; line<linesInDoc; line++) {
			if (lines[line].visible) {
				for (int linePiece=0; linePiece<lines[line].height; linePiece++) {
					docLines[lineInDisplay] = line;
					lineInDisplay++;
				}
			}
		}
		valid = true;
	}
}

void ContractionState::Clear() {
	delete []lines;
	lines = 0;
	size = 0;
	linesInDoc = 1;
	linesInDisplay = 1;
	delete []docLines;
	docLines = 0;
	sizeDocLines = 0;
}

int ContractionState::LinesInDoc() const {
	return linesInDoc;
}

int ContractionState::LinesDisplayed() const {
	if (size != 0) {
		MakeValid();
	}
	return linesInDisplay;
}

int ContractionState::DisplayFromDoc(int lineDoc) const {
	if (size == 0) {
		return lineDoc;
	}
	MakeValid();
	if ((lineDoc >= 0) && (lineDoc < linesInDoc)) {
		return lines[lineDoc].displayLine;
	}
	return -1;
}

int ContractionState::DocFromDisplay(int lineDisplay) const {
	if (lineDisplay <= 0)
		return 0;
	if (lineDisplay >= linesInDisplay)
		return linesInDoc;
	if (size == 0)
		return lineDisplay;
	MakeValid();
	if (docLines) {	// Valid allocation
		return docLines[lineDisplay];
	} else {
		return 0;
	}
}

void ContractionState::Grow(int sizeNew) {
	OneLine *linesNew = new OneLine[sizeNew];
	if (linesNew) {
		int i = 0;
		for (; i < size; i++) {
			linesNew[i] = lines[i];
		}
		for (; i < sizeNew; i++) {
			linesNew[i].displayLine = i;
		}
		delete []lines;
		lines = linesNew;
		size = sizeNew;
		valid = false;
	} else {
		Platform::DebugPrintf("No memory available\n");
		// TODO: Blow up
	}
}

void ContractionState::InsertLines(int lineDoc, int lineCount) {
	if (size == 0) {
		linesInDoc += lineCount;
		linesInDisplay += lineCount;
		return;
	}
	//Platform::DebugPrintf("InsertLine[%d] = %d\n", lineDoc);
	if ((linesInDoc + lineCount + 2) >= size) {
		Grow(linesInDoc + lineCount + growSize);
	}
	linesInDoc += lineCount;
	for (int i = linesInDoc; i >= lineDoc + lineCount; i--) {
		lines[i].visible = lines[i - lineCount].visible;
		lines[i].height = lines[i - lineCount].height;
		linesInDisplay += lines[i].height;
		lines[i].expanded = lines[i - lineCount].expanded;
	}
	for (int d=0;d<lineCount;d++) {
		lines[lineDoc+d].visible = true;	// Should inherit visibility from context ?
		lines[lineDoc+d].height = 1;
		lines[lineDoc+d].expanded = true;
	}
	valid = false;
}

void ContractionState::DeleteLines(int lineDoc, int lineCount) {
	if (size == 0) {
		linesInDoc -= lineCount;
		linesInDisplay -= lineCount;
		return;
	}
	int deltaDisplayed = 0;
	for (int d=0;d<lineCount;d++) {
		if (lines[lineDoc+d].visible)
			deltaDisplayed -= lines[lineDoc+d].height;
	}
	for (int i = lineDoc; i < linesInDoc-lineCount; i++) {
		if (i != 0) // Line zero is always visible
			lines[i].visible = lines[i + lineCount].visible;
		lines[i].expanded = lines[i + lineCount].expanded;
		lines[i].height = lines[i + lineCount].height;
	}
	linesInDoc -= lineCount;
	linesInDisplay += deltaDisplayed;
	valid = false;
}

bool ContractionState::GetVisible(int lineDoc) const {
	if (size == 0)
		return true;
	if ((lineDoc >= 0) && (lineDoc < linesInDoc)) {
		return lines[lineDoc].visible;
	} else {
		return false;
	}
}

bool ContractionState::SetVisible(int lineDocStart, int lineDocEnd, bool visible) {
	if (lineDocStart == 0)
		lineDocStart++;
	if (lineDocStart > lineDocEnd)
		return false;
	if (size == 0) {
		Grow(linesInDoc + growSize);
	}
	// TODO: modify docLine members to mirror displayLine
	int delta = 0;
	// Change lineDocs
	if ((lineDocStart <= lineDocEnd) && (lineDocStart >= 0) && (lineDocEnd < linesInDoc)) {
		for (int line=lineDocStart; line <= lineDocEnd; line++) {
			if (lines[line].visible != visible) {
				delta += visible ? lines[line].height : -lines[line].height;
				lines[line].visible = visible;
				valid = false;
			}
		}
	}
	linesInDisplay += delta;
	return delta != 0;
}

bool ContractionState::GetExpanded(int lineDoc) const {
	if (size == 0)
		return true;
	if ((lineDoc >= 0) && (lineDoc < linesInDoc)) {
		return lines[lineDoc].expanded;
	} else {
		return false;
	}
}

bool ContractionState::SetExpanded(int lineDoc, bool expanded) {
	if (size == 0) {
		if (expanded) {
			// If in completely expanded state then setting
			// one line to expanded has no effect.
			return false;
		}
		Grow(linesInDoc + growSize);
	}
	if ((lineDoc >= 0) && (lineDoc < linesInDoc)) {
		if (lines[lineDoc].expanded != expanded) {
			lines[lineDoc].expanded = expanded;
			return true;
		}
	}
	return false;
}

int ContractionState::GetHeight(int lineDoc) const {
	if (size == 0)
		return 1;
	if ((lineDoc >= 0) && (lineDoc < linesInDoc)) {
		return lines[lineDoc].height;
	} else {
		return 1;
	}
}

// Set the number of display lines needed for this line.
// Return true if this is a change.
bool ContractionState::SetHeight(int lineDoc, int height) {
	if (lineDoc > linesInDoc)
		return false;
	if (size == 0) {
		if (height == 1) {
			// If in completely expanded state then all lines
			// assumed to have height of one so no effect here.
			return false;
		}
		Grow(linesInDoc + growSize);
	}
	if (lines[lineDoc].height != height) {
		lines[lineDoc].height = height;
		valid = false;
		return true;
	} else {
		return false;
	}
}

void ContractionState::ShowAll() {
	delete []lines;
	lines = 0;
	size = 0;

	delete []docLines;
	docLines = 0;
	sizeDocLines = 0;

	linesInDisplay = linesInDoc;
}
