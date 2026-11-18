// NoteX modifications: Copyright (C) 2026 Berk Kivircik.
// Modified for NoteX; latest publication changes: 2026-10-28.
// See MODIFICATIONS.md. Original copyright and license notices are preserved.

// Scintilla source code edit control
/** @file EditModel.cxx
 ** Defines the editor state that must be visible to EditorView.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterCategoryMap.h"

#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

void ModelState::RememberSelectionForUndo(int index, const Selection &sel) {
	historyForUndo.indexCurrent = index;
	historyForUndo.ssCurrent = sel.ToString();
}

void ModelState::ForgetSelectionForUndo() noexcept {
	historyForUndo.indexCurrent = -1;
}

void ModelState::RememberSelectionOntoStack(int index, Sci::Line topLine) {
	if ((historyForUndo.indexCurrent >= 0) && (index == historyForUndo.indexCurrent + 1)) {
		// Don't overwrite initial selection save if most recent action was coalesced
		historyForUndo.stack[index] = { historyForUndo.ssCurrent, topLine };
	}
}

void ModelState::RememberSelectionForRedoOntoStack(int index, const Selection &sel, Sci::Line topLine) {
	historyForRedo.stack[index] = { sel.ToString(), topLine };
}

SelectionWithScroll ModelState::SelectionFromStack(int index, UndoRedo history) const {
	const SelectionHistory &sh = history == UndoRedo::undo ? historyForUndo : historyForRedo;
	const SelectionStack::const_iterator it = sh.stack.find(index);
	if (it != sh.stack.end()) {
		return it->second;
	}
	return {};
}

void ModelState::TruncateUndo(int index) {
	const SelectionStack::const_iterator itUndo = historyForUndo.stack.find(index);
	historyForUndo.stack.erase(itUndo, historyForUndo.stack.end());
	const SelectionStack::const_iterator itRedo = historyForRedo.stack.find(index);
	historyForRedo.stack.erase(itRedo, historyForRedo.stack.end());
}

EditModel::EditModel() : braces{} {
	inOverstrike = false;
	xOffset = 0;
	trackLineWidth = false;
	posDrag = SelectionPosition(Sci::invalidPosition);
	braces[0] = Sci::invalidPosition;
	braces[1] = Sci::invalidPosition;
	bracesMatchStyle = StyleBraceBad;
	highlightGuideColumn = 0;
	hasFocus = false;
	primarySelection = true;
	imeInteraction = IMEInteraction::Windowed;
	bidirectional = Bidirectional::Disabled;
	foldFlags = FoldFlag::None;
	foldDisplayTextStyle = FoldDisplayTextStyle::Hidden;
	hotspot = Range(Sci::invalidPosition);
	hotspotSingleLine = true;
	hoverIndicatorPos = Sci::invalidPosition;
	wrapWidth = LineLayout::wrapWidthInfinite;
	reprs = std::make_unique<SpecialRepresentations>();
	pdoc = new Document(DocumentOption::Default);
	pdoc->AddRef();
	pcs = ContractionStateCreate(pdoc->IsLarge());
}

EditModel::~EditModel() {
	try {
		// Erasing the view state won't throw even though SetViewState
		// and the resulting map::erase aren't marked noexcept.
		pdoc->SetViewState(this, {});
		// This never throws but isn't marked noexcept for compatibility
		pdoc->Release();
	} catch (...) {
		// Ignore any exception
	}
	pdoc = nullptr;
}

bool EditModel::BidirectionalEnabled() const noexcept {
	return (bidirectional != Bidirectional::Disabled) &&
		(CpUtf8 == pdoc->dbcsCodePage);
}

bool EditModel::BidirectionalR2L() const noexcept {
	return bidirectional == Bidirectional::R2L;
}

SurfaceMode EditModel::CurrentSurfaceMode() const noexcept {
	return SurfaceMode(pdoc->dbcsCodePage, BidirectionalR2L());
}

void EditModel::SetDefaultFoldDisplayText(const char *text) {
	defaultFoldDisplayText = IsNullOrEmpty(text) ? UniqueString() : UniqueStringCopy(text);
}

const char *EditModel::GetDefaultFoldDisplayText() const noexcept {
	return defaultFoldDisplayText.get();
}

const char *EditModel::GetFoldDisplayText(Sci::Line lineDoc) const noexcept {
	if (foldDisplayTextStyle == FoldDisplayTextStyle::Hidden || pcs->GetExpanded(lineDoc)) {
		return nullptr;
	}

	const char *text = pcs->GetFoldDisplayText(lineDoc);
	return text ? text : defaultFoldDisplayText.get();
}

InSelection EditModel::LineEndInSelection(Sci::Line lineDoc) const {
	const Sci::Position posAfterLineEnd = pdoc->LineStart(lineDoc + 1);
	return sel.InSelectionForEOL(posAfterLineEnd);
}

Sci::Position EditModel::VirtualSpaceForLine(Sci::Line lineDoc) const {
	return sel.VirtualSpaceFor(pdoc->LineEnd(lineDoc));
}

int EditModel::GetMark(Sci::Line line) const {
	return pdoc->GetMark(line, FlagSet(changeHistoryOption, ChangeHistoryOption::Markers));
}

void EditModel::EnsureModelState() {
	if (!modelState && (undoSelectionHistoryOption != UndoSelectionHistoryOption::Disabled)) {
		if (ViewStateShared vss = pdoc->GetViewState(this)) {
			modelState = std::dynamic_pointer_cast<ModelState>(vss);
		} else {
			modelState = std::make_shared<ModelState>();
			pdoc->SetViewState(this, std::static_pointer_cast<ViewState>(modelState));
		}
	}
}

void EditModel::ChangeUndoSelectionHistory(Scintilla::UndoSelectionHistoryOption undoSelectionHistoryOptionNew) {
	undoSelectionHistoryOption = undoSelectionHistoryOptionNew;
	if (undoSelectionHistoryOption == UndoSelectionHistoryOption::Disabled) {
		modelState.reset();
		pdoc->SetViewState(this, {});
	}
}

// One coordinate model is shared by text, margins, hit testing and caret navigation.
void EditModel::UpdateRichMetrics(const ViewStyle &vs) {
    baseLineMetrics = {vs.maxAscent, vs.maxDescent, vs.lineHeight};
    const bool annotations=vs.annotationVisible != AnnotationVisible::Hidden;
    if(metricsAnnotations != annotations) measuredContractionRevision=size_t(-1);
    metricsAnnotations=annotations;
    if (measuredDocument == pdoc && measuredRichRevision == pdoc->rich.revision)
        return;
    measuredDocument = pdoc;
    measuredRichRevision = pdoc->rich.revision;
    measuredContractionRevision = size_t(-1);
    richLineY.clear();
    richLineMetrics.clear();
    if (pdoc->rich.state.runs.empty()) return;
    const auto count = pdoc->LinesTotal();
    richLineMetrics.assign(static_cast<size_t>(count), baseLineMetrics);
    std::vector<bool> affected(static_cast<size_t>(count));
    for (const auto &run : pdoc->rich.state.runs) {
        const auto first = pdoc->SciLineFromPosition(run.start);
        const auto last = pdoc->SciLineFromPosition(run.end - 1);
        for (auto line = first; line <= last; ++line) affected[line] = true;
    }
    for (Sci::Line line = 0; line < count; ++line) {
        if (!affected[line]) continue;
        auto start = pdoc->LineStart(line), end = pdoc->LineEnd(line);
        XYPOSITION ascent = 1, descent = 0;
        do {
            const auto &style = vs.RenderStyle(vs.RenderIndex(pdoc->StyleIndexAt(start), pdoc->rich.at(start)));
            ascent = std::max(ascent, style.ascent);
            descent = std::max(descent, style.descent);
        } while (++start < end);
        ascent = std::max(1.0, ascent + vs.extraAscent);
        descent = std::max(0.0, descent + vs.extraDescent);
        richLineMetrics[line] = {ascent, descent, std::max(1, static_cast<int>(std::ceil(ascent + descent)))};
    }
}
EditModel::LineMetrics EditModel::MetricsForLine(Sci::Line line) const noexcept {
    return line >= 0 && static_cast<size_t>(line) < richLineMetrics.size() ? richLineMetrics[line] : baseLineMetrics;
}
void EditModel::EnsureRichLineY() const {
    if (richLineMetrics.empty()) return;
    if (measuredContraction == pcs.get() && measuredContractionRevision == pcs->revision && !richLineY.empty()) return;
    measuredContraction = pcs.get();
    measuredContractionRevision = pcs->revision;
    richLineY.assign(richLineMetrics.size() + 1, 0);
    for (size_t line = 0; line < richLineMetrics.size(); ++line) {
        const int rows = pcs->GetVisible(line) ? pcs->GetHeight(line) : 0;
        const int annotations = rows && metricsAnnotations ? std::min(rows, pdoc->AnnotationLines(line)) : 0;
        richLineY[line + 1] = richLineY[line] + (rows - annotations) * richLineMetrics[line].height + annotations * baseLineMetrics.height;
    }
}
double EditModel::DisplayY(Sci::Line displayLine) const {
    const int base = std::max(1, baseLineMetrics.height);
    if (richLineMetrics.empty() || displayLine < 0) return static_cast<double>(displayLine) * base;
    EnsureRichLineY();
    if (displayLine >= pcs->LinesDisplayed()) return richLineY.back() + (displayLine - pcs->LinesDisplayed()) * base;
    const auto doc = pcs->DocFromDisplay(displayLine);
    const auto sub = displayLine - pcs->DisplayFromDoc(doc);
    const auto rows = pcs->GetHeight(doc) - (metricsAnnotations ? pdoc->AnnotationLines(doc) : 0);
    return richLineY[doc] + std::min(sub, static_cast<Sci::Line>(rows)) * MetricsForLine(doc).height + std::max<Sci::Line>(0, sub - rows) * base;
}
Sci::Line EditModel::DisplayLineFromY(double y) const {
    const int base = std::max(1, baseLineMetrics.height);
    if (richLineMetrics.empty() || y < 0) return static_cast<Sci::Line>(std::floor(y / base));
    EnsureRichLineY();
    if (y >= richLineY.back()) return pcs->LinesDisplayed() + static_cast<Sci::Line>((y - richLineY.back()) / base);
    const auto doc = static_cast<Sci::Line>(std::upper_bound(richLineY.begin(), richLineY.end(), y) - richLineY.begin() - 1);
    const int height = MetricsForLine(doc).height;
    const int rows = pcs->GetHeight(doc) - (metricsAnnotations ? pdoc->AnnotationLines(doc) : 0);
    const double offset = y - richLineY[doc];
    const auto sub = offset < rows * height ? static_cast<Sci::Line>(offset / height) : rows + static_cast<Sci::Line>((offset - rows * height) / base);
    return pcs->DisplayFromDoc(doc) + sub;
}
int EditModel::DisplayHeight(Sci::Line displayLine) const {
    return std::max(1, static_cast<int>(DisplayY(displayLine + 1) - DisplayY(displayLine)));
}
int EditModel::MaximumLineHeight() const noexcept {
    int height = baseLineMetrics.height;
    for (const auto &metrics : richLineMetrics) height = std::max(height, metrics.height);
    return height;
}

const Style &EditModel::StyleAtCaret(Sci::Position position, const ViewStyle &vs) const noexcept {
    const auto line = pdoc->SciLineFromPosition(position);
    if (position > pdoc->LineStart(line) && position >= pdoc->LineEnd(line)) --position;
    return vs.RenderStyle(vs.RenderIndex(pdoc->StyleIndexAt(position), pdoc->rich.at(position)));
}
