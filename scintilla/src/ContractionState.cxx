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

	// line_cast(): cast Sci::Line to either 32-bit or 64-bit value
	// This avoids warnings from Visual C++ Code Analysis and shortens code
	static constexpr LINE line_cast(Sci::Line line) noexcept {
		return static_cast<LINE>(line);
	}

public:
	ContractionState() noexcept;

	void Clear() noexcept override;

	Sci::Line LinesInDoc() const noexcept override;
	Sci::Line LinesDisplayed() const noexcept override;
	Sci::Line DisplayFromDoc(Sci::Line lineDoc) const noexcept override;
	Sci::Line DisplayFromDocSub(Sci::Line lineDoc, Sci::Line lineSub) const noexcept override;
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
	bool ExpandAll() override;
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
		const LINE lineDocCast = line_cast(lineDoc);
		visible->InsertSpace(lineDocCast, 1);
		visible->SetValueAt(lineDocCast, 1);
		expanded->InsertSpace(lineDocCast, 1);
		expanded->SetValueAt(lineDocCast, 1);
		heights->InsertSpace(lineDocCast, 1);
		heights->SetValueAt(lineDocCast, 1);
		foldDisplayTexts->InsertSpace(lineDocCast, 1);
		foldDisplayTexts->SetValueAt(lineDocCast, nullptr);
		const Sci::Line lineDisplay = DisplayFromDoc(lineDoc);
		displayLines->InsertPartition(lineDocCast, line_cast(lineDisplay));
		displayLines->InsertText(lineDocCast, 1);
	}
}

template <typename LINE>
void ContractionState<LINE>::DeleteLine(Sci::Line lineDoc) {
	if (OneToOne()) {
		linesInDocument--;
	} else {
		const LINE lineDocCast = line_cast(lineDoc);
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
		return displayLines->PositionFromPartition(line_cast(LinesInDoc()));
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::DisplayFromDoc(Sci::Line lineDoc) const noexcept {
	if (OneToOne()) {
		return (lineDoc <= linesInDocument) ? lineDoc : linesInDocument;
	} else {
		if (lineDoc > displayLines->Partitions())
			lineDoc = displayLines->Partitions();
		return displayLines->PositionFromPartition(line_cast(lineDoc));
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::DisplayFromDocSub(Sci::Line lineDoc, Sci::Line lineSub) const noexcept {
	return DisplayFromDoc(lineDoc) +
		std::min(lineSub, static_cast<Sci::Line>(GetHeight(lineDoc) - 1));
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
			return displayLines->PartitionFromPosition(line_cast(LinesDisplayed()));
		}
		const Sci::Line lineDoc = displayLines->PartitionFromPosition(line_cast(lineDisplay));
		PLATFORM_ASSERT(GetVisible(lineDoc));
		return lineDoc;
	}
}

template <typename LINE>
void ContractionState<LINE>::InsertLines(Sci::Line lineDoc, Sci::Line lineCount) {
	if (OneToOne()) {
		linesInDocument += line_cast(lineCount);
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
		linesInDocument -= line_cast(lineCount);
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
		return visible->ValueAt(line_cast(lineDoc)) == 1;
	}
}

template <typename LINE>
bool ContractionState<LINE>::SetVisible(Sci::Line lineDocStart, Sci::Line lineDocEnd, bool isVisible) {
	if (OneToOne() && isVisible) {
		return false;
	} else {
		EnsureData();
		Check();
		if ((lineDocStart <= lineDocEnd) && (lineDocStart >= 0) && (lineDocEnd < LinesInDoc())) {
			bool changed = false;
			for (Sci::Line line = lineDocStart; line <= lineDocEnd; line++) {
				if (GetVisible(line) != isVisible) {
					changed = true;
					const int heightLine = heights->ValueAt(line_cast(line));
					const int difference = isVisible ? heightLine : -heightLine;
					displayLines->InsertText(line_cast(line), difference);
				}
			}
			if (changed) {
				visible->FillRange(line_cast(lineDocStart), isVisible ? 1 : 0,
					line_cast(lineDocEnd - lineDocStart) + 1);
			}
			Check();
			return changed;
		} else {
			return false;
		}
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
		return expanded->ValueAt(line_cast(lineDoc)) == 1;
	}
}

template <typename LINE>
bool ContractionState<LINE>::SetExpanded(Sci::Line lineDoc, bool isExpanded) {
	if (OneToOne() && isExpanded) {
		return false;
	} else {
		EnsureData();
		if (isExpanded != (expanded->ValueAt(line_cast(lineDoc)) == 1)) {
			expanded->SetValueAt(line_cast(lineDoc), isExpanded ? 1 : 0);
			Check();
			return true;
		} else {
			Check();
			return false;
		}
	}
}

template <typename LINE>
bool ContractionState<LINE>::ExpandAll() {
	if (OneToOne()) {
		return false;
	} else {
		const LINE lines = expanded->Length();
		const bool changed = expanded->FillRange(0, 1, lines).changed;
		Check();
		return changed;
	}
}

template <typename LINE>
Sci::Line ContractionState<LINE>::ContractedNext(Sci::Line lineDocStart) const noexcept {
	if (OneToOne()) {
		return -1;
	} else {
		Check();
		if (!expanded->ValueAt(line_cast(lineDocStart))) {
			return lineDocStart;
		} else {
			const Sci::Line lineDocNextChange = expanded->EndRun(line_cast(lineDocStart));
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
		return heights->ValueAt(line_cast(lineDoc));
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
				displayLines->InsertText(line_cast(lineDoc), height - GetHeight(lineDoc));
			}
			heights->SetValueAt(line_cast(lineDoc), height);
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
	const LINE lines = line_cast(LinesInDoc());
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
