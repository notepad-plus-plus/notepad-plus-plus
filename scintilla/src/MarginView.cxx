// Scintilla source code edit control
/** @file MarginView.cxx
 ** Defines the appearance of the editor margin.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
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
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
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
#include "KeyMap.h"
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
#include "MarginView.h"
#include "EditView.h"

using namespace Scintilla;

namespace Scintilla::Internal {

void DrawWrapMarker(Surface *surface, PRectangle rcPlace,
	bool isEndMarker, ColourRGBA wrapColour) {

	const XYPOSITION extraFinalPixel = surface->SupportsFeature(Supports::LineDrawsFinal) ? 0.0f : 1.0f;

	const PRectangle rcAligned = PixelAlignOutside(rcPlace, surface->PixelDivisions());

	const XYPOSITION widthStroke = std::floor(rcAligned.Width() / 6);

	constexpr XYPOSITION xa = 1; // gap before start
	const XYPOSITION w = rcAligned.Width() - xa - widthStroke;

	// isEndMarker -> x-mirrored symbol for start marker

	const XYPOSITION x0 = isEndMarker ? rcAligned.left : rcAligned.right - widthStroke;
	const XYPOSITION y0 = rcAligned.top;

	const XYPOSITION dy = std::floor(rcAligned.Height() / 5);
	const XYPOSITION y = std::floor(rcAligned.Height() / 2) + dy;

	struct Relative {
		XYPOSITION xBase;
		int xDir;
		XYPOSITION yBase;
		int yDir;
		XYPOSITION halfWidth;
		Point At(XYPOSITION xRelative, XYPOSITION yRelative) const noexcept {
			return Point(xBase + xDir * xRelative + halfWidth, yBase + yDir * yRelative + halfWidth);
		}
	};

	Relative rel = { x0, isEndMarker ? 1 : -1, y0, 1, widthStroke / 2.0f };

	// arrow head
	const Point head[] = {
		rel.At(xa + dy, y - dy),
		rel.At(xa, y),
		rel.At(xa + dy + extraFinalPixel, y + dy + extraFinalPixel)
	};
	surface->PolyLine(head, std::size(head), Stroke(wrapColour, widthStroke));

	// arrow body
	const Point body[] = {
		rel.At(xa, y),
		rel.At(xa + w, y),
		rel.At(xa + w, y - 2 * dy),
		rel.At(xa, y - 2 * dy),
	};
	surface->PolyLine(body, std::size(body), Stroke(wrapColour, widthStroke));
}

MarginView::MarginView() noexcept {
	wrapMarkerPaddingRight = 3;
	customDrawWrapMarker = nullptr;
}

void MarginView::DropGraphics() noexcept {
	pixmapSelMargin.reset();
	pixmapSelPattern.reset();
	pixmapSelPatternOffset1.reset();
}

void MarginView::RefreshPixMaps(Surface *surfaceWindow, const ViewStyle &vsDraw) {
	if (!pixmapSelPattern) {
		constexpr int patternSize = 8;
		pixmapSelPattern = surfaceWindow->AllocatePixMap(patternSize, patternSize);
		pixmapSelPatternOffset1 = surfaceWindow->AllocatePixMap(patternSize, patternSize);
		// This complex procedure is to reproduce the checkerboard dithered pattern used by windows
		// for scroll bars and Visual Studio for its selection margin. The colour of this pattern is half
		// way between the chrome colour and the chrome highlight colour making a nice transition
		// between the window chrome and the content area. And it works in low colour depths.
		const PRectangle rcPattern = PRectangle::FromInts(0, 0, patternSize, patternSize);

		// Initialize default colours based on the chrome colour scheme.  Typically the highlight is white.
		ColourRGBA colourFMFill = vsDraw.selbar;
		ColourRGBA colourFMStripes = vsDraw.selbarlight;

		if (!(vsDraw.selbarlight == ColourRGBA(0xff, 0xff, 0xff))) {
			// User has chosen an unusual chrome colour scheme so just use the highlight edge colour.
			// (Typically, the highlight colour is white.)
			colourFMFill = vsDraw.selbarlight;
		}

		if (vsDraw.foldmarginColour) {
			// override default fold margin colour
			colourFMFill = *vsDraw.foldmarginColour;
		}
		if (vsDraw.foldmarginHighlightColour) {
			// override default fold margin highlight colour
			colourFMStripes = *vsDraw.foldmarginHighlightColour;
		}

		pixmapSelPattern->FillRectangle(rcPattern, colourFMFill);
		pixmapSelPatternOffset1->FillRectangle(rcPattern, colourFMStripes);
		for (int y = 0; y < patternSize; y++) {
			for (int x = y % 2; x < patternSize; x += 2) {
				const PRectangle rcPixel = PRectangle::FromInts(x, y, x + 1, y + 1);
				pixmapSelPattern->FillRectangle(rcPixel, colourFMStripes);
				pixmapSelPatternOffset1->FillRectangle(rcPixel, colourFMFill);
			}
		}
		pixmapSelPattern->FlushDrawing();
		pixmapSelPatternOffset1->FlushDrawing();
	}
}

namespace {

MarkerOutline SubstituteMarkerIfEmpty(MarkerOutline markerCheck, MarkerOutline markerDefault, const ViewStyle &vs) noexcept {
	if (vs.markers[static_cast<size_t>(markerCheck)].markType == MarkerSymbol::Empty)
		return markerDefault;
	return markerCheck;
}

constexpr MarkerOutline TailFromNextLevel(FoldLevel levelNextNum) noexcept {
	return (levelNextNum > FoldLevel::Base) ? MarkerOutline::FolderMidTail : MarkerOutline::FolderTail;
}

int FoldingMark(FoldLevel level, FoldLevel levelNext, bool firstSubLine, bool lastSubLine,
	bool isExpanded, bool needWhiteClosure, MarkerOutline folderOpenMid, MarkerOutline folderEnd) noexcept {

	const FoldLevel levelNum = LevelNumberPart(level);
	const FoldLevel levelNextNum = LevelNumberPart(levelNext);

	if (LevelIsHeader(level)) {
		if (firstSubLine) {
			if (levelNum < levelNextNum) {
				if (levelNum == FoldLevel::Base) {
					return 1 << (isExpanded ? MarkerOutline::FolderOpen : MarkerOutline::Folder);
				} else {
					return 1 << (isExpanded ? folderOpenMid : folderEnd);
				}
			} else if (levelNum > FoldLevel::Base) {
				return 1 << MarkerOutline::FolderSub;
			}
		} else {
			if (levelNum < levelNextNum) {
				if (isExpanded) {
					return 1 << MarkerOutline::FolderSub;
				} else if (levelNum > FoldLevel::Base) {
					return 1 << MarkerOutline::FolderSub;
				}
			} else if (levelNum > FoldLevel::Base) {
				return 1 << MarkerOutline::FolderSub;
			}
		}
	} else if (LevelIsWhitespace(level)) {
		if (needWhiteClosure) {
			if (LevelIsWhitespace(levelNext)) {
				return 1 << MarkerOutline::FolderSub;
			} else {
				return 1 << TailFromNextLevel(levelNextNum);
			}
		} else if (levelNum > FoldLevel::Base) {
			if (levelNextNum < levelNum) {
				return 1 << TailFromNextLevel(levelNextNum);
			} else {
				return 1 << MarkerOutline::FolderSub;
			}
		}
	} else if (levelNum > FoldLevel::Base) {
		if (levelNextNum < levelNum) {
			if (LevelIsWhitespace(levelNext)) {
				return 1 << MarkerOutline::FolderSub;
			} else if (lastSubLine) {
				return 1 << TailFromNextLevel(levelNextNum);
			} else {
				return 1 << MarkerOutline::FolderSub;
			}
		} else {
			return 1 << MarkerOutline::FolderSub;
		}
	}

	// No folding mark on this line
	return 0;
}

LineMarker::FoldPart PartForFoldHighlight(const HighlightDelimiter &highlightDelimiter, Sci::Line lineDoc, bool firstSubLine, bool headWithTail, bool isExpanded) noexcept {
	if (highlightDelimiter.IsFoldBlockHighlighted(lineDoc)) {
		if (highlightDelimiter.IsBodyOfFoldBlock(lineDoc)) {
			return LineMarker::FoldPart::body;
		}
		if (highlightDelimiter.IsHeadOfFoldBlock(lineDoc)) {
			if (firstSubLine) {
				return headWithTail ? LineMarker::FoldPart::headWithTail : LineMarker::FoldPart::head;
			}
			if (isExpanded || headWithTail) {
				return LineMarker::FoldPart::body;
			}
		} else if (highlightDelimiter.IsTailOfFoldBlock(lineDoc)) {
			return LineMarker::FoldPart::tail;
		}
	}
	return LineMarker::FoldPart::undefined;
}

constexpr LineMarker::FoldPart PartForBar(bool markBefore, bool markAfter) {
	if (markBefore) {
		if (markAfter) {
			return LineMarker::FoldPart::body;
		}
		return LineMarker::FoldPart::tail;
	}
	if (markAfter) {
		return LineMarker::FoldPart::head;
	}
	return LineMarker::FoldPart::headWithTail;
}

}

void MarginView::PaintOneMargin(Surface *surface, PRectangle rc, PRectangle rcOneMargin, const MarginStyle &marginStyle,
	const EditModel &model, const ViewStyle &vs) const {
	const Point ptOrigin = model.GetVisibleOriginInMain();
	const Sci::Line lineStartPaint = static_cast<Sci::Line>(rcOneMargin.top + ptOrigin.y) / vs.lineHeight;
	Sci::Line visibleLine = model.TopLineOfMain() + lineStartPaint;
	XYPOSITION yposScreen = lineStartPaint * vs.lineHeight - ptOrigin.y;
	// Work out whether the top line is whitespace located after a
	// lessening of fold level which implies a 'fold tail' but which should not
	// be displayed until the last of a sequence of whitespace.
	bool needWhiteClosure = false;
	if (marginStyle.ShowsFolding()) {
		const FoldLevel level = model.pdoc->GetFoldLevel(model.pcs->DocFromDisplay(visibleLine));
		if (LevelIsWhitespace(level)) {
			Sci::Line lineBack = model.pcs->DocFromDisplay(visibleLine);
			FoldLevel levelPrev = level;
			while ((lineBack > 0) && LevelIsWhitespace(levelPrev)) {
				lineBack--;
				levelPrev = model.pdoc->GetFoldLevel(lineBack);
			}
			if (!LevelIsHeader(levelPrev)) {
				if (LevelNumber(level) < LevelNumber(levelPrev))
					needWhiteClosure = true;
			}
		}
	}

	// Old code does not know about new markers needed to distinguish all cases
	const MarkerOutline folderOpenMid = SubstituteMarkerIfEmpty(MarkerOutline::FolderOpenMid,
		MarkerOutline::FolderOpen, vs);
	const MarkerOutline folderEnd = SubstituteMarkerIfEmpty(MarkerOutline::FolderEnd,
		MarkerOutline::Folder, vs);

	while ((visibleLine < model.pcs->LinesDisplayed()) && yposScreen < rc.bottom) {

		PLATFORM_ASSERT(visibleLine < model.pcs->LinesDisplayed());
		const Sci::Line lineDoc = model.pcs->DocFromDisplay(visibleLine);
		PLATFORM_ASSERT((lineDoc == 0) || model.pcs->GetVisible(lineDoc));
		const Sci::Line firstVisibleLine = model.pcs->DisplayFromDoc(lineDoc);
		const Sci::Line lastVisibleLine = model.pcs->DisplayLastFromDoc(lineDoc);
		const bool firstSubLine = visibleLine == firstVisibleLine;
		const bool lastSubLine = visibleLine == lastVisibleLine;

		int marks = model.GetMark(lineDoc);
		if (!firstSubLine) {
			// Mask off non-continuing marks
			marks = marks & vs.maskDrawWrapped;
		}

		bool headWithTail = false;
		bool isExpanded = false;

		if (marginStyle.ShowsFolding()) {
			// Decide which fold indicator should be displayed
			const FoldLevel level = model.pdoc->GetFoldLevel(lineDoc);
			const FoldLevel levelNext = model.pdoc->GetFoldLevel(lineDoc + 1);
			isExpanded = model.pcs->GetExpanded(lineDoc);

			marks |= FoldingMark(level, levelNext, firstSubLine, lastSubLine,
				isExpanded, needWhiteClosure, folderOpenMid, folderEnd);

			const FoldLevel levelNum = LevelNumberPart(level);
			const FoldLevel levelNextNum = LevelNumberPart(levelNext);

			// Change needWhiteClosure and headWithTail if needed
			if (LevelIsHeader(level)) {
				needWhiteClosure = false;
				if (!isExpanded) {
					const Sci::Line firstFollowupLine = model.pcs->DocFromDisplay(model.pcs->DisplayFromDoc(lineDoc + 1));
					const FoldLevel firstFollowupLineLevel = model.pdoc->GetFoldLevel(firstFollowupLine);
					const FoldLevel secondFollowupLineLevelNum = LevelNumberPart(model.pdoc->GetFoldLevel(firstFollowupLine + 1));
					if (LevelIsWhitespace(firstFollowupLineLevel) &&
						(levelNum > secondFollowupLineLevelNum))
						needWhiteClosure = true;

					if (highlightDelimiter.IsFoldBlockHighlighted(firstFollowupLine))
						headWithTail = true;
				}
			} else if (LevelIsWhitespace(level)) {
				if (needWhiteClosure) {
					needWhiteClosure = LevelIsWhitespace(levelNext);
				}
			} else if (levelNum > FoldLevel::Base) {
				if (levelNextNum < levelNum) {
					needWhiteClosure = LevelIsWhitespace(levelNext);
				}
			}
		}

		const PRectangle rcMarker(
			rcOneMargin.left,
			yposScreen,
			rcOneMargin.right,
			yposScreen + vs.lineHeight);
		if (marginStyle.style == MarginType::Number) {
			if (firstSubLine) {
				std::string sNumber;
				if (lineDoc >= 0) {
					sNumber = std::to_string(lineDoc + 1);
				}
				if (FlagSet(model.foldFlags, (FoldFlag::LevelNumbers | FoldFlag::LineState))) {
					char number[100] = "";
					if (FlagSet(model.foldFlags, FoldFlag::LevelNumbers)) {
						const FoldLevel lev = model.pdoc->GetFoldLevel(lineDoc);
						sprintf(number, "%c%c %03X %03X",
							LevelIsHeader(lev) ? 'H' : '_',
							LevelIsWhitespace(lev) ? 'W' : '_',
							LevelNumber(lev),
							static_cast<int>(lev) >> 16
						);
					} else {
						const int state = model.pdoc->GetLineState(lineDoc);
						sprintf(number, "%0X", state);
					}
					sNumber = number;
				}
				PRectangle rcNumber = rcMarker;
				// Right justify
				const XYPOSITION width = surface->WidthText(vs.styles[StyleLineNumber].font.get(), sNumber);
				const XYPOSITION xpos = rcNumber.right - width - vs.marginNumberPadding;
				rcNumber.left = xpos;
				DrawTextNoClipPhase(surface, rcNumber, vs.styles[StyleLineNumber],
					rcNumber.top + vs.maxAscent, sNumber, DrawPhase::all);
			} else if (FlagSet(vs.wrap.visualFlags, WrapVisualFlag::Margin)) {
				PRectangle rcWrapMarker = rcMarker;
				rcWrapMarker.right -= wrapMarkerPaddingRight;
				rcWrapMarker.left = rcWrapMarker.right - vs.styles[StyleLineNumber].aveCharWidth;
				if (!customDrawWrapMarker) {
					DrawWrapMarker(surface, rcWrapMarker, false, vs.styles[StyleLineNumber].fore);
				} else {
					customDrawWrapMarker(surface, rcWrapMarker, false, vs.styles[StyleLineNumber].fore);
				}
			}
		} else if (marginStyle.style == MarginType::Text || marginStyle.style == MarginType::RText) {
			const StyledText stMargin = model.pdoc->MarginStyledText(lineDoc);
			if (stMargin.text && ValidStyledText(vs, vs.marginStyleOffset, stMargin)) {
				if (firstSubLine) {
					surface->FillRectangle(rcMarker,
						vs.styles[stMargin.StyleAt(0) + vs.marginStyleOffset].back);
					PRectangle rcText = rcMarker;
					if (marginStyle.style == MarginType::RText) {
						const int width = WidestLineWidth(surface, vs, vs.marginStyleOffset, stMargin);
						rcText.left = rcText.right - width - 3;
					}
					DrawStyledText(surface, vs, vs.marginStyleOffset, rcText,
						stMargin, 0, stMargin.length, DrawPhase::all);
				} else {
					// if we're displaying annotation lines, colour the margin to match the associated document line
					const int annotationLines = model.pdoc->AnnotationLines(lineDoc);
					if (annotationLines && (visibleLine > lastVisibleLine - annotationLines)) {
						surface->FillRectangle(rcMarker, vs.styles[stMargin.StyleAt(0) + vs.marginStyleOffset].back);
					}
				}
			}
		}

		marks &= marginStyle.mask;

		if (marks) {
			// Draw all the bar markers first so they are underneath as they often cover
			// multiple lines for change history and other markers mark individual lines.
			int marksBar = marks;
			for (int markBit = 0; (markBit <= MarkerMax) && marksBar; markBit++) {
				if ((marksBar & 1) && (vs.markers[markBit].markType == MarkerSymbol::Bar)) {
					const int mask = 1 << markBit;
					const bool markBefore = firstSubLine ? (model.GetMark(lineDoc - 1) & mask) : true;
					const bool markAfter = lastSubLine ? (model.GetMark(lineDoc + 1) & mask) : true;
					vs.markers[markBit].Draw(surface, rcMarker, vs.styles[StyleLineNumber].font.get(),
						PartForBar(markBefore, markAfter), marginStyle.style);
				}
				marksBar >>= 1;
			}
			// Draw all the other markers over the bar markers
			for (int markBit = 0; (markBit <= MarkerMax) && marks; markBit++) {
				if ((marks & 1) && (vs.markers[markBit].markType != MarkerSymbol::Bar)) {
					const LineMarker::FoldPart part = marginStyle.ShowsFolding() ?
						PartForFoldHighlight(highlightDelimiter, lineDoc, firstSubLine, headWithTail, isExpanded) :
						LineMarker::FoldPart::undefined;
					vs.markers[markBit].Draw(surface, rcMarker, vs.styles[StyleLineNumber].font.get(), part, marginStyle.style);
				}
				marks >>= 1;
			}
		}

		visibleLine++;
		yposScreen += vs.lineHeight;
	}
}

void MarginView::PaintMargin(Surface *surface, Sci::Line topLine, PRectangle rc, PRectangle rcMargin,
	const EditModel &model, const ViewStyle &vs) {

	PRectangle rcOneMargin = rcMargin;
	rcOneMargin.right = rcMargin.left;
	if (rcOneMargin.bottom < rc.bottom)
		rcOneMargin.bottom = rc.bottom;

	const Point ptOrigin = model.GetVisibleOriginInMain();
	for (const MarginStyle &marginStyle : vs.ms) {
		if (marginStyle.width > 0) {

			rcOneMargin.left = rcOneMargin.right;
			rcOneMargin.right = rcOneMargin.left + marginStyle.width;

			if (marginStyle.style != MarginType::Number) {
				if (marginStyle.ShowsFolding()) {
					// Required because of special way brush is created for selection margin
					// Ensure patterns line up when scrolling with separate margin view
					// by choosing correctly aligned variant.
					const bool invertPhase = static_cast<int>(ptOrigin.y) & 1;
					surface->FillRectangle(rcOneMargin,
						invertPhase ? *pixmapSelPattern : *pixmapSelPatternOffset1);
				} else {
					ColourRGBA colour;
					switch (marginStyle.style) {
					case MarginType::Back:
						colour = vs.styles[StyleDefault].back;
						break;
					case MarginType::Fore:
						colour = vs.styles[StyleDefault].fore;
						break;
					case MarginType::Colour:
						colour = marginStyle.back;
						break;
					default:
						colour = vs.styles[StyleLineNumber].back;
						break;
					}
					surface->FillRectangle(rcOneMargin, colour);
				}
			} else {
				surface->FillRectangle(rcOneMargin, vs.styles[StyleLineNumber].back);
			}

			if (marginStyle.ShowsFolding() && highlightDelimiter.isEnabled) {
				const Sci::Line lastLine = model.pcs->DocFromDisplay(topLine + model.LinesOnScreen()) + 1;
				model.pdoc->GetHighlightDelimiters(highlightDelimiter,
					model.pdoc->SciLineFromPosition(model.sel.MainCaret()), lastLine);
			}

			PaintOneMargin(surface, rc, rcOneMargin, marginStyle, model, vs);
		}
	}

	PRectangle rcBlankMargin = rcMargin;
	rcBlankMargin.left = rcOneMargin.right;
	surface->FillRectangle(rcBlankMargin, vs.styles[StyleDefault].back);
}

}

