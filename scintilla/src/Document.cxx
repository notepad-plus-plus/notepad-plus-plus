// Scintilla source code edit control
/** @file Document.cxx
 ** Text document that handles notifications, DBCS, styling, words and end of line.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <forward_list>
#include <optional>
#include <algorithm>
#include <memory>
#include <chrono>

#ifndef NO_CXX11_REGEX
#include <regex>
#endif

#include "ScintillaTypes.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"

#include "CharacterType.h"
#include "CharacterCategoryMap.h"
#include "Position.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "RESearch.h"
#include "UniConversion.h"
#include "ElapsedPeriod.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

#if defined(__GNUC__) && !defined(__clang__)
// False warnings from g++ 14.1 for UTF-8 accumulation code where UTF8MaxBytes allocated.
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

LexInterface::LexInterface(Document *pdoc_) noexcept : pdoc(pdoc_), performingStyle(false) {
}

LexInterface::~LexInterface() noexcept = default;

void LexInterface::SetInstance(ILexer5 *instance_) noexcept {
	instance.reset(instance_);
}

void LexInterface::Colourise(Sci::Position start, Sci::Position end) {
	if (pdoc && instance && !performingStyle) {
		// Protect against reentrance, which may occur, for example, when
		// fold points are discovered while performing styling and the folding
		// code looks for child lines which may trigger styling.
		performingStyle = true;

		const Sci::Position lengthDoc = pdoc->Length();
		if (end == -1)
			end = lengthDoc;
		const Sci::Position len = end - start;

		PLATFORM_ASSERT(len >= 0);
		PLATFORM_ASSERT(start + len <= lengthDoc);

		int styleStart = 0;
		if (start > 0)
			styleStart = pdoc->StyleAt(start - 1);

		if (len > 0) {
			instance->Lex(start, len, styleStart, pdoc);
			instance->Fold(start, len, styleStart, pdoc);
		}

		performingStyle = false;
	}
}

LineEndType LexInterface::LineEndTypesSupported() {
	if (instance) {
		return static_cast<LineEndType>(instance->LineEndTypesSupported());
	}
	return LineEndType::Default;
}

bool LexInterface::UseContainerLexing() const noexcept {
	return !instance;
}

ActionDuration::ActionDuration(double duration_, double minDuration_, double maxDuration_) noexcept :
	duration(duration_), minDuration(minDuration_), maxDuration(maxDuration_) {
}

void ActionDuration::AddSample(size_t numberActions, double durationOfActions) noexcept {
	// Only adjust for multiple actions to avoid instability
	if (numberActions < 8)
		return;

	// Alpha value for exponential smoothing.
	// Most recent value contributes 25% to smoothed value.
	constexpr double alpha = 0.25;

	const double durationOne = durationOfActions / numberActions;
	duration = std::clamp(alpha * durationOne + (1.0 - alpha) * duration,
		minDuration, maxDuration);
}

double ActionDuration::Duration() const noexcept {
	return duration;
}

size_t ActionDuration::ActionsInAllowedTime(double secondsAllowed) const noexcept {
	return std::lround(secondsAllowed / Duration());
}

CharacterExtracted::CharacterExtracted(const unsigned char *charBytes, size_t widthCharBytes) noexcept {
	const int utf8status = UTF8Classify(charBytes, widthCharBytes);
	if (utf8status & UTF8MaskInvalid) {
		// Treat as invalid and use up just one byte
		character = unicodeReplacementChar;
		widthBytes = 1;
	} else {
		character = UnicodeFromUTF8(charBytes);
		widthBytes = utf8status & UTF8MaskWidth;
	}
}

Document::Document(DocumentOption options) :
	refCount(0),
	cb(!FlagSet(options, DocumentOption::StylesNone), FlagSet(options, DocumentOption::TextLarge)),
	endStyled(0),
	styleClock(0),
	enteredModification(0),
	enteredStyling(0),
	enteredReadOnlyCount(0),
	insertionSet(false),
#ifdef _WIN32
	eolMode(EndOfLine::CrLf),
#else
	eolMode(EndOfLine::Lf),
#endif
	dbcsCodePage(CpUtf8),
	lineEndBitSet(LineEndType::Default),
	tabInChars(8),
	indentInChars(0),
	actualIndentInChars(8),
	useTabs(true),
	tabIndents(true),
	backspaceUnindents(false),
	durationStyleOneByte(0.000001, 0.0000001, 0.00001) {

	perLineData[ldMarkers] = std::make_unique<LineMarkers>();
	perLineData[ldLevels] = std::make_unique<LineLevels>();
	perLineData[ldState] = std::make_unique<LineState>();
	perLineData[ldMargin] = std::make_unique<LineAnnotation>();
	perLineData[ldAnnotation] = std::make_unique<LineAnnotation>();
	perLineData[ldEOLAnnotation] = std::make_unique<LineAnnotation>();

	decorations = DecorationListCreate(IsLarge());

	cb.SetPerLine(this);
	cb.SetUTF8Substance(CpUtf8 == dbcsCodePage);
}

Document::~Document() {
	for (const WatcherWithUserData &watcher : watchers) {
		watcher.watcher->NotifyDeleted(this, watcher.userData);
	}
}

// Increase reference count and return its previous value.
int SCI_METHOD Document::AddRef() noexcept {
	return refCount++;
}

// Decrease reference count and return its previous value.
// Delete the document if reference count reaches zero.
int SCI_METHOD Document::Release() {
	const int curRefCount = --refCount;
	if (curRefCount == 0)
		delete this;
	return curRefCount;
}

void Document::Init() {
	for (const std::unique_ptr<PerLine> &pl : perLineData) {
		if (pl)
			pl->Init();
	}
}

void Document::InsertLine(Sci::Line line) {
	for (const std::unique_ptr<PerLine> &pl : perLineData) {
		if (pl)
			pl->InsertLine(line);
	}
}

void Document::InsertLines(Sci::Line line, Sci::Line lines) {
	for (const auto &pl : perLineData) {
		if (pl)
			pl->InsertLines(line, lines);
	}
}

void Document::RemoveLine(Sci::Line line) {
	for (const std::unique_ptr<PerLine> &pl : perLineData) {
		if (pl)
			pl->RemoveLine(line);
	}
}

LineMarkers *Document::Markers() const noexcept {
	return static_cast<LineMarkers *>(perLineData[ldMarkers].get());
}

LineLevels *Document::Levels() const noexcept {
	return static_cast<LineLevels *>(perLineData[ldLevels].get());
}

LineState *Document::States() const noexcept {
	return static_cast<LineState *>(perLineData[ldState].get());
}

LineAnnotation *Document::Margins() const noexcept {
	return static_cast<LineAnnotation *>(perLineData[ldMargin].get());
}

LineAnnotation *Document::Annotations() const noexcept {
	return static_cast<LineAnnotation *>(perLineData[ldAnnotation].get());
}

LineAnnotation *Document::EOLAnnotations() const noexcept {
	return static_cast<LineAnnotation *>(perLineData[ldEOLAnnotation].get());
}

LineEndType Document::LineEndTypesSupported() const {
	if ((CpUtf8 == dbcsCodePage) && pli)
		return pli->LineEndTypesSupported();
	else
		return LineEndType::Default;
}

bool Document::SetDBCSCodePage(int dbcsCodePage_) {
	if (dbcsCodePage != dbcsCodePage_) {
		dbcsCodePage = dbcsCodePage_;
		SetCaseFolder(nullptr);
		cb.SetLineEndTypes(lineEndBitSet & LineEndTypesSupported());
		cb.SetUTF8Substance(CpUtf8 == dbcsCodePage);
		ModifiedAt(0);	// Need to restyle whole document
		return true;
	} else {
		return false;
	}
}

bool Document::SetLineEndTypesAllowed(LineEndType lineEndBitSet_) {
	if (lineEndBitSet != lineEndBitSet_) {
		lineEndBitSet = lineEndBitSet_;
		const LineEndType lineEndBitSetActive = lineEndBitSet & LineEndTypesSupported();
		if (lineEndBitSetActive != cb.GetLineEndTypes()) {
			ModifiedAt(0);
			cb.SetLineEndTypes(lineEndBitSetActive);
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void Document::SetSavePoint() {
	cb.SetSavePoint();
	NotifySavePoint(true);
}

void Document::TentativeUndo() {
	if (!TentativeActive())
		return;
	CheckReadOnly();
	if (enteredModification == 0) {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			const bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			const int steps = cb.TentativeSteps();
			//Platform::DebugPrintf("Steps=%d\n", steps);
			for (int step = 0; step < steps; step++) {
				const Sci::Line prevLinesTotal = LinesTotal();
				const Action action = cb.GetUndoStep();
				if (action.at == ActionType::remove) {
					NotifyModified(DocModification(
									ModificationFlags::BeforeInsert | ModificationFlags::Undo, action));
				} else if (action.at == ActionType::container) {
					DocModification dm(ModificationFlags::Container | ModificationFlags::Undo);
					dm.token = action.position;
					NotifyModified(dm);
				} else {
					NotifyModified(DocModification(
									ModificationFlags::BeforeDelete | ModificationFlags::Undo, action));
				}
				cb.PerformUndoStep();
				if (action.at != ActionType::container) {
					ModifiedAt(action.position);
				}

				ModificationFlags modFlags = ModificationFlags::Undo;
				// With undo, an insertion action becomes a deletion notification
				if (action.at == ActionType::remove) {
					modFlags |= ModificationFlags::InsertText;
				} else if (action.at == ActionType::insert) {
					modFlags |= ModificationFlags::DeleteText;
				}
				if (steps > 1)
					modFlags |= ModificationFlags::MultiStepUndoRedo;
				const Sci::Line linesAdded = LinesTotal() - prevLinesTotal;
				if (linesAdded != 0)
					multiLine = true;
				if (step == steps - 1) {
					modFlags |= ModificationFlags::LastStepInUndoRedo;
					if (multiLine)
						modFlags |= ModificationFlags::MultilineUndoRedo;
				}
				NotifyModified(DocModification(modFlags, action.position, action.lenData,
											   linesAdded, action.data));
			}

			const bool endSavePoint = cb.IsSavePoint();
			if (startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);

			cb.TentativeCommit();
		}
		enteredModification--;
	}
}

int Document::UndoActions() const noexcept {
	return cb.UndoActions();
}

void Document::SetUndoSavePoint(int action) noexcept {
	cb.SetUndoSavePoint(action);
}

int Document::UndoSavePoint() const noexcept {
	return cb.UndoSavePoint();
}

void Document::SetUndoDetach(int action) noexcept {
	cb.SetUndoDetach(action);
}

int Document::UndoDetach() const noexcept {
	return cb.UndoDetach();
}

void Document::SetUndoTentative(int action) noexcept {
	cb.SetUndoTentative(action);
}

int Document::UndoTentative() const noexcept {
	return cb.UndoTentative();
}

void Document::SetUndoCurrent(int action) {
	cb.SetUndoCurrent(action);
}

int Document::UndoCurrent() const noexcept {
	return cb.UndoCurrent();
}

int Document::UndoActionType(int action) const noexcept {
	return cb.UndoActionType(action);
}

Sci::Position Document::UndoActionPosition(int action) const noexcept {
	return cb.UndoActionPosition(action);
}

std::string_view Document::UndoActionText(int action) const noexcept {
	return cb.UndoActionText(action);
}

void Document::PushUndoActionType(int type, Sci::Position position) {
	cb.PushUndoActionType(type, position);
}

void Document::ChangeLastUndoActionText(size_t length, const char *text) {
	cb.ChangeLastUndoActionText(length, text);
}

int Document::GetMark(Sci::Line line, bool includeChangeHistory) const {
	int marksHistory = 0;
	if (includeChangeHistory && (line < LinesTotal())) {
		int marksEdition = 0;

		const Sci::Position start = LineStart(line);
		const Sci::Position lineNext = LineStart(line + 1);
		for (Sci::Position position = start; position < lineNext;) {
			const int edition = EditionAt(position);
			if (edition) {
				marksEdition |= 1 << (edition-1);
			}
			position = EditionEndRun(position);
		}
		const Sci::Position lineEnd = LineEnd(line);
		for (Sci::Position position = start; position <= lineEnd;) {
			marksEdition |= EditionDeletesAt(position);
			position = EditionNextDelete(position);
		}

		/* Bits: RevertedToOrigin, Saved, Modified, RevertedToModified */
		constexpr unsigned int editionShift = static_cast<unsigned int>(MarkerOutline::HistoryRevertedToOrigin);
		marksHistory = marksEdition << editionShift;
	}

	return marksHistory | Markers()->MarkValue(line);
}

Sci::Line Document::MarkerNext(Sci::Line lineStart, int mask) const noexcept {
	return Markers()->MarkerNext(lineStart, mask);
}

int Document::AddMark(Sci::Line line, int markerNum) {
	if (line >= 0 && line < LinesTotal()) {
		const int prev = Markers()->AddMark(line, markerNum, LinesTotal());
		const DocModification mh(ModificationFlags::ChangeMarker, LineStart(line), 0, 0, nullptr, line);
		NotifyModified(mh);
		return prev;
	} else {
		return -1;
	}
}

void Document::AddMarkSet(Sci::Line line, int valueSet) {
	if (line < 0 || line >= LinesTotal()) {
		return;
	}
	unsigned int m = valueSet;
	for (int i = 0; m; i++, m >>= 1) {
		if (m & 1)
			Markers()->AddMark(line, i, LinesTotal());
	}
	const DocModification mh(ModificationFlags::ChangeMarker, LineStart(line), 0, 0, nullptr, line);
	NotifyModified(mh);
}

void Document::DeleteMark(Sci::Line line, int markerNum) {
	Markers()->DeleteMark(line, markerNum, false);
	const DocModification mh(ModificationFlags::ChangeMarker, LineStart(line), 0, 0, nullptr, line);
	NotifyModified(mh);
}

void Document::DeleteMarkFromHandle(int markerHandle) {
	Markers()->DeleteMarkFromHandle(markerHandle);
	DocModification mh(ModificationFlags::ChangeMarker);
	mh.line = -1;
	NotifyModified(mh);
}

void Document::DeleteAllMarks(int markerNum) {
	bool someChanges = false;
	for (Sci::Line line = 0; line < LinesTotal(); line++) {
		if (Markers()->DeleteMark(line, markerNum, true))
			someChanges = true;
	}
	if (someChanges) {
		DocModification mh(ModificationFlags::ChangeMarker);
		mh.line = -1;
		NotifyModified(mh);
	}
}

Sci::Line Document::LineFromHandle(int markerHandle) const noexcept {
	return Markers()->LineFromHandle(markerHandle);
}

int Document::MarkerNumberFromLine(Sci::Line line, int which) const noexcept {
	return Markers()->NumberFromLine(line, which);
}

int Document::MarkerHandleFromLine(Sci::Line line, int which) const noexcept {
	return Markers()->HandleFromLine(line, which);
}

Sci_Position SCI_METHOD Document::LineStart(Sci_Position line) const {
	return cb.LineStart(line);
}

Range Document::LineRange(Sci::Line line) const noexcept {
	return {cb.LineStart(line), cb.LineStart(line + 1)};
}

bool Document::IsLineStartPosition(Sci::Position position) const noexcept {
	return LineStartPosition(position) == position;
}

Sci_Position SCI_METHOD Document::LineEnd(Sci_Position line) const {
	return cb.LineEnd(line);
}

int SCI_METHOD Document::DEVersion() const noexcept {
	return deRelease0;
}

void SCI_METHOD Document::SetErrorStatus(int status) {
	// Tell the watchers an error has occurred.
	for (const WatcherWithUserData &watcher : watchers) {
		watcher.watcher->NotifyErrorOccurred(this, watcher.userData, static_cast<Status>(status));
	}
}

Sci_Position SCI_METHOD Document::LineFromPosition(Sci_Position pos) const {
	return cb.LineFromPosition(pos);
}

Sci::Line Document::SciLineFromPosition(Sci::Position pos) const noexcept {
	// Avoids casting in callers for this very common function
	return cb.LineFromPosition(pos);
}

Sci::Position Document::LineStartPosition(Sci::Position position) const noexcept {
	return cb.LineStart(cb.LineFromPosition(position));
}

Sci::Position Document::LineEndPosition(Sci::Position position) const noexcept {
	return cb.LineEnd(cb.LineFromPosition(position));
}

bool Document::IsLineEndPosition(Sci::Position position) const noexcept {
	return LineEndPosition(position) == position;
}

bool Document::IsPositionInLineEnd(Sci::Position position) const noexcept {
	return position >= LineEndPosition(position);
}

Sci::Position Document::VCHomePosition(Sci::Position position) const {
	const Sci::Line line = SciLineFromPosition(position);
	const Sci::Position startPosition = LineStart(line);
	const Sci::Position endLine = LineEnd(line);
	Sci::Position startText = startPosition;
	while (startText < endLine && IsSpaceOrTab(cb.CharAt(startText)))
		startText++;
	if (position == startText)
		return startPosition;
	else
		return startText;
}

Sci::Position Document::IndexLineStart(Sci::Line line, LineCharacterIndexType lineCharacterIndex) const noexcept {
	return cb.IndexLineStart(line, lineCharacterIndex);
}

Sci::Line Document::LineFromPositionIndex(Sci::Position pos, LineCharacterIndexType lineCharacterIndex) const noexcept {
	return cb.LineFromPositionIndex(pos, lineCharacterIndex);
}

Sci::Line Document::LineFromPositionAfter(Sci::Line line, Sci::Position length) const noexcept {
	const Sci::Position posAfter = cb.LineStart(line) + length;
	if (posAfter >= LengthNoExcept()) {
		return LinesTotal();
	}
	const Sci::Line lineAfter = SciLineFromPosition(posAfter);
	if (lineAfter > line) {
		return lineAfter;
	} else {
		// Want to make some progress so return next line
		return lineAfter + 1;
	}
}

int SCI_METHOD Document::SetLevel(Sci_Position line, int level) {
	const int prev = Levels()->SetLevel(line, level, LinesTotal());
	if (prev != level) {
		DocModification mh(ModificationFlags::ChangeFold | ModificationFlags::ChangeMarker,
		                   LineStart(line), 0, 0, nullptr, line);
		mh.foldLevelNow = static_cast<FoldLevel>(level);
		mh.foldLevelPrev = static_cast<FoldLevel>(prev);
		NotifyModified(mh);
	}
	return prev;
}

int SCI_METHOD Document::GetLevel(Sci_Position line) const {
	return Levels()->GetLevel(line);
}

FoldLevel Document::GetFoldLevel(Sci_Position line) const noexcept {
	return Levels()->GetFoldLevel(line);
}

void Document::ClearLevels() {
	Levels()->ClearLevels();
}

static bool IsSubordinate(FoldLevel levelStart, FoldLevel levelTry) noexcept {
	if (LevelIsWhitespace(levelTry))
		return true;
	else
		return LevelNumber(levelStart) < LevelNumber(levelTry);
}

Sci::Line Document::GetLastChild(Sci::Line lineParent, std::optional<FoldLevel> level, Sci::Line lastLine) {
	const FoldLevel levelStart = LevelNumberPart(level ? *level : GetFoldLevel(lineParent));
	const Sci::Line maxLine = LinesTotal();
	const Sci::Line lookLastLine = (lastLine != -1) ? std::min(LinesTotal() - 1, lastLine) : -1;
	Sci::Line lineMaxSubord = lineParent;
	while (lineMaxSubord < maxLine - 1) {
		EnsureStyledTo(LineStart(lineMaxSubord + 2));
		if (!IsSubordinate(levelStart, GetFoldLevel(lineMaxSubord + 1)))
			break;
		if ((lookLastLine != -1) && (lineMaxSubord >= lookLastLine) && !LevelIsWhitespace(GetFoldLevel(lineMaxSubord)))
			break;
		lineMaxSubord++;
	}
	if (lineMaxSubord > lineParent) {
		if (levelStart > LevelNumberPart(GetFoldLevel(lineMaxSubord + 1))) {
			// Have chewed up some whitespace that belongs to a parent so seek back
			if (LevelIsWhitespace(GetFoldLevel(lineMaxSubord))) {
				lineMaxSubord--;
			}
		}
	}
	return lineMaxSubord;
}

Sci::Line Document::GetFoldParent(Sci::Line line) const noexcept {
	return Levels()->GetFoldParent(line);
}

void Document::GetHighlightDelimiters(HighlightDelimiter &highlightDelimiter, Sci::Line line, Sci::Line lastLine) {
	const FoldLevel level = GetFoldLevel(line);
	const Sci::Line lookLastLine = std::max(line, lastLine) + 1;

	Sci::Line lookLine = line;
	FoldLevel lookLineLevel = level;
	FoldLevel lookLineLevelNum = LevelNumberPart(lookLineLevel);
	while ((lookLine > 0) && (LevelIsWhitespace(lookLineLevel) ||
		(LevelIsHeader(lookLineLevel) && (lookLineLevelNum >= LevelNumberPart(GetFoldLevel(lookLine + 1)))))) {
		lookLineLevel = GetFoldLevel(--lookLine);
		lookLineLevelNum = LevelNumberPart(lookLineLevel);
	}

	Sci::Line beginFoldBlock = LevelIsHeader(lookLineLevel) ? lookLine : GetFoldParent(lookLine);
	if (beginFoldBlock == -1) {
		highlightDelimiter.Clear();
		return;
	}

	Sci::Line endFoldBlock = GetLastChild(beginFoldBlock, {}, lookLastLine);
	Sci::Line firstChangeableLineBefore = -1;
	if (endFoldBlock < line) {
		lookLine = beginFoldBlock - 1;
		lookLineLevel = GetFoldLevel(lookLine);
		lookLineLevelNum = LevelNumberPart(lookLineLevel);
		while ((lookLine >= 0) && (lookLineLevelNum >= FoldLevel::Base)) {
			if (LevelIsHeader(lookLineLevel)) {
				if (GetLastChild(lookLine, {}, lookLastLine) == line) {
					beginFoldBlock = lookLine;
					endFoldBlock = line;
					firstChangeableLineBefore = line - 1;
				}
			}
			if ((lookLine > 0) && (lookLineLevelNum == FoldLevel::Base) && (LevelNumberPart(GetFoldLevel(lookLine - 1)) > lookLineLevelNum))
				break;
			lookLineLevel = GetFoldLevel(--lookLine);
			lookLineLevelNum = LevelNumberPart(lookLineLevel);
		}
	}
	if (firstChangeableLineBefore == -1) {
		for (lookLine = line - 1, lookLineLevel = GetFoldLevel(lookLine), lookLineLevelNum = LevelNumberPart(lookLineLevel);
			lookLine >= beginFoldBlock;
			lookLineLevel = GetFoldLevel(--lookLine), lookLineLevelNum = LevelNumberPart(lookLineLevel)) {
			if (LevelIsWhitespace(lookLineLevel) || (lookLineLevelNum > LevelNumberPart(level))) {
				firstChangeableLineBefore = lookLine;
				break;
			}
		}
	}
	if (firstChangeableLineBefore == -1)
		firstChangeableLineBefore = beginFoldBlock - 1;

	Sci::Line firstChangeableLineAfter = -1;
	for (lookLine = line + 1, lookLineLevel = GetFoldLevel(lookLine), lookLineLevelNum = LevelNumberPart(lookLineLevel);
		lookLine <= endFoldBlock;
		lookLineLevel = GetFoldLevel(++lookLine), lookLineLevelNum = LevelNumberPart(lookLineLevel)) {
		if (LevelIsHeader(lookLineLevel) && (lookLineLevelNum < LevelNumberPart(GetFoldLevel(lookLine + 1)))) {
			firstChangeableLineAfter = lookLine;
			break;
		}
	}
	if (firstChangeableLineAfter == -1)
		firstChangeableLineAfter = endFoldBlock + 1;

	highlightDelimiter.beginFoldBlock = beginFoldBlock;
	highlightDelimiter.endFoldBlock = endFoldBlock;
	highlightDelimiter.firstChangeableLineBefore = firstChangeableLineBefore;
	highlightDelimiter.firstChangeableLineAfter = firstChangeableLineAfter;
}

Sci::Position Document::ClampPositionIntoDocument(Sci::Position pos) const noexcept {
	return std::clamp<Sci::Position>(pos, 0, LengthNoExcept());
}

bool Document::IsCrLf(Sci::Position pos) const noexcept {
	if (pos < 0)
		return false;
	if (pos >= (LengthNoExcept() - 1))
		return false;
	return (cb.CharAt(pos) == '\r') && (cb.CharAt(pos + 1) == '\n');
}

int Document::LenChar(Sci::Position pos) const noexcept {
	if (pos < 0 || pos >= LengthNoExcept()) {
		// Returning 1 instead of 0 to defend against hanging with a loop that goes (or starts) out of bounds.
		return 1;
	} else if (IsCrLf(pos)) {
		return 2;
	}

	const unsigned char leadByte = cb.UCharAt(pos);
	if (!dbcsCodePage || UTF8IsAscii(leadByte)) {
		// Common case: ASCII character
		return 1;
	}
	if (CpUtf8 == dbcsCodePage) {
		const int widthCharBytes = UTF8BytesOfLead[leadByte];
		unsigned char charBytes[UTF8MaxBytes] = { leadByte, 0, 0, 0 };
		for (int b = 1; b < widthCharBytes; b++) {
			charBytes[b] = cb.UCharAt(pos + b);
		}
		const int utf8status = UTF8Classify(charBytes, widthCharBytes);
		if (utf8status & UTF8MaskInvalid) {
			// Treat as invalid and use up just one byte
			return 1;
		} else {
			return utf8status & UTF8MaskWidth;
		}
	} else {
		if (IsDBCSLeadByteNoExcept(leadByte) && IsDBCSTrailByteNoExcept(cb.CharAt(pos + 1))) {
			return 2;
		} else {
			return 1;
		}
	}
}

bool Document::InGoodUTF8(Sci::Position pos, Sci::Position &start, Sci::Position &end) const noexcept {
	Sci::Position trail = pos;
	while ((trail>0) && (pos-trail < UTF8MaxBytes) && UTF8IsTrailByte(cb.UCharAt(trail-1)))
		trail--;
	start = (trail > 0) ? trail-1 : trail;

	const unsigned char leadByte = cb.UCharAt(start);
	const int widthCharBytes = UTF8BytesOfLead[leadByte];
	if (widthCharBytes == 1) {
		return false;
	} else {
		const int trailBytes = widthCharBytes - 1;
		const Sci::Position len = pos - start;
		if (len > trailBytes)
			// pos too far from lead
			return false;
		unsigned char charBytes[UTF8MaxBytes] = {leadByte,0,0,0};
		for (Sci::Position b=1; b<widthCharBytes && ((start+b) < cb.Length()); b++)
			charBytes[b] = cb.CharAt(start+b);
		const int utf8status = UTF8Classify(charBytes, widthCharBytes);
		if (utf8status & UTF8MaskInvalid)
			return false;
		end = start + widthCharBytes;
		return true;
	}
}

// Normalise a position so that it is not part way through a multi-byte character.
// This can occur in two situations -
// When lines are terminated with \r\n pairs which should be treated as one character.
// When displaying DBCS text such as Japanese.
// If moving, move the position in the indicated direction.
Sci::Position Document::MovePositionOutsideChar(Sci::Position pos, Sci::Position moveDir, bool checkLineEnd) const noexcept {
	//Platform::DebugPrintf("NoCRLF %d %d\n", pos, moveDir);
	// If out of range, just return minimum/maximum value.
	if (pos <= 0)
		return 0;
	if (pos >= LengthNoExcept())
		return LengthNoExcept();

	// PLATFORM_ASSERT(pos > 0 && pos < LengthNoExcept());
	if (checkLineEnd && IsCrLf(pos - 1)) {
		if (moveDir > 0)
			return pos + 1;
		else
			return pos - 1;
	}

	if (dbcsCodePage) {
		if (CpUtf8 == dbcsCodePage) {
			const unsigned char ch = cb.UCharAt(pos);
			// If ch is not a trail byte then pos is valid intercharacter position
			if (UTF8IsTrailByte(ch)) {
				Sci::Position startUTF = pos;
				Sci::Position endUTF = pos;
				if (InGoodUTF8(pos, startUTF, endUTF)) {
					// ch is a trail byte within a UTF-8 character
					if (moveDir > 0)
						pos = endUTF;
					else
						pos = startUTF;
				}
				// Else invalid UTF-8 so return position of isolated trail byte
			}
		} else {
			// Step back until a non-lead-byte is found.
			Sci::Position posCheck = pos;
			while ((posCheck > 0) && IsDBCSLeadByteNoExcept(cb.CharAt(posCheck-1)))
				posCheck--;

			// Check from known start of character.
			while (posCheck < pos) {
				const int mbsize = IsDBCSDualByteAt(posCheck) ? 2 : 1;
				if (posCheck + mbsize == pos) {
					return pos;
				} else if (posCheck + mbsize > pos) {
					if (moveDir > 0) {
						return posCheck + mbsize;
					} else {
						return posCheck;
					}
				}
				posCheck += mbsize;
			}
		}
	}

	return pos;
}

// NextPosition moves between valid positions - it can not handle a position in the middle of a
// multi-byte character. It is used to iterate through text more efficiently than MovePositionOutsideChar.
// A \r\n pair is treated as two characters.
Sci::Position Document::NextPosition(Sci::Position pos, int moveDir) const noexcept {
	// If out of range, just return minimum/maximum value.
	const int increment = (moveDir > 0) ? 1 : -1;
	if (pos + increment <= 0)
		return 0;
	if (pos + increment >= cb.Length())
		return cb.Length();

	if (dbcsCodePage) {
		if (CpUtf8 == dbcsCodePage) {
			if (increment == 1) {
				// Simple forward movement case so can avoid some checks
				const unsigned char leadByte = cb.UCharAt(pos);
				if (UTF8IsAscii(leadByte)) {
					// Single byte character or invalid
					pos++;
				} else {
					const int widthCharBytes = UTF8BytesOfLead[leadByte];
					unsigned char charBytes[UTF8MaxBytes] = {leadByte,0,0,0};
					for (int b=1; b<widthCharBytes; b++)
						charBytes[b] = cb.CharAt(pos+b);
					const int utf8status = UTF8Classify(charBytes, widthCharBytes);
					if (utf8status & UTF8MaskInvalid)
						pos++;
					else
						pos += utf8status & UTF8MaskWidth;
				}
			} else {
				// Examine byte before position
				pos--;
				const unsigned char ch = cb.UCharAt(pos);
				// If ch is not a trail byte then pos is valid intercharacter position
				if (UTF8IsTrailByte(ch)) {
					// If ch is a trail byte in a valid UTF-8 character then return start of character
					Sci::Position startUTF = pos;
					Sci::Position endUTF = pos;
					if (InGoodUTF8(pos, startUTF, endUTF)) {
						pos = startUTF;
					}
					// Else invalid UTF-8 so return position of isolated trail byte
				}
			}
		} else {
			if (moveDir > 0) {
				const int mbsize = IsDBCSDualByteAt(pos) ? 2 : 1;
				pos += mbsize;
				if (pos > cb.Length())
					pos = cb.Length();
			} else {
				// How to Go Backward in a DBCS String
				// https://msdn.microsoft.com/en-us/library/cc194792.aspx
				// DBCS-Enabled Programs vs. Non-DBCS-Enabled Programs
				// https://msdn.microsoft.com/en-us/library/cc194790.aspx
				if (IsDBCSLeadByteNoExcept(cb.CharAt(pos - 1))) {
					// Should actually be trail byte
					if (IsDBCSDualByteAt(pos - 2)) {
						return pos - 2;
					} else {
						// Invalid byte pair so treat as one byte wide
						return pos - 1;
					}
				} else {
					// Otherwise, step back until a non-lead-byte is found.
					Sci::Position posTemp = pos - 1;
					while (--posTemp >= 0 && IsDBCSLeadByteNoExcept(cb.CharAt(posTemp)))
						;
					// Now posTemp+1 must point to the beginning of a character,
					// so figure out whether we went back an even or an odd
					// number of bytes and go back 1 or 2 bytes, respectively.
					const Sci::Position widthLast = ((pos - posTemp) & 1) + 1;
					if ((widthLast == 2) && (IsDBCSDualByteAt(pos - widthLast))) {
						return pos - widthLast;
					}
					// Byte before pos may be valid character or may be an invalid second byte
					return pos - 1;
				}
			}
		}
	} else {
		pos += increment;
	}

	return pos;
}

bool Document::NextCharacter(Sci::Position &pos, int moveDir) const noexcept {
	// Returns true if pos changed
	Sci::Position posNext = NextPosition(pos, moveDir);
	if (posNext == pos) {
		return false;
	} else {
		pos = posNext;
		return true;
	}
}

CharacterExtracted Document::CharacterAfter(Sci::Position position) const noexcept {
	if (position >= LengthNoExcept()) {
		return CharacterExtracted(unicodeReplacementChar, 0);
	}
	const unsigned char leadByte = cb.UCharAt(position);
	if (!dbcsCodePage || UTF8IsAscii(leadByte)) {
		// Common case: ASCII character
		return CharacterExtracted(leadByte, 1);
	}
	if (CpUtf8 == dbcsCodePage) {
		const int widthCharBytes = UTF8BytesOfLead[leadByte];
		unsigned char charBytes[UTF8MaxBytes] = { leadByte, 0, 0, 0 };
		for (int b = 1; b<widthCharBytes; b++)
			charBytes[b] = cb.UCharAt(position + b);
		return CharacterExtracted(charBytes, widthCharBytes);
	} else {
		if (IsDBCSLeadByteNoExcept(leadByte)) {
			const unsigned char trailByte = cb.UCharAt(position + 1);
			if (IsDBCSTrailByteNoExcept(trailByte)) {
				return CharacterExtracted::DBCS(leadByte, trailByte);
			}
		}
		return CharacterExtracted(leadByte, 1);
	}
}

CharacterExtracted Document::CharacterBefore(Sci::Position position) const noexcept {
	if (position <= 0) {
		return CharacterExtracted(unicodeReplacementChar, 0);
	}
	const unsigned char previousByte = cb.UCharAt(position - 1);
	if (0 == dbcsCodePage) {
		return CharacterExtracted(previousByte, 1);
	}
	if (CpUtf8 == dbcsCodePage) {
		if (UTF8IsAscii(previousByte)) {
			return CharacterExtracted(previousByte, 1);
		}
		position--;
		// If previousByte is not a trail byte then its invalid
		if (UTF8IsTrailByte(previousByte)) {
			// If previousByte is a trail byte in a valid UTF-8 character then find start of character
			Sci::Position startUTF = position;
			Sci::Position endUTF = position;
			if (InGoodUTF8(position, startUTF, endUTF)) {
				const Sci::Position widthCharBytes = endUTF - startUTF;
				unsigned char charBytes[UTF8MaxBytes] = { 0, 0, 0, 0 };
				for (Sci::Position b = 0; b<widthCharBytes; b++)
					charBytes[b] = cb.UCharAt(startUTF + b);
				return CharacterExtracted(charBytes, widthCharBytes);
			}
			// Else invalid UTF-8 so return position of isolated trail byte
		}
		return CharacterExtracted(unicodeReplacementChar, 1);
	} else {
		// Moving backwards in DBCS is complex so use NextPosition
		const Sci::Position posStartCharacter = NextPosition(position, -1);
		return CharacterAfter(posStartCharacter);
	}
}

// Return -1  on out-of-bounds
Sci_Position SCI_METHOD Document::GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const {
	Sci::Position pos = positionStart;
	if (dbcsCodePage) {
		const int increment = (characterOffset > 0) ? 1 : -1;
		while (characterOffset != 0) {
			const Sci::Position posNext = NextPosition(pos, increment);
			if (posNext == pos)
				return Sci::invalidPosition;
			pos = posNext;
			characterOffset -= increment;
		}
	} else {
		pos = positionStart + characterOffset;
		if ((pos < 0) || (pos > Length()))
			return Sci::invalidPosition;
	}
	return pos;
}

Sci::Position Document::GetRelativePositionUTF16(Sci::Position positionStart, Sci::Position characterOffset) const noexcept {
	Sci::Position pos = positionStart;
	if (dbcsCodePage) {
		const int increment = (characterOffset > 0) ? 1 : -1;
		while (characterOffset != 0) {
			const Sci::Position posNext = NextPosition(pos, increment);
			if (posNext == pos)
				return Sci::invalidPosition;
			if (std::abs(pos-posNext) > 3)	// 4 byte character = 2*UTF16.
				characterOffset -= increment;
			pos = posNext;
			characterOffset -= increment;
		}
	} else {
		pos = positionStart + characterOffset;
		if ((pos < 0) || (pos > LengthNoExcept()))
			return Sci::invalidPosition;
	}
	return pos;
}

int SCI_METHOD Document::GetCharacterAndWidth(Sci_Position position, Sci_Position *pWidth) const {
	int bytesInCharacter = 1;
	const unsigned char leadByte = cb.UCharAt(position);
	int character = leadByte;
	if (dbcsCodePage && !UTF8IsAscii(leadByte)) {
		if (CpUtf8 == dbcsCodePage) {
			const int widthCharBytes = UTF8BytesOfLead[leadByte];
			unsigned char charBytes[UTF8MaxBytes] = {leadByte,0,0,0};
			for (int b=1; b<widthCharBytes; b++)
				charBytes[b] = cb.UCharAt(position+b);
			const int utf8status = UTF8Classify(charBytes, widthCharBytes);
			if (utf8status & UTF8MaskInvalid) {
				// Report as singleton surrogate values which are invalid Unicode
				character =  0xDC80 + leadByte;
			} else {
				bytesInCharacter = utf8status & UTF8MaskWidth;
				character = UnicodeFromUTF8(charBytes);
			}
		} else {
			if (IsDBCSLeadByteNoExcept(leadByte)) {
				const unsigned char trailByte = cb.UCharAt(position + 1);
				if (IsDBCSTrailByteNoExcept(trailByte)) {
					bytesInCharacter = 2;
					character = (leadByte << 8) | trailByte;
				}
			}
		}
	}
	if (pWidth) {
		*pWidth = bytesInCharacter;
	}
	return character;
}

int SCI_METHOD Document::CodePage() const {
	return dbcsCodePage;
}

bool SCI_METHOD Document::IsDBCSLeadByte(char ch) const {
	// Used by lexers so must match IDocument method exactly
	return IsDBCSLeadByteNoExcept(ch);
}

bool Document::IsDBCSLeadByteNoExcept(char ch) const noexcept {
	// Used inside core Scintilla
	// Byte ranges found in Wikipedia articles with relevant search strings in each case
	const unsigned char uch = ch;
	switch (dbcsCodePage) {
		case 932:
			// Shift_jis
			return ((uch >= 0x81) && (uch <= 0x9F)) ||
				((uch >= 0xE0) && (uch <= 0xFC));
				// Lead bytes F0 to FC may be a Microsoft addition.
		case 936:
			// GBK
			return (uch >= 0x81) && (uch <= 0xFE);
		case 949:
			// Korean Wansung KS C-5601-1987
			return (uch >= 0x81) && (uch <= 0xFE);
		case 950:
			// Big5
			return (uch >= 0x81) && (uch <= 0xFE);
		case 1361:
			// Korean Johab KS C-5601-1992
			return
				((uch >= 0x84) && (uch <= 0xD3)) ||
				((uch >= 0xD8) && (uch <= 0xDE)) ||
				((uch >= 0xE0) && (uch <= 0xF9));
	}
	return false;
}

bool Document::IsDBCSTrailByteNoExcept(char ch) const noexcept {
	const unsigned char trail = ch;
	switch (dbcsCodePage) {
	case 932:
		// Shift_jis
		return (trail != 0x7F) &&
			((trail >= 0x40) && (trail <= 0xFC));
	case 936:
		// GBK
		return (trail != 0x7F) &&
			((trail >= 0x40) && (trail <= 0xFE));
	case 949:
		// Korean Wansung KS C-5601-1987
		return
			((trail >= 0x41) && (trail <= 0x5A)) ||
			((trail >= 0x61) && (trail <= 0x7A)) ||
			((trail >= 0x81) && (trail <= 0xFE));
	case 950:
		// Big5
		return
			((trail >= 0x40) && (trail <= 0x7E)) ||
			((trail >= 0xA1) && (trail <= 0xFE));
	case 1361:
		// Korean Johab KS C-5601-1992
		return
			((trail >= 0x31) && (trail <= 0x7E)) ||
			((trail >= 0x81) && (trail <= 0xFE));
	}
	return false;
}

unsigned char Document::DBCSMinTrailByte() const noexcept {
	switch (dbcsCodePage) {
	case 932:
		// Shift_jis
		return 0x40;
	case 936:
		// GBK
		return 0x40;
	case 949:
		// Korean Wansung KS C-5601-1987
		return 0x41;
	case 950:
		// Big5
		return 0x40;
	case 1361:
		// Korean Johab KS C-5601-1992
		return 0x31;
	default:
		// UTF-8 or single byte, should not occur as not DBCS
		return 0;
	}
}

int Document::DBCSDrawBytes(std::string_view text) const noexcept {
	if (text.length() <= 1) {
		return static_cast<int>(text.length());
	}
	if (IsDBCSLeadByteNoExcept(text[0])) {
		return IsDBCSTrailByteNoExcept(text[1]) ? 2 : 1;
	} else {
		return 1;
	}
}

bool Document::IsDBCSDualByteAt(Sci::Position pos) const noexcept {
	return IsDBCSLeadByteNoExcept(cb.CharAt(pos))
		&& IsDBCSTrailByteNoExcept(cb.CharAt(pos + 1));
}

// Need to break text into segments near end but taking into account the
// encoding to not break inside a UTF-8 or DBCS character and also trying
// to avoid breaking inside a pair of combining characters, or inside
// ligatures.
// TODO: implement grapheme cluster boundaries,
// see https://www.unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries.
//
// The segment length must always be long enough (more than 4 bytes)
// so that there will be at least one whole character to make a segment.
// For UTF-8, text must consist only of valid whole characters.
// In preference order from best to worst:
//   1) Break before or after spaces or controls
//   2) Break at word and punctuation boundary for better kerning and ligature support
//   3) Break after whole character, this may break combining characters

size_t Document::SafeSegment(std::string_view text) const noexcept {
	// check space first as most written language use spaces.
	for (std::string_view::iterator it = text.end() - 1; it != text.begin(); --it) {
		if (IsBreakSpace(*it)) {
			return it - text.begin();
		}
	}

	if (!dbcsCodePage || dbcsCodePage == CpUtf8) {
		// backward iterate for UTF-8 and single byte encoding to find word and punctuation boundary.
		std::string_view::iterator it = text.end() - 1;
		const bool punctuation = IsPunctuation(*it);
		do {
			--it;
			if (punctuation != IsPunctuation(*it)) {
				return it - text.begin() + 1;
			}
		} while (it != text.begin());

		it = text.end() - 1;
		if (dbcsCodePage) {
			// for UTF-8 go back to the start of last character.
			for (int trail = 0; trail < UTF8MaxBytes - 1 && UTF8IsTrailByte(*it); trail++) {
				--it;
			}
		}
		return it - text.begin();
	}

	{
		// forward iterate for DBCS to find word and punctuation boundary.
		size_t lastPunctuationBreak = 0;
		size_t lastEncodingAllowedBreak = 0;
		CharacterClass ccPrev = CharacterClass::space;
		for (size_t j = 0; j < text.length();) {
			const unsigned char ch = text[j];
			lastEncodingAllowedBreak = j++;

			CharacterClass cc = CharacterClass::word;
			if (UTF8IsAscii(ch)) {
				if (IsPunctuation(ch)) {
					cc = CharacterClass::punctuation;
				}
			} else {
				j += IsDBCSLeadByteNoExcept(ch);
			}
			if (cc != ccPrev) {
				ccPrev = cc;
				lastPunctuationBreak = lastEncodingAllowedBreak;
			}
		}
		return lastPunctuationBreak ? lastPunctuationBreak : lastEncodingAllowedBreak;
	}
}

EncodingFamily Document::CodePageFamily() const noexcept {
	if (CpUtf8 == dbcsCodePage)
		return EncodingFamily::unicode;
	else if (dbcsCodePage)
		return EncodingFamily::dbcs;
	else
		return EncodingFamily::eightBit;
}

void Document::ModifiedAt(Sci::Position pos) noexcept {
	if (endStyled > pos)
		endStyled = pos;
}

void Document::CheckReadOnly() {
	if (cb.IsReadOnly() && enteredReadOnlyCount == 0) {
		enteredReadOnlyCount++;
		NotifyModifyAttempt();
		enteredReadOnlyCount--;
	}
}

void Document::TrimReplacement(std::string_view &text, Range &range) const noexcept {
	while (!text.empty() && !range.Empty() && (text.front() == CharAt(range.start))) {
		text.remove_prefix(1);
		range.start++;
	}
	while (!text.empty() && !range.Empty() && (text.back() == CharAt(range.end-1))) {
		text.remove_suffix(1);
		range.end--;
	}
}

// Document only modified by gateways DeleteChars, InsertString, Undo, Redo, and SetStyleAt.
// SetStyleAt does not change the persistent state of a document

bool Document::DeleteChars(Sci::Position pos, Sci::Position len) {
	if (pos < 0)
		return false;
	if (len <= 0)
		return false;
	if ((pos + len) > LengthNoExcept())
		return false;
	CheckReadOnly();
	if (enteredModification != 0) {
		return false;
	} else {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			NotifyModified(
			    DocModification(
			        ModificationFlags::BeforeDelete | ModificationFlags::User,
			        pos, len,
				0, nullptr));
			const Sci::Line prevLinesTotal = LinesTotal();
			const bool startSavePoint = cb.IsSavePoint();
			bool startSequence = false;
			const char *text = cb.DeleteChars(pos, len, startSequence);
			if (startSavePoint && cb.IsCollectingUndo())
				NotifySavePoint(false);
			if ((pos < LengthNoExcept()) || (pos == 0))
				ModifiedAt(pos);
			else
				ModifiedAt(pos-1);
			NotifyModified(
			    DocModification(
			        ModificationFlags::DeleteText | ModificationFlags::User |
					(startSequence?ModificationFlags::StartAction:ModificationFlags::None),
			        pos, len,
			        LinesTotal() - prevLinesTotal, text));
		}
		enteredModification--;
	}
	return !cb.IsReadOnly();
}

/**
 * Insert a string with a length.
 */
Sci::Position Document::InsertString(Sci::Position position, const char *s, Sci::Position insertLength) {
	if (insertLength <= 0) {
		return 0;
	}
	CheckReadOnly();	// Application may change read only state here
	if (cb.IsReadOnly()) {
		return 0;
	}
	if (enteredModification != 0) {
		return 0;
	}
	enteredModification++;
	insertionSet = false;
	insertion.clear();
	NotifyModified(
		DocModification(
			ModificationFlags::InsertCheck,
			position, insertLength,
			0, s));
	if (insertionSet) {
		s = insertion.c_str();
		insertLength = insertion.length();
	}
	NotifyModified(
		DocModification(
			ModificationFlags::BeforeInsert | ModificationFlags::User,
			position, insertLength,
			0, s));
	const Sci::Line prevLinesTotal = LinesTotal();
	const bool startSavePoint = cb.IsSavePoint();
	bool startSequence = false;
	const char *text = cb.InsertString(position, s, insertLength, startSequence);
	if (startSavePoint && cb.IsCollectingUndo())
		NotifySavePoint(false);
	ModifiedAt(position);
	NotifyModified(
		DocModification(
			ModificationFlags::InsertText | ModificationFlags::User |
			(startSequence?ModificationFlags::StartAction:ModificationFlags::None),
			position, insertLength,
			LinesTotal() - prevLinesTotal, text));
	if (insertionSet) {	// Free memory as could be large
		std::string().swap(insertion);
	}
	enteredModification--;
	return insertLength;
}

Sci::Position Document::InsertString(Sci::Position position, std::string_view sv) {
	return InsertString(position, sv.data(), sv.length());
}

void Document::ChangeInsertion(const char *s, Sci::Position length) {
	insertionSet = true;
	insertion.assign(s, length);
}

int SCI_METHOD Document::AddData(const char *data, Sci_Position length) {
	try {
		const Sci::Position position = Length();
		InsertString(position, data, length);
	} catch (std::bad_alloc &) {
		return static_cast<int>(Status::BadAlloc);
	} catch (...) {
		return static_cast<int>(Status::Failure);
	}
	return static_cast<int>(Status::Ok);
}

IDocumentEditable *Document::AsDocumentEditable() noexcept {
	return static_cast<IDocumentEditable *>(this);
}

void *SCI_METHOD Document::ConvertToDocument() {
	return AsDocumentEditable();
}

Sci::Position Document::Undo() {
	Sci::Position newPos = -1;
	CheckReadOnly();
	if ((enteredModification == 0) && (cb.IsCollectingUndo())) {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			const bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			const int steps = cb.StartUndo();
			//Platform::DebugPrintf("Steps=%d\n", steps);
			Range coalescedRemove;	// Default is empty at 0
			for (int step = 0; step < steps; step++) {
				const Sci::Line prevLinesTotal = LinesTotal();
				const Action action = cb.GetUndoStep();
				if (action.at == ActionType::remove) {
					NotifyModified(DocModification(
									ModificationFlags::BeforeInsert | ModificationFlags::Undo, action));
				} else if (action.at == ActionType::container) {
					DocModification dm(ModificationFlags::Container | ModificationFlags::Undo);
					dm.token = action.position;
					NotifyModified(dm);
				} else {
					NotifyModified(DocModification(
									ModificationFlags::BeforeDelete | ModificationFlags::Undo, action));
				}
				cb.PerformUndoStep();
				if (action.at != ActionType::container) {
					ModifiedAt(action.position);
					newPos = action.position;
				}

				ModificationFlags modFlags = ModificationFlags::Undo;
				// With undo, an insertion action becomes a deletion notification
				if (action.at == ActionType::remove) {
					newPos += action.lenData;
					modFlags |= ModificationFlags::InsertText;
					if (coalescedRemove.Contains(action.position)) {
						coalescedRemove.end += action.lenData;
						newPos = coalescedRemove.end;
					} else {
						coalescedRemove = Range(action.position, action.position + action.lenData);
					}
				} else if (action.at == ActionType::insert) {
					modFlags |= ModificationFlags::DeleteText;
					coalescedRemove = Range();
				}
				if (steps > 1)
					modFlags |= ModificationFlags::MultiStepUndoRedo;
				const Sci::Line linesAdded = LinesTotal() - prevLinesTotal;
				if (linesAdded != 0)
					multiLine = true;
				if (step == steps - 1) {
					modFlags |= ModificationFlags::LastStepInUndoRedo;
					if (multiLine)
						modFlags |= ModificationFlags::MultilineUndoRedo;
				}
				NotifyModified(DocModification(modFlags, action.position, action.lenData,
											   linesAdded, action.data));
			}

			const bool endSavePoint = cb.IsSavePoint();
			if (startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);
		}
		enteredModification--;
	}
	return newPos;
}

Sci::Position Document::Redo() {
	Sci::Position newPos = -1;
	CheckReadOnly();
	if ((enteredModification == 0) && (cb.IsCollectingUndo())) {
		enteredModification++;
		if (!cb.IsReadOnly()) {
			const bool startSavePoint = cb.IsSavePoint();
			bool multiLine = false;
			const int steps = cb.StartRedo();
			for (int step = 0; step < steps; step++) {
				const Sci::Line prevLinesTotal = LinesTotal();
				const Action action = cb.GetRedoStep();
				if (action.at == ActionType::insert) {
					NotifyModified(DocModification(
									ModificationFlags::BeforeInsert | ModificationFlags::Redo, action));
				} else if (action.at == ActionType::container) {
					DocModification dm(ModificationFlags::Container | ModificationFlags::Redo);
					dm.token = action.position;
					NotifyModified(dm);
				} else {
					NotifyModified(DocModification(
									ModificationFlags::BeforeDelete | ModificationFlags::Redo, action));
				}
				cb.PerformRedoStep();
				if (action.at != ActionType::container) {
					ModifiedAt(action.position);
					newPos = action.position;
				}

				ModificationFlags modFlags = ModificationFlags::Redo;
				if (action.at == ActionType::insert) {
					newPos += action.lenData;
					modFlags |= ModificationFlags::InsertText;
				} else if (action.at == ActionType::remove) {
					modFlags |= ModificationFlags::DeleteText;
				}
				if (steps > 1)
					modFlags |= ModificationFlags::MultiStepUndoRedo;
				const Sci::Line linesAdded = LinesTotal() - prevLinesTotal;
				if (linesAdded != 0)
					multiLine = true;
				if (step == steps - 1) {
					modFlags |= ModificationFlags::LastStepInUndoRedo;
					if (multiLine)
						modFlags |= ModificationFlags::MultilineUndoRedo;
				}
				NotifyModified(
					DocModification(modFlags, action.position, action.lenData,
									linesAdded, action.data));
			}

			const bool endSavePoint = cb.IsSavePoint();
			if (startSavePoint != endSavePoint)
				NotifySavePoint(endSavePoint);
		}
		enteredModification--;
	}
	return newPos;
}

int Document::UndoSequenceDepth() const noexcept {
	return cb.UndoSequenceDepth();
}

void Document::DelChar(Sci::Position pos) {
	DeleteChars(pos, LenChar(pos));
}

void Document::DelCharBack(Sci::Position pos) {
	if (pos <= 0) {
		return;
	} else if (IsCrLf(pos - 2)) {
		DeleteChars(pos - 2, 2);
	} else if (dbcsCodePage) {
		const Sci::Position startChar = NextPosition(pos, -1);
		DeleteChars(startChar, pos - startChar);
	} else {
		DeleteChars(pos - 1, 1);
	}
}

static constexpr Sci::Position NextTab(Sci::Position pos, Sci::Position tabSize) noexcept {
	return ((pos / tabSize) + 1) * tabSize;
}

static std::string CreateIndentation(Sci::Position indent, int tabSize, bool insertSpaces) {
	std::string indentation;
	if (!insertSpaces) {
		while (indent >= tabSize) {
			indentation += '\t';
			indent -= tabSize;
		}
	}
	while (indent > 0) {
		indentation += ' ';
		indent--;
	}
	return indentation;
}

int SCI_METHOD Document::GetLineIndentation(Sci_Position line) {
	int indent = 0;
	if ((line >= 0) && (line < LinesTotal())) {
		const Sci::Position lineStart = LineStart(line);
		const Sci::Position length = Length();
		for (Sci::Position i = lineStart; i < length; i++) {
			const char ch = cb.CharAt(i);
			if (ch == ' ')
				indent++;
			else if (ch == '\t')
				indent = static_cast<int>(NextTab(indent, tabInChars));
			else
				return indent;
		}
	}
	return indent;
}

Sci::Position Document::SetLineIndentation(Sci::Line line, Sci::Position indent) {
	const int indentOfLine = GetLineIndentation(line);
	if (indent < 0)
		indent = 0;
	if (indent != indentOfLine) {
		const std::string linebuf = CreateIndentation(indent, tabInChars, !useTabs);
		const Sci::Position thisLineStart = LineStart(line);
		const Sci::Position indentPos = GetLineIndentPosition(line);
		UndoGroup ug(this);
		DeleteChars(thisLineStart, indentPos - thisLineStart);
		return thisLineStart + InsertString(thisLineStart, linebuf);
	} else {
		return GetLineIndentPosition(line);
	}
}

Sci::Position Document::GetLineIndentPosition(Sci::Line line) const {
	if (line < 0)
		return 0;
	Sci::Position pos = LineStart(line);
	const Sci::Position length = Length();
	while ((pos < length) && IsSpaceOrTab(cb.CharAt(pos))) {
		pos++;
	}
	return pos;
}

Sci::Position Document::GetColumn(Sci::Position pos) const {
	Sci::Position column = 0;
	const Sci::Line line = SciLineFromPosition(pos);
	if ((line >= 0) && (line < LinesTotal())) {
		for (Sci::Position i = LineStart(line); i < pos;) {
			const char ch = cb.CharAt(i);
			if (ch == '\t') {
				column = NextTab(column, tabInChars);
				i++;
			} else if (ch == '\r') {
				return column;
			} else if (ch == '\n') {
				return column;
			} else if (i >= Length()) {
				return column;
			} else if (UTF8IsAscii(ch)) {
				column++;
				i++;
			} else {
				column++;
				i = NextPosition(i, 1);
			}
		}
	}
	return column;
}

Sci::Position Document::CountCharacters(Sci::Position startPos, Sci::Position endPos) const noexcept {
	startPos = MovePositionOutsideChar(startPos, 1, false);
	endPos = MovePositionOutsideChar(endPos, -1, false);
	Sci::Position count = 0;
	Sci::Position i = startPos;
	while (i < endPos) {
		count++;
		i = NextPosition(i, 1);
	}
	return count;
}

Sci::Position Document::CountUTF16(Sci::Position startPos, Sci::Position endPos) const noexcept {
	startPos = MovePositionOutsideChar(startPos, 1, false);
	endPos = MovePositionOutsideChar(endPos, -1, false);
	Sci::Position count = 0;
	Sci::Position i = startPos;
	while (i < endPos) {
		count++;
		const Sci::Position next = NextPosition(i, 1);
		if ((next - i) > 3)
			count++;
		i = next;
	}
	return count;
}

Sci::Position Document::FindColumn(Sci::Line line, Sci::Position column) {
	Sci::Position position = LineStart(line);
	if ((line >= 0) && (line < LinesTotal())) {
		Sci::Position columnCurrent = 0;
		while ((columnCurrent < column) && (position < Length())) {
			const char ch = cb.CharAt(position);
			if (ch == '\t') {
				columnCurrent = NextTab(columnCurrent, tabInChars);
				if (columnCurrent > column)
					return position;
				position++;
			} else if (ch == '\r') {
				return position;
			} else if (ch == '\n') {
				return position;
			} else {
				columnCurrent++;
				position = NextPosition(position, 1);
			}
		}
	}
	return position;
}

void Document::Indent(bool forwards, Sci::Line lineBottom, Sci::Line lineTop) {
	// Dedent - suck white space off the front of the line to dedent by equivalent of a tab
	for (Sci::Line line = lineBottom; line >= lineTop; line--) {
		const Sci::Position indentOfLine = GetLineIndentation(line);
		if (forwards) {
			if (LineStart(line) < LineEnd(line)) {
				SetLineIndentation(line, indentOfLine + IndentSize());
			}
		} else {
			SetLineIndentation(line, indentOfLine - IndentSize());
		}
	}
}

namespace {

constexpr std::string_view EOLForMode(EndOfLine eolMode) noexcept {
	switch (eolMode) {
	case EndOfLine::CrLf:
		return "\r\n";
	case EndOfLine::Cr:
		return "\r";
	default:
		return "\n";
	}
}

}

// Convert line endings for a piece of text to a particular mode.
// Stop at len or when a NUL is found.
std::string Document::TransformLineEnds(const char *s, size_t len, EndOfLine eolModeWanted) {
	std::string dest;
	const std::string_view eol = EOLForMode(eolModeWanted);
	for (size_t i = 0; (i < len) && (s[i]); i++) {
		if (s[i] == '\n' || s[i] == '\r') {
			dest.append(eol);
			if ((s[i] == '\r') && (i+1 < len) && (s[i+1] == '\n')) {
				i++;
			}
		} else {
			dest.push_back(s[i]);
		}
	}
	return dest;
}

void Document::ConvertLineEnds(EndOfLine eolModeSet) {
	UndoGroup ug(this);

	for (Sci::Position pos = 0; pos < Length(); pos++) {
		const char ch = cb.CharAt(pos);
		if (ch == '\r') {
			if (cb.CharAt(pos + 1) == '\n') {
				// CRLF
				if (eolModeSet == EndOfLine::Cr) {
					DeleteChars(pos + 1, 1); // Delete the LF
				} else if (eolModeSet == EndOfLine::Lf) {
					DeleteChars(pos, 1); // Delete the CR
				} else {
					pos++;
				}
			} else {
				// CR
				if (eolModeSet == EndOfLine::CrLf) {
					pos += InsertString(pos + 1, "\n", 1); // Insert LF
				} else if (eolModeSet == EndOfLine::Lf) {
					pos += InsertString(pos, "\n", 1); // Insert LF
					DeleteChars(pos, 1); // Delete CR
					pos--;
				}
			}
		} else if (ch == '\n') {
			// LF
			if (eolModeSet == EndOfLine::CrLf) {
				pos += InsertString(pos, "\r", 1); // Insert CR
			} else if (eolModeSet == EndOfLine::Cr) {
				pos += InsertString(pos, "\r", 1); // Insert CR
				DeleteChars(pos, 1); // Delete LF
				pos--;
			}
		}
	}

}

std::string_view Document::EOLString() const noexcept {
	return EOLForMode(eolMode);
}

DocumentOption Document::Options() const noexcept {
	return (IsLarge() ? DocumentOption::TextLarge : DocumentOption::Default) |
		(cb.HasStyles() ? DocumentOption::Default : DocumentOption::StylesNone);
}

bool Document::IsWhiteLine(Sci::Line line) const {
	Sci::Position currentChar = LineStart(line);
	const Sci::Position endLine = LineEnd(line);
	while (currentChar < endLine) {
		if (!IsSpaceOrTab(cb.CharAt(currentChar))) {
			return false;
		}
		++currentChar;
	}
	return true;
}

Sci::Position Document::ParaUp(Sci::Position pos) const {
	Sci::Line line = SciLineFromPosition(pos);
	const Sci::Position start = LineStart(line);
	if (pos == start) {
		line--;
	}
	while (line >= 0 && IsWhiteLine(line)) { // skip empty lines
		line--;
	}
	while (line >= 0 && !IsWhiteLine(line)) { // skip non-empty lines
		line--;
	}
	line++;
	return LineStart(line);
}

Sci::Position Document::ParaDown(Sci::Position pos) const {
	Sci::Line line = SciLineFromPosition(pos);
	while (line < LinesTotal() && !IsWhiteLine(line)) { // skip non-empty lines
		line++;
	}
	while (line < LinesTotal() && IsWhiteLine(line)) { // skip empty lines
		line++;
	}
	if (line < LinesTotal())
		return LineStart(line);
	else // end of a document
		return LineEnd(line-1);
}

CharacterClass Document::WordCharacterClass(unsigned int ch) const {
	if (dbcsCodePage && (ch >= 0x80)) {
		if (CpUtf8 == dbcsCodePage) {
			// Use hard coded Unicode class
			const CharacterCategory cc = charMap.CategoryFor(ch);
			switch (cc) {

				// Separator, Line/Paragraph
			case ccZl:
			case ccZp:
				return CharacterClass::newLine;

				// Separator, Space
			case ccZs:
				// Other
			case ccCc:
			case ccCf:
			case ccCs:
			case ccCo:
			case ccCn:
				return CharacterClass::space;

				// Letter
			case ccLu:
			case ccLl:
			case ccLt:
			case ccLm:
			case ccLo:
				// Number
			case ccNd:
			case ccNl:
			case ccNo:
				// Mark - includes combining diacritics
			case ccMn:
			case ccMc:
			case ccMe:
				return CharacterClass::word;

				// Punctuation
			case ccPc:
			case ccPd:
			case ccPs:
			case ccPe:
			case ccPi:
			case ccPf:
			case ccPo:
				// Symbol
			case ccSm:
			case ccSc:
			case ccSk:
			case ccSo:
				return CharacterClass::punctuation;

			}
		} else {
			// Asian DBCS
			return CharacterClass::word;
		}
	}
	return charClass.GetClass(static_cast<unsigned char>(ch));
}

/**
 * Used by commands that want to select whole words.
 * Finds the start of word at pos when delta < 0 or the end of the word when delta >= 0.
 */
Sci::Position Document::ExtendWordSelect(Sci::Position pos, int delta, bool onlyWordCharacters) const {
	CharacterClass ccStart = CharacterClass::word;
	if (delta < 0) {
		if (!onlyWordCharacters) {
			const CharacterExtracted ce = CharacterBefore(pos);
			ccStart = WordCharacterClass(ce.character);
		}
		while (pos > 0) {
			const CharacterExtracted ce = CharacterBefore(pos);
			if (WordCharacterClass(ce.character) != ccStart)
				break;
			pos -= ce.widthBytes;
		}
	} else {
		if (!onlyWordCharacters && pos < LengthNoExcept()) {
			const CharacterExtracted ce = CharacterAfter(pos);
			ccStart = WordCharacterClass(ce.character);
		}
		while (pos < LengthNoExcept()) {
			const CharacterExtracted ce = CharacterAfter(pos);
			if (WordCharacterClass(ce.character) != ccStart)
				break;
			pos += ce.widthBytes;
		}
	}
	return MovePositionOutsideChar(pos, delta, true);
}

/**
 * Find the start of the next word in either a forward (delta >= 0) or backwards direction
 * (delta < 0).
 * This is looking for a transition between character classes although there is also some
 * additional movement to transit white space.
 * Used by cursor movement by word commands.
 */
Sci::Position Document::NextWordStart(Sci::Position pos, int delta) const {
	if (delta < 0) {
		while (pos > 0) {
			const CharacterExtracted ce = CharacterBefore(pos);
			if (WordCharacterClass(ce.character) != CharacterClass::space)
				break;
			pos -= ce.widthBytes;
		}
		if (pos > 0) {
			CharacterExtracted ce = CharacterBefore(pos);
			const CharacterClass ccStart = WordCharacterClass(ce.character);
			while (pos > 0) {
				ce = CharacterBefore(pos);
				if (WordCharacterClass(ce.character) != ccStart)
					break;
				pos -= ce.widthBytes;
			}
		}
	} else {
		CharacterExtracted ce = CharacterAfter(pos);
		const CharacterClass ccStart = WordCharacterClass(ce.character);
		while (pos < LengthNoExcept()) {
			ce = CharacterAfter(pos);
			if (WordCharacterClass(ce.character) != ccStart)
				break;
			pos += ce.widthBytes;
		}
		while (pos < LengthNoExcept()) {
			ce = CharacterAfter(pos);
			if (WordCharacterClass(ce.character) != CharacterClass::space)
				break;
			pos += ce.widthBytes;
		}
	}
	return pos;
}

/**
 * Find the end of the next word in either a forward (delta >= 0) or backwards direction
 * (delta < 0).
 * This is looking for a transition between character classes although there is also some
 * additional movement to transit white space.
 * Used by cursor movement by word commands.
 */
Sci::Position Document::NextWordEnd(Sci::Position pos, int delta) const {
	if (delta < 0) {
		if (pos > 0) {
			CharacterExtracted ce = CharacterBefore(pos);
			const CharacterClass ccStart = WordCharacterClass(ce.character);
			if (ccStart != CharacterClass::space) {
				while (pos > 0) {
					ce = CharacterBefore(pos);
					if (WordCharacterClass(ce.character) != ccStart)
						break;
					pos -= ce.widthBytes;
				}
			}
			while (pos > 0) {
				ce = CharacterBefore(pos);
				if (WordCharacterClass(ce.character) != CharacterClass::space)
					break;
				pos -= ce.widthBytes;
			}
		}
	} else {
		while (pos < LengthNoExcept()) {
			const CharacterExtracted ce = CharacterAfter(pos);
			if (WordCharacterClass(ce.character) != CharacterClass::space)
				break;
			pos += ce.widthBytes;
		}
		if (pos < LengthNoExcept()) {
			CharacterExtracted ce = CharacterAfter(pos);
			const CharacterClass ccStart = WordCharacterClass(ce.character);
			while (pos < LengthNoExcept()) {
				ce = CharacterAfter(pos);
				if (WordCharacterClass(ce.character) != ccStart)
					break;
				pos += ce.widthBytes;
			}
		}
	}
	return pos;
}

namespace {

constexpr bool IsWordEdge(CharacterClass cc, CharacterClass ccNext) noexcept {
	return (cc != ccNext) &&
		(cc == CharacterClass::word || cc == CharacterClass::punctuation);
}

}

/**
 * Check that the character at the given position is a word or punctuation character and that
 * the previous character is of a different character class.
 */
bool Document::IsWordStartAt(Sci::Position pos) const {
	if (pos >= LengthNoExcept())
		return false;
	if (pos >= 0) {
		const CharacterExtracted cePos = CharacterAfter(pos);
		// At start of document, treat as if space before so can be word start
		const CharacterExtracted cePrev = (pos > 0) ?
			CharacterBefore(pos) : CharacterExtracted(' ', 1);
		return IsWordEdge(WordCharacterClass(cePos.character), WordCharacterClass(cePrev.character));
	}
	return true;
}

/**
 * Check that the character before the given position is a word or punctuation character and that
 * the next character is of a different character class.
 */
bool Document::IsWordEndAt(Sci::Position pos) const {
	if (pos <= 0)
		return false;
	if (pos <= LengthNoExcept()) {
		// At end of document, treat as if space after so can be word end
		const CharacterExtracted cePos = (pos < LengthNoExcept()) ?
			CharacterAfter(pos) : CharacterExtracted(' ', 1);
		const CharacterExtracted cePrev = CharacterBefore(pos);
		return IsWordEdge(WordCharacterClass(cePrev.character), WordCharacterClass(cePos.character));
	}
	return true;
}

/**
 * Check that the given range is has transitions between character classes at both
 * ends and where the characters on the inside are word or punctuation characters.
 */
bool Document::IsWordAt(Sci::Position start, Sci::Position end) const {
	return (start < end) && IsWordStartAt(start) && IsWordEndAt(end);
}

bool Document::MatchesWordOptions(bool word, bool wordStart, Sci::Position pos, Sci::Position length) const {
	return (!word && !wordStart) ||
			(word && IsWordAt(pos, pos + length)) ||
			(wordStart && IsWordStartAt(pos));
}

bool Document::HasCaseFolder() const noexcept {
	return pcf != nullptr;
}

void Document::SetCaseFolder(std::unique_ptr<CaseFolder> pcf_) noexcept {
	pcf = std::move(pcf_);
}

CharacterExtracted Document::ExtractCharacter(Sci::Position position) const noexcept {
	const unsigned char leadByte = cb.UCharAt(position);
	if (UTF8IsAscii(leadByte)) {
		// Common case: ASCII character
		return CharacterExtracted(leadByte, 1);
	}
	const int widthCharBytes = UTF8BytesOfLead[leadByte];
	unsigned char charBytes[UTF8MaxBytes] = { leadByte, 0, 0, 0 };
	for (int b=1; b<widthCharBytes; b++)
		charBytes[b] = cb.UCharAt(position + b);
	return CharacterExtracted(charBytes, widthCharBytes);
}

namespace {

// Equivalent of memchr over the split view
ptrdiff_t SplitFindChar(const SplitView &view, size_t start, size_t length, int ch) noexcept {
	size_t range1Length = 0;
	if (start < view.length1) {
		range1Length = std::min(length, view.length1 - start);
		const char *match = static_cast<const char *>(memchr(view.segment1 + start, ch, range1Length));
		if (match) {
			return match - view.segment1;
		}
		start += range1Length;
	}
	const char *match2 = static_cast<const char *>(memchr(view.segment2 + start, ch, length - range1Length));
	if (match2) {
		return match2 - view.segment2;
	}
	return -1;
}

// Equivalent of memcmp over the split view
// This does not call memcmp as search texts are commonly too short to overcome the
// call overhead.
bool SplitMatch(const SplitView &view, size_t start, std::string_view text) noexcept {
	for (size_t i = 0; i < text.length(); i++) {
		if (view.CharAt(i + start) != text[i]) {
			return false;
		}
	}
	return true;
}

}

/**
 * Find text in document, supporting both forward and backward
 * searches (just pass minPos > maxPos to do a backward search)
 * Has not been tested with backwards DBCS searches yet.
 */
Sci::Position Document::FindText(Sci::Position minPos, Sci::Position maxPos, const char *search,
                        FindOption flags, Sci::Position *length) {
	if (*length <= 0)
		return minPos;
	const bool caseSensitive = FlagSet(flags, FindOption::MatchCase);
	const bool word = FlagSet(flags, FindOption::WholeWord);
	const bool wordStart = FlagSet(flags, FindOption::WordStart);
	const bool regExp = FlagSet(flags, FindOption::RegExp);
	if (regExp) {
		if (!regex)
			regex = std::unique_ptr<RegexSearchBase>(CreateRegexSearch(&charClass));
		return regex->FindText(this, minPos, maxPos, search, caseSensitive, word, wordStart, flags, length);
	} else {

		const bool forward = minPos <= maxPos;
		const int increment = forward ? 1 : -1;

		// Range endpoints should not be inside DBCS characters, but just in case, move them.
		const Sci::Position startPos = MovePositionOutsideChar(minPos, increment, false);
		const Sci::Position endPos = MovePositionOutsideChar(maxPos, increment, false);

		// Compute actual search ranges needed
		const Sci::Position lengthFind = *length;

		//Platform::DebugPrintf("Find %d %d %s %d\n", startPos, endPos, ft->lpstrText, lengthFind);
		const Sci::Position limitPos = std::max(startPos, endPos);
		Sci::Position pos = startPos;
		if (!forward) {
			// Back all of a character
			pos = NextPosition(pos, increment);
		}
		const SplitView cbView = cb.AllView();
		if (caseSensitive) {
			const Sci::Position endSearch = (startPos <= endPos) ? endPos - lengthFind + 1 : endPos;
			const unsigned char charStartSearch =  search[0];
			if (forward && ((0 == dbcsCodePage) || (CpUtf8 == dbcsCodePage && !UTF8IsTrailByte(charStartSearch)))) {
				// This is a fast case where there is no need to test byte values to iterate
				// so becomes the equivalent of a memchr+memcmp loop.
				// UTF-8 search will not be self-synchronizing when starts with trail byte
				const std::string_view suffix(search + 1, lengthFind - 1);
				while (pos < endSearch) {
					pos = SplitFindChar(cbView, pos, limitPos - pos, charStartSearch);
					if (pos < 0) {
						break;
					}
					if (SplitMatch(cbView, pos + 1, suffix) && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
						return pos;
					}
					pos++;
				}
			} else {
				while (forward ? (pos < endSearch) : (pos >= endSearch)) {
					const unsigned char leadByte = cbView.CharAt(pos);
					if (leadByte == charStartSearch) {
						bool found = (pos + lengthFind) <= limitPos;
						// SplitMatch could be called here but it is slower with g++ -O2
						for (int indexSearch = 1; (indexSearch < lengthFind) && found; indexSearch++) {
							found = cbView.CharAt(pos + indexSearch) == search[indexSearch];
						}
						if (found && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
							return pos;
						}
					}
					if (forward && UTF8IsAscii(leadByte)) {
						pos++;
					} else {
						if (dbcsCodePage) {
							if (!NextCharacter(pos, increment)) {
								break;
							}
						} else {
							pos += increment;
						}
					}
				}
			}
		} else if (CpUtf8 == dbcsCodePage) {
			constexpr size_t maxFoldingExpansion = 4;
			std::vector<char> searchThing((lengthFind+1) * UTF8MaxBytes * maxFoldingExpansion + 1);
			const size_t lenSearch =
				pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while (forward ? (pos < endPos) : (pos >= endPos)) {
				int widthFirstCharacter = 1;
				Sci::Position posIndexDocument = pos;
				size_t indexSearch = 0;
				bool characterMatches = true;
				while (indexSearch < lenSearch) {
					const unsigned char leadByte = cbView.CharAt(posIndexDocument);
					int widthChar = 1;
					size_t lenFlat = 1;
					if (UTF8IsAscii(leadByte)) {
						if ((posIndexDocument + 1) > limitPos) {
							break;
						}
						characterMatches = searchThing[indexSearch] == MakeLowerCase(leadByte);
					} else {
						char bytes[UTF8MaxBytes]{ static_cast<char>(leadByte) };
						const int widthCharBytes = UTF8BytesOfLead[leadByte];
						for (int b = 1; b < widthCharBytes; b++) {
							bytes[b] = cbView.CharAt(posIndexDocument + b);
						}
						widthChar = UTF8Classify(bytes, widthCharBytes) & UTF8MaskWidth;
						if (!indexSearch) {	// First character
							widthFirstCharacter = widthChar;
						}
						if ((posIndexDocument + widthChar) > limitPos) {
							break;
						}
						char folded[UTF8MaxBytes * maxFoldingExpansion + 1];
						lenFlat = pcf->Fold(folded, sizeof(folded), bytes, widthChar);
						// memcmp may examine lenFlat bytes in both arguments so assert it doesn't read past end of searchThing
						assert((indexSearch + lenFlat) <= searchThing.size());
						// Does folded match the buffer
						characterMatches = 0 == memcmp(folded, &searchThing[0] + indexSearch, lenFlat);
					}
					if (!characterMatches) {
						break;
					}
					posIndexDocument += widthChar;
					indexSearch += lenFlat;
				}
				if (characterMatches && (indexSearch == lenSearch)) {
					if (MatchesWordOptions(word, wordStart, pos, posIndexDocument - pos)) {
						*length = posIndexDocument - pos;
						return pos;
					}
				}
				if (forward) {
					pos += widthFirstCharacter;
				} else {
					if (!NextCharacter(pos, increment)) {
						break;
					}
				}
			}
		} else if (dbcsCodePage) {
			constexpr size_t maxBytesCharacter = 2;
			constexpr size_t maxFoldingExpansion = 4;
			std::vector<char> searchThing((lengthFind+1) * maxBytesCharacter * maxFoldingExpansion + 1);
			const size_t lenSearch = pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while (forward ? (pos < endPos) : (pos >= endPos)) {
				int widthFirstCharacter = 0;
				Sci::Position indexDocument = 0;
				size_t indexSearch = 0;
				bool characterMatches = true;
				while (((pos + indexDocument) < limitPos) &&
					(indexSearch < lenSearch)) {
					const unsigned char leadByte = cbView.CharAt(pos + indexDocument);
					const int widthChar = (!UTF8IsAscii(leadByte) && IsDBCSLeadByteNoExcept(leadByte)) ? 2 : 1;
					if (!widthFirstCharacter) {
						widthFirstCharacter = widthChar;
					}
					if ((pos + indexDocument + widthChar) > limitPos) {
						break;
					}
					size_t lenFlat = 1;
					if (widthChar == 1) {
						characterMatches = searchThing[indexSearch] == MakeLowerCase(leadByte);
					} else {
						const char bytes[maxBytesCharacter + 1] {
							static_cast<char>(leadByte),
							cbView.CharAt(pos + indexDocument + 1)
						};
						char folded[maxBytesCharacter * maxFoldingExpansion + 1];
						lenFlat = pcf->Fold(folded, sizeof(folded), bytes, widthChar);
						// memcmp may examine lenFlat bytes in both arguments so assert it doesn't read past end of searchThing
						assert((indexSearch + lenFlat) <= searchThing.size());
						// Does folded match the buffer
						characterMatches = 0 == memcmp(folded, &searchThing[0] + indexSearch, lenFlat);
					}
					if (!characterMatches) {
						break;
					}
					indexDocument += widthChar;
					indexSearch += lenFlat;
				}
				if (characterMatches && (indexSearch == lenSearch)) {
					if (MatchesWordOptions(word, wordStart, pos, indexDocument)) {
						*length = indexDocument;
						return pos;
					}
				}
				if (forward) {
					pos += widthFirstCharacter;
				} else {
					if (!NextCharacter(pos, increment)) {
						break;
					}
				}
			}
		} else {
			const Sci::Position endSearch = (startPos <= endPos) ? endPos - lengthFind + 1 : endPos;
			std::vector<char> searchThing(lengthFind + 1);
			pcf->Fold(&searchThing[0], searchThing.size(), search, lengthFind);
			while (forward ? (pos < endSearch) : (pos >= endSearch)) {
				bool found = (pos + lengthFind) <= limitPos;
				for (int indexSearch = 0; (indexSearch < lengthFind) && found; indexSearch++) {
					const char ch = cbView.CharAt(pos + indexSearch);
					const char chTest = searchThing[indexSearch];
					if (UTF8IsAscii(ch)) {
						found = chTest == MakeLowerCase(ch);
					} else {
						char folded[2];
						pcf->Fold(folded, sizeof(folded), &ch, 1);
						found = folded[0] == chTest;
					}
				}
				if (found && MatchesWordOptions(word, wordStart, pos, lengthFind)) {
					return pos;
				}
				pos += increment;
			}
		}
	}
	//Platform::DebugPrintf("Not found\n");
	return -1;
}

const char *Document::SubstituteByPosition(const char *text, Sci::Position *length) {
	if (regex)
		return regex->SubstituteByPosition(this, text, length);
	else
		return nullptr;
}

LineCharacterIndexType Document::LineCharacterIndex() const noexcept {
	return cb.LineCharacterIndex();
}

void Document::AllocateLineCharacterIndex(LineCharacterIndexType lineCharacterIndex) {
	return cb.AllocateLineCharacterIndex(lineCharacterIndex);
}

void Document::ReleaseLineCharacterIndex(LineCharacterIndexType lineCharacterIndex) {
	return cb.ReleaseLineCharacterIndex(lineCharacterIndex);
}

Sci::Line Document::LinesTotal() const noexcept {
	return cb.Lines();
}

void Document::AllocateLines(Sci::Line lines) {
	cb.AllocateLines(lines);
}

void Document::SetDefaultCharClasses(bool includeWordClass) {
	charClass.SetDefaultCharClasses(includeWordClass);
}

void Document::SetCharClasses(const unsigned char *chars, CharacterClass newCharClass) {
	charClass.SetCharClasses(chars, newCharClass);
}

int Document::GetCharsOfClass(CharacterClass characterClass, unsigned char *buffer) const {
	return charClass.GetCharsOfClass(characterClass, buffer);
}

void Document::SetCharacterCategoryOptimization(int countCharacters) {
	charMap.Optimize(countCharacters);
}

int Document::CharacterCategoryOptimization() const noexcept {
	return charMap.Size();
}

void SCI_METHOD Document::StartStyling(Sci_Position position) {
	endStyled = position;
}

bool SCI_METHOD Document::SetStyleFor(Sci_Position length, char style) {
	if (enteredStyling != 0) {
		return false;
	} else {
		enteredStyling++;
		const Sci::Position prevEndStyled = endStyled;
		if (cb.SetStyleFor(endStyled, length, style)) {
			const DocModification mh(ModificationFlags::ChangeStyle | ModificationFlags::User,
			                   prevEndStyled, length);
			NotifyModified(mh);
		}
		endStyled += length;
		enteredStyling--;
		return true;
	}
}

bool SCI_METHOD Document::SetStyles(Sci_Position length, const char *styles) {
	if (enteredStyling != 0) {
		return false;
	} else {
		enteredStyling++;
		bool didChange = false;
		Sci::Position startMod = 0;
		Sci::Position endMod = 0;
		for (int iPos = 0; iPos < length; iPos++, endStyled++) {
			PLATFORM_ASSERT(endStyled < Length());
			if (cb.SetStyleAt(endStyled, styles[iPos])) {
				if (!didChange) {
					startMod = endStyled;
				}
				didChange = true;
				endMod = endStyled;
			}
		}
		if (didChange) {
			const DocModification mh(ModificationFlags::ChangeStyle | ModificationFlags::User,
			                   startMod, endMod - startMod + 1);
			NotifyModified(mh);
		}
		enteredStyling--;
		return true;
	}
}

void Document::EnsureStyledTo(Sci::Position pos) {
	if ((enteredStyling == 0) && (pos > GetEndStyled())) {
		IncrementStyleClock();
		if (pli && !pli->UseContainerLexing()) {
			const Sci::Position endStyledTo = LineStartPosition(GetEndStyled());
			pli->Colourise(endStyledTo, pos);
		} else {
			// Ask the watchers to style, and stop as soon as one responds.
			for (std::vector<WatcherWithUserData>::iterator it = watchers.begin();
				(pos > GetEndStyled()) && (it != watchers.end()); ++it) {
				it->watcher->NotifyStyleNeeded(this, it->userData, pos);
			}
		}
	}
}

void Document::StyleToAdjustingLineDuration(Sci::Position pos) {
	const Sci::Position stylingStart = GetEndStyled();
	ElapsedPeriod epStyling;
	EnsureStyledTo(pos);
	durationStyleOneByte.AddSample(pos - stylingStart, epStyling.Duration());
}

LexInterface *Document::GetLexInterface() const noexcept {
	return pli.get();
}

void Document::SetLexInterface(std::unique_ptr<LexInterface> pLexInterface) noexcept {
	pli = std::move(pLexInterface);
}

int SCI_METHOD Document::SetLineState(Sci_Position line, int state) {
	const int statePrevious = States()->SetLineState(line, state, LinesTotal());
	if (state != statePrevious) {
		const DocModification mh(ModificationFlags::ChangeLineState, LineStart(line), 0, 0, nullptr,
			static_cast<Sci::Line>(line));
		NotifyModified(mh);
	}
	return statePrevious;
}

int SCI_METHOD Document::GetLineState(Sci_Position line) const {
	return States()->GetLineState(line);
}

Sci::Line Document::GetMaxLineState() const noexcept {
	return States()->GetMaxLineState();
}

void SCI_METHOD Document::ChangeLexerState(Sci_Position start, Sci_Position end) {
	const DocModification mh(ModificationFlags::LexerState, start,
		end-start, 0, nullptr, 0);
	NotifyModified(mh);
}

StyledText Document::MarginStyledText(Sci::Line line) const noexcept {
	const LineAnnotation *pla = Margins();
	return StyledText(pla->Length(line), pla->Text(line),
		pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
}

void Document::MarginSetText(Sci::Line line, const char *text) {
	Margins()->SetText(line, text);
	const DocModification mh(ModificationFlags::ChangeMargin, LineStart(line),
		0, 0, nullptr, line);
	NotifyModified(mh);
}

void Document::MarginSetStyle(Sci::Line line, int style) {
	Margins()->SetStyle(line, style);
	NotifyModified(DocModification(ModificationFlags::ChangeMargin, LineStart(line),
		0, 0, nullptr, line));
}

void Document::MarginSetStyles(Sci::Line line, const unsigned char *styles) {
	Margins()->SetStyles(line, styles);
	NotifyModified(DocModification(ModificationFlags::ChangeMargin, LineStart(line),
		0, 0, nullptr, line));
}

void Document::MarginClearAll() {
	const Sci::Line maxEditorLine = LinesTotal();
	for (Sci::Line l=0; l<maxEditorLine; l++)
		MarginSetText(l, nullptr);
	// Free remaining data
	Margins()->ClearAll();
}

StyledText Document::AnnotationStyledText(Sci::Line line) const noexcept {
	const LineAnnotation *pla = Annotations();
	return StyledText(pla->Length(line), pla->Text(line),
		pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
}

void Document::AnnotationSetText(Sci::Line line, const char *text) {
	if (line >= 0 && line < LinesTotal()) {
		const Sci::Line linesBefore = AnnotationLines(line);
		Annotations()->SetText(line, text);
		const int linesAfter = AnnotationLines(line);
		DocModification mh(ModificationFlags::ChangeAnnotation, LineStart(line),
			0, 0, nullptr, line);
		mh.annotationLinesAdded = linesAfter - linesBefore;
		NotifyModified(mh);
	}
}

void Document::AnnotationSetStyle(Sci::Line line, int style) {
	if (line >= 0 && line < LinesTotal()) {
		Annotations()->SetStyle(line, style);
		const DocModification mh(ModificationFlags::ChangeAnnotation, LineStart(line),
			0, 0, nullptr, line);
		NotifyModified(mh);
	}
}

void Document::AnnotationSetStyles(Sci::Line line, const unsigned char *styles) {
	if (line >= 0 && line < LinesTotal()) {
		Annotations()->SetStyles(line, styles);
	}
}

int Document::AnnotationLines(Sci::Line line) const noexcept {
	return Annotations()->Lines(line);
}

void Document::AnnotationClearAll() {
	if (Annotations()->Empty()) {
		return;
	}
	const Sci::Line maxEditorLine = LinesTotal();
	for (Sci::Line l=0; l<maxEditorLine; l++)
		AnnotationSetText(l, nullptr);
	// Free remaining data
	Annotations()->ClearAll();
}

StyledText Document::EOLAnnotationStyledText(Sci::Line line) const noexcept {
	const LineAnnotation *pla = EOLAnnotations();
	return StyledText(pla->Length(line), pla->Text(line),
		pla->MultipleStyles(line), pla->Style(line), pla->Styles(line));
}

void Document::EOLAnnotationSetText(Sci::Line line, const char *text) {
	if (line >= 0 && line < LinesTotal()) {
		EOLAnnotations()->SetText(line, text);
		const DocModification mh(ModificationFlags::ChangeEOLAnnotation, LineStart(line),
			0, 0, nullptr, line);
		NotifyModified(mh);
	}
}

void Document::EOLAnnotationSetStyle(Sci::Line line, int style) {
	if (line >= 0 && line < LinesTotal()) {
		EOLAnnotations()->SetStyle(line, style);
		const DocModification mh(ModificationFlags::ChangeEOLAnnotation, LineStart(line),
			0, 0, nullptr, line);
		NotifyModified(mh);
	}
}

void Document::EOLAnnotationClearAll() {
	if (EOLAnnotations()->Empty()) {
		return;
	}
	const Sci::Line maxEditorLine = LinesTotal();
	for (Sci::Line l=0; l<maxEditorLine; l++)
		EOLAnnotationSetText(l, nullptr);
	// Free remaining data
	EOLAnnotations()->ClearAll();
}

void Document::IncrementStyleClock() noexcept {
	styleClock = (styleClock + 1) % 0x100000;
}

void SCI_METHOD Document::DecorationSetCurrentIndicator(int indicator) {
	decorations->SetCurrentIndicator(indicator);
}

void SCI_METHOD Document::DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength) {
	const FillResult<Sci::Position> fr = decorations->FillRange(
		position, value, fillLength);
	if (fr.changed) {
		const DocModification mh(ModificationFlags::ChangeIndicator | ModificationFlags::User,
							fr.position, fr.fillLength);
		NotifyModified(mh);
	}
}

bool Document::AddWatcher(DocWatcher *watcher, void *userData) {
	const WatcherWithUserData wwud(watcher, userData);
	std::vector<WatcherWithUserData>::iterator it =
		std::find(watchers.begin(), watchers.end(), wwud);
	if (it != watchers.end())
		return false;
	watchers.push_back(wwud);
	return true;
}

bool Document::RemoveWatcher(DocWatcher *watcher, void *userData) noexcept {
	try {
		// This can never fail as WatcherWithUserData constructor and == are noexcept
		// but std::find is not noexcept.
		std::vector<WatcherWithUserData>::iterator it =
			std::find(watchers.begin(), watchers.end(), WatcherWithUserData(watcher, userData));
		if (it != watchers.end()) {
			watchers.erase(it);
			return true;
		}
	} catch (...) {
		// Ignore any exception
	}
	return false;
}

void Document::NotifyModifyAttempt() {
	for (const WatcherWithUserData &watcher : watchers) {
		watcher.watcher->NotifyModifyAttempt(this, watcher.userData);
	}
}

void Document::NotifySavePoint(bool atSavePoint) {
	for (const WatcherWithUserData &watcher : watchers) {
		watcher.watcher->NotifySavePoint(this, watcher.userData, atSavePoint);
	}
}

void Document::NotifyModified(DocModification mh) {
	if (FlagSet(mh.modificationType, ModificationFlags::InsertText)) {
		decorations->InsertSpace(mh.position, mh.length);
	} else if (FlagSet(mh.modificationType, ModificationFlags::DeleteText)) {
		decorations->DeleteRange(mh.position, mh.length);
	}
	for (const WatcherWithUserData &watcher : watchers) {
		watcher.watcher->NotifyModified(this, mh, watcher.userData);
	}
}

bool Document::IsWordPartSeparator(unsigned int ch) const {
	return (WordCharacterClass(ch) == CharacterClass::word) && IsPunctuation(ch);
}

Sci::Position Document::WordPartLeft(Sci::Position pos) const {
	if (pos > 0) {
		pos -= CharacterBefore(pos).widthBytes;
		CharacterExtracted ceStart = CharacterAfter(pos);
		if (IsWordPartSeparator(ceStart.character)) {
			while (pos > 0 && IsWordPartSeparator(CharacterAfter(pos).character)) {
				pos -= CharacterBefore(pos).widthBytes;
			}
		}
		if (pos > 0) {
			ceStart = CharacterAfter(pos);
			pos -= CharacterBefore(pos).widthBytes;
			if (IsLowerCase(ceStart.character)) {
				while (pos > 0 && IsLowerCase(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if (!IsUpperCase(CharacterAfter(pos).character) && !IsLowerCase(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			} else if (IsUpperCase(ceStart.character)) {
				while (pos > 0 && IsUpperCase(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if (!IsUpperCase(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			} else if (IsADigit(ceStart.character)) {
				while (pos > 0 && IsADigit(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if (!IsADigit(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			} else if (IsPunctuation(ceStart.character)) {
				while (pos > 0 && IsPunctuation(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if (!IsPunctuation(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			} else if (IsASpace(ceStart.character)) {
				while (pos > 0 && IsASpace(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if (!IsASpace(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			} else if (!IsASCII(ceStart.character)) {
				while (pos > 0 && !IsASCII(CharacterAfter(pos).character))
					pos -= CharacterBefore(pos).widthBytes;
				if (IsASCII(CharacterAfter(pos).character))
					pos += CharacterAfter(pos).widthBytes;
			} else {
				pos += CharacterAfter(pos).widthBytes;
			}
		}
	}
	return pos;
}

Sci::Position Document::WordPartRight(Sci::Position pos) const {
	CharacterExtracted ceStart = CharacterAfter(pos);
	const Sci::Position length = LengthNoExcept();
	if (IsWordPartSeparator(ceStart.character)) {
		while (pos < length && IsWordPartSeparator(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
		ceStart = CharacterAfter(pos);
	}
	if (!IsASCII(ceStart.character)) {
		while (pos < length && !IsASCII(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	} else if (IsLowerCase(ceStart.character)) {
		while (pos < length && IsLowerCase(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	} else if (IsUpperCase(ceStart.character)) {
		if (IsLowerCase(CharacterAfter(pos + ceStart.widthBytes).character)) {
			pos += CharacterAfter(pos).widthBytes;
			while (pos < length && IsLowerCase(CharacterAfter(pos).character))
				pos += CharacterAfter(pos).widthBytes;
		} else {
			while (pos < length && IsUpperCase(CharacterAfter(pos).character))
				pos += CharacterAfter(pos).widthBytes;
		}
		if (IsLowerCase(CharacterAfter(pos).character) && IsUpperCase(CharacterBefore(pos).character))
			pos -= CharacterBefore(pos).widthBytes;
	} else if (IsADigit(ceStart.character)) {
		while (pos < length && IsADigit(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	} else if (IsPunctuation(ceStart.character)) {
		while (pos < length && IsPunctuation(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	} else if (IsASpace(ceStart.character)) {
		while (pos < length && IsASpace(CharacterAfter(pos).character))
			pos += CharacterAfter(pos).widthBytes;
	} else {
		pos += CharacterAfter(pos).widthBytes;
	}
	return pos;
}

Sci::Position Document::ExtendStyleRange(Sci::Position pos, int delta, bool singleLine) noexcept {
	const char sStart = cb.StyleAt(pos);
	if (delta < 0) {
		while (pos > 0 && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsEOLCharacter(cb.CharAt(pos))))
			pos--;
		pos++;
	} else {
		while (pos < (LengthNoExcept()) && (cb.StyleAt(pos) == sStart) && (!singleLine || !IsEOLCharacter(cb.CharAt(pos))))
			pos++;
	}
	return pos;
}

static char BraceOpposite(char ch) noexcept {
	switch (ch) {
	case '(':
		return ')';
	case ')':
		return '(';
	case '[':
		return ']';
	case ']':
		return '[';
	case '{':
		return '}';
	case '}':
		return '{';
	case '<':
		return '>';
	case '>':
		return '<';
	default:
		return '\0';
	}
}

// TODO: should be able to extend styled region to find matching brace
Sci::Position Document::BraceMatch(Sci::Position position, Sci::Position /*maxReStyle*/, Sci::Position startPos, bool useStartPos) noexcept {
	const unsigned char chBrace = CharAt(position);
	const unsigned char chSeek = BraceOpposite(chBrace);
	if (chSeek == '\0')
		return -1;
	const int styBrace = StyleIndexAt(position);
	int direction = -1;
	if (chBrace == '(' || chBrace == '[' || chBrace == '{' || chBrace == '<')
		direction = 1;
	int depth = 1;
	position = useStartPos ? startPos : position + direction;

	// Avoid using MovePositionOutsideChar to check DBCS trail byte
	unsigned char maxSafeChar = 0xff;
	if (dbcsCodePage != 0 && dbcsCodePage != CpUtf8) {
		maxSafeChar = DBCSMinTrailByte() - 1;
	}

	while ((position >= 0) && (position < LengthNoExcept())) {
		const unsigned char chAtPos = CharAt(position);
		if (chAtPos == chBrace || chAtPos == chSeek) {
			if (((position > GetEndStyled()) || (StyleIndexAt(position) == styBrace)) &&
				(chAtPos <= maxSafeChar || position == MovePositionOutsideChar(position, direction, false))) {
				depth += (chAtPos == chBrace) ? 1 : -1;
				if (depth == 0)
					return position;
			}
		}
		position += direction;
	}
	return -1;
}

/**
 * Implementation of RegexSearchBase for the default built-in regular expression engine
 */
class BuiltinRegex : public RegexSearchBase {
public:
	explicit BuiltinRegex(CharClassify *charClassTable) : search(charClassTable) {}

	Sci::Position FindText(Document *doc, Sci::Position minPos, Sci::Position maxPos, const char *s,
                        bool caseSensitive, bool word, bool wordStart, FindOption flags,
                        Sci::Position *length) override;

	const char *SubstituteByPosition(Document *doc, const char *text, Sci::Position *length) override;

private:
	RESearch search;
	std::string substituted;
};

namespace {

/**
* RESearchRange keeps track of search range.
*/
class RESearchRange {
public:
	int increment;
	Sci::Position startPos;
	Sci::Position endPos;
	Sci::Line lineRangeStart;
	Sci::Line lineRangeEnd;
	Sci::Line lineRangeBreak;
	RESearchRange(const Document *doc, Sci::Position minPos, Sci::Position maxPos) noexcept {
		increment = (minPos <= maxPos) ? 1 : -1;

		// Range endpoints should not be inside DBCS characters or between a CR and LF,
		// but just in case, move them.
		startPos = doc->MovePositionOutsideChar(minPos, 1, true);
		endPos = doc->MovePositionOutsideChar(maxPos, 1, true);

		lineRangeStart = doc->SciLineFromPosition(startPos);
		lineRangeEnd = doc->SciLineFromPosition(endPos);
		lineRangeBreak = lineRangeEnd + increment;
	}
	Range LineRange(Sci::Line line, Sci::Position lineStartPos, Sci::Position lineEndPos) const noexcept {
		Range range(lineStartPos, lineEndPos);
		if (increment == 1) {
			if (line == lineRangeStart)
				range.start = startPos;
			if (line == lineRangeEnd)
				range.end = endPos;
		} else {
			if (line == lineRangeEnd)
				range.start = endPos;
			if (line == lineRangeStart)
				range.end = startPos;
		}
		return range;
	}
};

// Define a way for the Regular Expression code to access the document
class DocumentIndexer final : public CharacterIndexer {
	Document *pdoc;
	Sci::Position end;
public:
	DocumentIndexer(Document *pdoc_, Sci::Position end_) noexcept :
		pdoc(pdoc_), end(end_) {
	}

	char CharAt(Sci::Position index) const noexcept override {
		if (index < 0 || index >= end)
			return 0;
		else
			return pdoc->CharAt(index);
	}
	Sci::Position MovePositionOutsideChar(Sci::Position pos, Sci::Position moveDir) const noexcept override {
		return pdoc->MovePositionOutsideChar(pos, moveDir, false);
	}
};

#ifndef NO_CXX11_REGEX

class ByteIterator {
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = char;
	using difference_type = ptrdiff_t;
	using pointer = char*;
	using reference = char&;

	const Document *doc;
	Sci::Position position;

	explicit ByteIterator(const Document *doc_=nullptr, Sci::Position position_=0) noexcept :
		doc(doc_), position(position_) {
	}
	char operator*() const noexcept {
		return doc->CharAt(position);
	}
	ByteIterator &operator++() noexcept {
		position++;
		return *this;
	}
	ByteIterator operator++(int) noexcept {
		ByteIterator retVal(*this);
		position++;
		return retVal;
	}
	ByteIterator &operator--() noexcept {
		position--;
		return *this;
	}
	bool operator==(const ByteIterator &other) const noexcept {
		return doc == other.doc && position == other.position;
	}
	bool operator!=(const ByteIterator &other) const noexcept {
		return doc != other.doc || position != other.position;
	}
	Sci::Position Pos() const noexcept {
		return position;
	}
	Sci::Position PosRoundUp() const noexcept {
		return position;
	}
};

// On Windows, wchar_t is 16 bits wide and on Unix it is 32 bits wide.
// Would be better to use sizeof(wchar_t) or similar to differentiate
// but easier for now to hard-code platforms.
// C++11 has char16_t and char32_t but neither Clang nor Visual C++
// appear to allow specializing basic_regex over these.

#ifdef _WIN32
#define WCHAR_T_IS_16 1
#else
#define WCHAR_T_IS_16 0
#endif

#if WCHAR_T_IS_16

// On Windows, report non-BMP characters as 2 separate surrogates as that
// matches wregex since it is based on wchar_t.
class UTF8Iterator {
	// These 3 fields determine the iterator position and are used for comparisons
	const Document *doc;
	Sci::Position position;
	size_t characterIndex;
	// Remaining fields are derived from the determining fields so are excluded in comparisons
	unsigned int lenBytes;
	size_t lenCharacters;
	wchar_t buffered[2];
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = wchar_t;
	using difference_type = ptrdiff_t;
	using pointer = wchar_t*;
	using reference = wchar_t&;

	explicit UTF8Iterator(const Document *doc_=nullptr, Sci::Position position_=0) noexcept :
		doc(doc_), position(position_), characterIndex(0), lenBytes(0), lenCharacters(0), buffered{} {
		buffered[0] = 0;
		buffered[1] = 0;
		if (doc) {
			ReadCharacter();
		}
	}
	wchar_t operator*() const noexcept {
		assert(lenCharacters != 0);
		return buffered[characterIndex];
	}
	UTF8Iterator &operator++() noexcept {
		if ((characterIndex + 1) < (lenCharacters)) {
			characterIndex++;
		} else {
			position += lenBytes;
			ReadCharacter();
			characterIndex = 0;
		}
		return *this;
	}
	UTF8Iterator operator++(int) noexcept {
		UTF8Iterator retVal(*this);
		if ((characterIndex + 1) < (lenCharacters)) {
			characterIndex++;
		} else {
			position += lenBytes;
			ReadCharacter();
			characterIndex = 0;
		}
		return retVal;
	}
	UTF8Iterator &operator--() noexcept {
		if (characterIndex) {
			characterIndex--;
		} else {
			position = doc->NextPosition(position, -1);
			ReadCharacter();
			characterIndex = lenCharacters - 1;
		}
		return *this;
	}
	bool operator==(const UTF8Iterator &other) const noexcept {
		// Only test the determining fields, not the character widths and values derived from this
		return doc == other.doc &&
			position == other.position &&
			characterIndex == other.characterIndex;
	}
	bool operator!=(const UTF8Iterator &other) const noexcept {
		// Only test the determining fields, not the character widths and values derived from this
		return doc != other.doc ||
			position != other.position ||
			characterIndex != other.characterIndex;
	}
	Sci::Position Pos() const noexcept {
		return position;
	}
	Sci::Position PosRoundUp() const noexcept {
		if (characterIndex)
			return position + lenBytes;	// Force to end of character
		else
			return position;
	}
private:
	void ReadCharacter() noexcept {
		const CharacterExtracted charExtracted = doc->ExtractCharacter(position);
		lenBytes = charExtracted.widthBytes;
		if (charExtracted.character == unicodeReplacementChar) {
			lenCharacters = 1;
			buffered[0] = static_cast<wchar_t>(charExtracted.character);
		} else {
			lenCharacters = UTF16FromUTF32Character(charExtracted.character, buffered);
		}
	}
};

#else

// On Unix, report non-BMP characters as single characters

class UTF8Iterator {
	const Document *doc;
	Sci::Position position;
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = wchar_t;
	using difference_type = ptrdiff_t;
	using pointer = wchar_t*;
	using reference = wchar_t&;

	explicit UTF8Iterator(const Document *doc_=nullptr, Sci::Position position_=0) noexcept :
		doc(doc_), position(position_) {
	}
	wchar_t operator*() const noexcept {
		const CharacterExtracted charExtracted = doc->ExtractCharacter(position);
		return charExtracted.character;
	}
	UTF8Iterator &operator++() noexcept {
		position = doc->NextPosition(position, 1);
		return *this;
	}
	UTF8Iterator operator++(int) noexcept {
		UTF8Iterator retVal(*this);
		position = doc->NextPosition(position, 1);
		return retVal;
	}
	UTF8Iterator &operator--() noexcept {
		position = doc->NextPosition(position, -1);
		return *this;
	}
	bool operator==(const UTF8Iterator &other) const noexcept {
		return doc == other.doc && position == other.position;
	}
	bool operator!=(const UTF8Iterator &other) const noexcept {
		return doc != other.doc || position != other.position;
	}
	Sci::Position Pos() const noexcept {
		return position;
	}
	Sci::Position PosRoundUp() const noexcept {
		return position;
	}
};

#endif

std::regex_constants::match_flag_type MatchFlags(const Document *doc, Sci::Position startPos, Sci::Position endPos, Sci::Position lineStartPos, Sci::Position lineEndPos) {
	std::regex_constants::match_flag_type flagsMatch = std::regex_constants::match_default;
	if (startPos != lineStartPos) {
#ifdef _LIBCPP_VERSION
		flagsMatch |= std::regex_constants::match_not_bol;
		if (!doc->IsWordStartAt(startPos)) {
			flagsMatch |= std::regex_constants::match_not_bow;
		}
#else
		flagsMatch |= std::regex_constants::match_prev_avail;
#endif
	}
	if (endPos != lineEndPos) {
		flagsMatch |= std::regex_constants::match_not_eol;
		if (!doc->IsWordEndAt(endPos)) {
			flagsMatch |= std::regex_constants::match_not_eow;
		}
	}
	return flagsMatch;
}

template<typename Iterator, typename Regex>
bool MatchOnLines(const Document *doc, const Regex &regexp, const RESearchRange &resr, RESearch &search) {
	std::match_results<Iterator> match;

	// MSVC and libc++ have problems with ^ and $ matching line ends inside a range.
	// CRLF line ends are also a problem as ^ and $ only treat LF as a line end.
	// The std::regex::multiline option was added to C++17 to improve behaviour but
	// has not been implemented by compiler runtimes with MSVC always in multiline
	// mode and libc++ and libstdc++ always in single-line mode.
	// If multiline regex worked well then the line by line iteration could be removed
	// for the forwards case and replaced with the following:
#ifdef REGEX_MULTILINE
	const Sci::Position lineStartPos = doc->LineStart(resr.lineRangeStart);
	const Sci::Position lineEndPos = doc->LineEnd(resr.lineRangeEnd);
	Iterator itStart(doc, resr.startPos);
	Iterator itEnd(doc, resr.endPos);
	const std::regex_constants::match_flag_type flagsMatch = MatchFlags(doc, resr.startPos, resr.endPos, lineStartPos, lineEndPos);
	const bool matched = std::regex_search(itStart, itEnd, match, regexp, flagsMatch);
#else
	// Line by line.
	bool matched = false;
	for (Sci::Line line = resr.lineRangeStart; line != resr.lineRangeBreak; line += resr.increment) {
		const Sci::Position lineStartPos = doc->LineStart(line);
		const Sci::Position lineEndPos = doc->LineEnd(line);
		const Range lineRange = resr.LineRange(line, lineStartPos, lineEndPos);
		Iterator itStart(doc, lineRange.start);
		Iterator itEnd(doc, lineRange.end);
		const std::regex_constants::match_flag_type flagsMatch = MatchFlags(doc, lineRange.start, lineRange.end, lineStartPos, lineEndPos);
		std::regex_iterator<Iterator> it(itStart, itEnd, regexp, flagsMatch);
		for (const std::regex_iterator<Iterator> last; it != last; ++it) {
			match = *it;
			matched = true;
			if (resr.increment > 0) {
				break;
			}
		}
		if (matched) {
			break;
		}
	}
#endif
	if (matched) {
		for (size_t co = 0; co < match.size() && co < RESearch::MAXTAG; co++) {
			search.bopat[co] = match[co].first.Pos();
			search.eopat[co] = match[co].second.PosRoundUp();
		}
	}
	return matched;
}

Sci::Position Cxx11RegexFindText(const Document *doc, Sci::Position minPos, Sci::Position maxPos, const char *s,
	bool caseSensitive, Sci::Position *length, RESearch &search) {
	const RESearchRange resr(doc, minPos, maxPos);
	try {
		//ElapsedPeriod ep;
		std::regex::flag_type flagsRe = std::regex::ECMAScript;
		// Flags that appear to have no effect:
		// | std::regex::collate | std::regex::extended;
		if (!caseSensitive)
			flagsRe = flagsRe | std::regex::icase;

#if defined(REGEX_MULTILINE) && !defined(_MSC_VER)
		flagsRe = flagsRe | std::regex::multiline;
#endif

		// Clear the RESearch so can fill in matches
		search.Clear();

		bool matched = false;
		if (CpUtf8 == doc->dbcsCodePage) {
			const std::wstring ws = WStringFromUTF8(s);
			std::wregex regexp;
			regexp.assign(ws, flagsRe);
			matched = MatchOnLines<UTF8Iterator>(doc, regexp, resr, search);
		} else {
			std::regex regexp;
			regexp.assign(s, flagsRe);
			matched = MatchOnLines<ByteIterator>(doc, regexp, resr, search);
		}

		Sci::Position posMatch = -1;
		if (matched) {
			posMatch = search.bopat[0];
			*length = search.eopat[0] - search.bopat[0];
		}
		// Example - search in doc/ScintillaHistory.html for
		// [[:upper:]]eta[[:space:]]
		// On MacBook, normally around 1 second but with locale imbued -> 14 seconds.
		//const double durSearch = ep.Duration(true);
		//Platform::DebugPrintf("Search:%9.6g \n", durSearch);
		return posMatch;
	} catch (std::regex_error &) {
		// Failed to create regular expression
		throw RegexError();
	} catch (...) {
		// Failed in some other way
		return -1;
	}
}

#endif

}

Sci::Position BuiltinRegex::FindText(Document *doc, Sci::Position minPos, Sci::Position maxPos, const char *s,
                        bool caseSensitive, bool, bool, FindOption flags,
                        Sci::Position *length) {

#ifndef NO_CXX11_REGEX
	if (FlagSet(flags, FindOption::Cxx11RegEx)) {
			return Cxx11RegexFindText(doc, minPos, maxPos, s,
			caseSensitive, length, search);
	}
#endif

	const RESearchRange resr(doc, minPos, maxPos);

	const bool posix = FlagSet(flags, FindOption::Posix);

	const char *errmsg = search.Compile(s, *length, caseSensitive, posix);
	if (errmsg) {
		return -1;
	}
	// Find a variable in a property file: \$(\([A-Za-z0-9_.]+\))
	// Replace first '.' with '-' in each property file variable reference:
	//     Search: \$(\([A-Za-z0-9_-]+\)\.\([A-Za-z0-9_.]+\))
	//     Replace: $(\1-\2)
	Sci::Position pos = -1;
	Sci::Position lenRet = 0;
	const bool searchforLineStart = s[0] == '^';
	const char searchEnd = s[*length - 1];
	const char searchEndPrev = (*length > 1) ? s[*length - 2] : '\0';
	const bool searchforLineEnd = (searchEnd == '$') && (searchEndPrev != '\\');
	for (Sci::Line line = resr.lineRangeStart; line != resr.lineRangeBreak; line += resr.increment) {
		const Sci::Position lineStartPos = doc->LineStart(line);
		const Sci::Position lineEndPos = doc->LineEnd(line);
		Sci::Position startOfLine = lineStartPos;
		Sci::Position endOfLine = lineEndPos;
		if (resr.increment == 1) {
			if (line == resr.lineRangeStart) {
				if ((resr.startPos != startOfLine) && searchforLineStart)
					continue;	// Can't match start of line if start position after start of line
				startOfLine = resr.startPos;
			}
			if (line == resr.lineRangeEnd) {
				if ((resr.endPos != endOfLine) && searchforLineEnd)
					continue;	// Can't match end of line if end position before end of line
				endOfLine = resr.endPos;
			}
		} else {
			if (line == resr.lineRangeEnd) {
				if ((resr.endPos != startOfLine) && searchforLineStart)
					continue;	// Can't match start of line if end position after start of line
				startOfLine = resr.endPos;
			}
			if (line == resr.lineRangeStart) {
				if ((resr.startPos != endOfLine) && searchforLineEnd)
					continue;	// Can't match end of line if start position before end of line
				endOfLine = resr.startPos;
			}
		}

		const DocumentIndexer di(doc, endOfLine);
		search.SetLineRange(lineStartPos, lineEndPos);
		int success = search.Execute(di, startOfLine, endOfLine);
		if (success) {
			Sci::Position endPos = search.eopat[0];
			// There can be only one start of a line, so no need to look for last match in line
			if ((resr.increment == -1) && !searchforLineStart) {
				// Check for the last match on this line.
				while (success && (endPos < endOfLine)) {
					const RESearch::MatchPositions bopat = search.bopat;
					const RESearch::MatchPositions eopat = search.eopat;
					pos = endPos;
					if (pos == bopat[0]) {
						// empty match
						pos = doc->NextPosition(pos, 1);
					}
					success = search.Execute(di, pos, endOfLine);
					if (success) {
						endPos = search.eopat[0];
					} else {
						search.bopat = bopat;
						search.eopat = eopat;
					}
				}
			}
			pos = search.bopat[0];
			lenRet = endPos - pos;
			break;
		}
	}
	*length = lenRet;
	return pos;
}

const char *BuiltinRegex::SubstituteByPosition(Document *doc, const char *text, Sci::Position *length) {
	substituted.clear();
	for (Sci::Position j = 0; j < *length; j++) {
		if (text[j] == '\\') {
			const char chNext = text[++j];
			if (chNext >= '0' && chNext <= '9') {
				const unsigned int patNum = chNext - '0';
				const Sci::Position startPos = search.bopat[patNum];
				const Sci::Position len = search.eopat[patNum] - startPos;
				if (len > 0) {	// Will be null if try for a match that did not occur
					const size_t size = substituted.length();
					substituted.resize(size + len);
					doc->GetCharRange(substituted.data() + size, startPos, len);
				}
			} else {
				switch (chNext) {
				case 'a':
					substituted.push_back('\a');
					break;
				case 'b':
					substituted.push_back('\b');
					break;
				case 'f':
					substituted.push_back('\f');
					break;
				case 'n':
					substituted.push_back('\n');
					break;
				case 'r':
					substituted.push_back('\r');
					break;
				case 't':
					substituted.push_back('\t');
					break;
				case 'v':
					substituted.push_back('\v');
					break;
				case '\\':
					substituted.push_back('\\');
					break;
				default:
					substituted.push_back('\\');
					j--;
				}
			}
		} else {
			substituted.push_back(text[j]);
		}
	}
	*length = substituted.length();
	return substituted.c_str();
}

#ifndef SCI_OWNREGEX

RegexSearchBase *Scintilla::Internal::CreateRegexSearch(CharClassify *charClassTable) {
	return new BuiltinRegex(charClassTable);
}

#endif
