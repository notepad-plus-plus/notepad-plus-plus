// Scintilla source code edit control
/** @file ContractionState.cxx
 ** Manages visibility of lines for folding and wrapping.
 **/
// Copyright 1998-2007 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cassert>
#include <cstring>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>

#include "Debugging.h"

#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "SparseVector.h"
#include "ContractionState.h"

using namespace Scintilla::Internal;

namespace {

template <typename LINE>
class ContractionState final : public IContractionState {
	// These contain 1 element for every document line.
	std::unique_ptr<RunStyles<LINE, char>> visible;
	std::unique_ptr<RunStyles<LINE, char>> expanded;
	std::unique_ptr<RunStyles<LINE, int>> heights;
	std::unique_ptr<SparseVector<UniqueString>> foldDisplayTexts;
	std::unique_ptr<Partitioning<LINE>> displayLines;
	LINE linesInDocument;

	void EnsureData();

	bool OneToOne() const noexcept {
		// True when each document line is exactly one display line so need for
		// complex data structures.
		return visible == nullptr;
	}

	void InsertLine(Sci::Line lineDoc);
	void DeleteLine(Sci::Line lineDoc);

public:
	ContractionState() noexcept;
	// Deleted so ContractionState objects can not be copied.
	ContractionState(const ContractionState &) = delete;
	void operator=(const ContractionState &) = delete;
	ContractionState(ContractionState &&) = delete;
	void operator=(ContractionState &&) = delete;
	~ContractionState() override;

	void Clear() noexcept override;

	Sci::Line LinesInDoc() const noexcept override;
	Sci::Line LinesDisplayed() const noexcept override;
	Sci::Line DisplayFromDoc(Sci::Line lineDoc) const noexcept override;
	Sci::Line DisplayLastFromDoc(Sci::Line lineDoc) const noexcept override;
	Sci::Line DocFromDisplay(Sci::Line lineDisplay) const noexcept override;

	void InsertLines(Sci::Line lineDoc, Sci::Line lineCount) override;
	void DeleteLines(Sci::Line lineDoc, Sci::Line lineCount) override;

	bool GetVisible(Sci::Line lineDoc) const noexcept override;
	bool SetVisible(Sci::Line lineDocStart, Sci::Line lineDocEnd, bool isVisible) override;
	bool HiddenLines() const noexcept override;

	const char *GetFoldDisplayText(Sci::Line lineDoc) const noexcept override;
	bool SetFoldDisplayText(Sci::Line lineDoc, const char *text) override;

	bool GetExpanded(Sci::Line lineDoc) const noexcept override;
	bool SetExpanded(Sci::Line lineDoc, bool isExpanded) override;
	Sci::Line ContractedNext(Sci::Line lineDocStart) const noexcept override;

	int GetHeight(Sci::Line lineDoc) const noexcept override;
	bool SetHeight(Sci::Line lineDoc, int height) override;

	void ShowAll() noexcept override;

	void Check() const noexcept;
};

template <typename LINE>
ContractionState<LINE>::ContractionState() noexcept : linesInDocument(1) {
}

template <typename LINE>
ContractionState<LINE>::~ContractionState() = default;

template <typename LINE>
void ContractionState<LINE>::EnsureData() {
	if (OneToOne()) {
		visible = std::make_unique<RunStyles<LINE, char>>();
		expanded = std::make_unique<RunStyles<LINE, char>>();
		heights = std::make_unique<RunStyles<LINE, int>>();
		foldDisplayTexts = std::make_unique<SparseVector<UniqueString>>();
		displayLines = std::make_unique<Partitioning<LINE>>(4);
		InsertLines(0, linesInDocument);
	}
}

template <typename LINE>
void ContractionState<LINE>::InsertLine(Sci::Line lineDoc) {
	if (OneToOne()) {
		linesInDocument++;
	} else {
		const LINE lineDocCast = static_cast<LINE>(lineDoc);
		visible->InsertSpace(lineDocCast, 1);
		visible->SetValueAt(lineDocCast, 1);
		expanded->InsertSpace(lineDocCast, 1);
		expanded->SetValueAt(lineDocCast, 1);
		heights->InsertSpace(lineDocCast, 1);
		heights->SetValueAt(lineDocCast, 1);
		foldDisplayTexts->InsertSpace(lineDocCast, 1);
		foldDisplayTexts->SetValueAt(lineDocCast, nullptr);
		const Sci::Line lineDisplay = DisplayFromDoc(lineDoc);
		displayLines->InsertPartition(lineDocCast, static_cast<LINE>(lineDisplay));
		displayLines->InsertText(lineDocCast, 1);
	}
}

template <typename LINE>
void ContractionState<LINE>::DeleteLine(Sci::Line lineDoc) {
	if (OneToOne()) {
		linesInDocument--;
	} else {
		const LINE lineDocCast = static_cast<LINE>(lineDoc);
		if (GetVisible(lineDoc)) {
			displayLines->InsertText(lineDocCast, -heights->ValueAt(lineDocCast));
		}
		displayLines->RemovePartition(lineDocCast);
		visible->DeleteRange(lineDocCast, 1);
		expanded->DeleteRange(lineDocCast, 1);
		heights->DeleteRange(lineDocCast, 1);
		foldDisplayTexts->DeletePosition(lineDocCast);
	}
}

template <typename LINE>
void ContractionState<LINE>::Clear() noexcept {
	visible.reset();
	expanded.reset();
	heights.reset();
	foldDisplayTexts.reset();
	displayLines.reset();
	linesInDocument = 1;
}

template <typename LINE>
Sci::Line ContractionState<LINE>::LinesInDoc() const noexcept {
	if (OneToOne()) {
		return linesInDocument;
	} else {
		return displayLines->Partitions() - 1;
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::LinesDisplayed() const noexcept {
	if (OneToOne()) {
		return linesInDocument;
	} else {
		return displayLines->PositionFromPartition(static_cast<LINE>(LinesInDoc()));
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::DisplayFromDoc(Sci::Line lineDoc) const noexcept {
	if (OneToOne()) {
		return (lineDoc <= linesInDocument) ? lineDoc : linesInDocument;
	} else {
		if (lineDoc > displayLines->Partitions())
			lineDoc = displayLines->Partitions();
		return displayLines->PositionFromPartition(static_cast<LINE>(lineDoc));
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::DisplayLastFromDoc(Sci::Line lineDoc) const noexcept {
	return DisplayFromDoc(lineDoc) + GetHeight(lineDoc) - 1;
}

template <typename LINE>
Sci::Line ContractionState<LINE>::DocFromDisplay(Sci::Line lineDisplay) const noexcept {
	if (OneToOne()) {
		return lineDisplay;
	} else {
		if (lineDisplay < 0) {
			return 0;
		}
		if (lineDisplay > LinesDisplayed()) {
			return displayLines->PartitionFromPosition(static_cast<LINE>(LinesDisplayed()));
		}
		const Sci::Line lineDoc = displayLines->PartitionFromPosition(static_cast<LINE>(lineDisplay));
		PLATFORM_ASSERT(GetVisible(lineDoc));
		return lineDoc;
	}
}

template <typename LINE>
void ContractionState<LINE>::InsertLines(Sci::Line lineDoc, Sci::Line lineCount) {
	if (OneToOne()) {
		linesInDocument += static_cast<LINE>(lineCount);
	} else {
		for (Sci::Line l = 0; l < lineCount; l++) {
			InsertLine(lineDoc + l);
		}
	}
	Check();
}

template <typename LINE>
void ContractionState<LINE>::DeleteLines(Sci::Line lineDoc, Sci::Line lineCount) {
	if (OneToOne()) {
		linesInDocument -= static_cast<LINE>(lineCount);
	} else {
		for (Sci::Line l = 0; l < lineCount; l++) {
			DeleteLine(lineDoc);
		}
	}
	Check();
}

template <typename LINE>
bool ContractionState<LINE>::GetVisible(Sci::Line lineDoc) const noexcept {
	if (OneToOne()) {
		return true;
	} else {
		if (lineDoc >= visible->Length())
			return true;
		return visible->ValueAt(static_cast<LINE>(lineDoc)) == 1;
	}
}

template <typename LINE>
bool ContractionState<LINE>::SetVisible(Sci::Line lineDocStart, Sci::Line lineDocEnd, bool isVisible) {
	if (OneToOne() && isVisible) {
		return false;
	} else {
		EnsureData();
		Sci::Line delta = 0;
		Check();
		if ((lineDocStart <= lineDocEnd) && (lineDocStart >= 0) && (lineDocEnd < LinesInDoc())) {
			for (Sci::Line line = lineDocStart; line <= lineDocEnd; line++) {
				if (GetVisible(line) != isVisible) {
					const int heightLine = heights->ValueAt(static_cast<LINE>(line));
					const int difference = isVisible ? heightLine : -heightLine;
					visible->SetValueAt(static_cast<LINE>(line), isVisible ? 1 : 0);
					displayLines->InsertText(static_cast<LINE>(line), difference);
					delta += difference;
				}
			}
		} else {
			return false;
		}
		Check();
		return delta != 0;
	}
}

template <typename LINE>
bool ContractionState<LINE>::HiddenLines() const noexcept {
	if (OneToOne()) {
		return false;
	} else {
		return !visible->AllSameAs(1);
	}
}

template <typename LINE>
const char *ContractionState<LINE>::GetFoldDisplayText(Sci::Line lineDoc) const noexcept {
	Check();
	return foldDisplayTexts->ValueAt(lineDoc).get();
}

template <typename LINE>
bool ContractionState<LINE>::SetFoldDisplayText(Sci::Line lineDoc, const char *text) {
	EnsureData();
	const char *foldText = foldDisplayTexts->ValueAt(lineDoc).get();
	if (!foldText || !text || 0 != strcmp(text, foldText)) {
		UniqueString uns = IsNullOrEmpty(text) ? UniqueString() : UniqueStringCopy(text);
		foldDisplayTexts->SetValueAt(lineDoc, std::move(uns));
		Check();
		return true;
	} else {
		Check();
		return false;
	}
}

template <typename LINE>
bool ContractionState<LINE>::GetExpanded(Sci::Line lineDoc) const noexcept {
	if (OneToOne()) {
		return true;
	} else {
		Check();
		return expanded->ValueAt(static_cast<LINE>(lineDoc)) == 1;
	}
}

template <typename LINE>
bool ContractionState<LINE>::SetExpanded(Sci::Line lineDoc, bool isExpanded) {
	if (OneToOne() && isExpanded) {
		return false;
	} else {
		EnsureData();
		if (isExpanded != (expanded->ValueAt(static_cast<LINE>(lineDoc)) == 1)) {
			expanded->SetValueAt(static_cast<LINE>(lineDoc), isExpanded ? 1 : 0);
			Check();
			return true;
		} else {
			Check();
			return false;
		}
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::ContractedNext(Sci::Line lineDocStart) const noexcept {
	if (OneToOne()) {
		return -1;
	} else {
		Check();
		if (!expanded->ValueAt(static_cast<LINE>(lineDocStart))) {
			return lineDocStart;
		} else {
			const Sci::Line lineDocNextChange = expanded->EndRun(static_cast<LINE>(lineDocStart));
			if (lineDocNextChange < LinesInDoc())
				return lineDocNextChange;
			else
				return -1;
		}
	}
}

template <typename LINE>
int ContractionState<LINE>::GetHeight(Sci::Line lineDoc) const noexcept {
	if (OneToOne()) {
		return 1;
	} else {
		return heights->ValueAt(static_cast<LINE>(lineDoc));
	}
}

// Set the number of display lines needed for this line.
// Return true if this is a change.
template <typename LINE>
bool ContractionState<LINE>::SetHeight(Sci::Line lineDoc, int height) {
	if (OneToOne() && (height == 1)) {
		return false;
	} else if (lineDoc < LinesInDoc()) {
		EnsureData();
		if (GetHeight(lineDoc) != height) {
			if (GetVisible(lineDoc)) {
				displayLines->InsertText(static_cast<LINE>(lineDoc), height - GetHeight(lineDoc));
			}
			heights->SetValueAt(static_cast<LINE>(lineDoc), height);
			Check();
			return true;
		} else {
			Check();
			return false;
		}
	} else {
		return false;
	}
}

template <typename LINE>
void ContractionState<LINE>::ShowAll() noexcept {
	const LINE lines = static_cast<LINE>(LinesInDoc());
	Clear();
	linesInDocument = lines;
}

// Debugging checks

template <typename LINE>
void ContractionState<LINE>::Check() const noexcept {
#ifdef CHECK_CORRECTNESS
	for (Sci::Line vline = 0; vline < LinesDisplayed(); vline++) {
		const Sci::Line lineDoc = DocFromDisplay(vline);
		PLATFORM_ASSERT(GetVisible(lineDoc));
	}
	for (Sci::Line lineDoc = 0; lineDoc < LinesInDoc(); lineDoc++) {
		const Sci::Line displayThis = DisplayFromDoc(lineDoc);
		const Sci::Line displayNext = DisplayFromDoc(lineDoc + 1);
		const Sci::Line height = displayNext - displayThis;
		PLATFORM_ASSERT(height >= 0);
		if (GetVisible(lineDoc)) {
			PLATFORM_ASSERT(GetHeight(lineDoc) == height);
		} else {
			PLATFORM_ASSERT(0 == height);
		}
	}
#endif
}

}

namespace Scintilla::Internal {

std::unique_ptr<IContractionState> ContractionStateCreate(bool largeDocument) {
	if (largeDocument)
		return std::make_unique<ContractionState<Sci::Line>>();
	else
		return std::make_unique<ContractionState<int>>();
}

}
