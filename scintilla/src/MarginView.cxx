// Scintilla source code edit control
/** @file MarginView.cxx
 ** Defines the appearance of the editor margin.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "CharacterCategory.h"
#include "Position.h"
#include "IntegerRectangle.h"
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

namespace Scintilla {

void DrawWrapMarker(Surface *surface, PRectangle rcPlace,
	bool isEndMarker, ColourDesired wrapColour) {
	surface->PenColour(wrapColour);

	const IntegerRectangle ircPlace(rcPlace);

	enum { xa = 1 }; // gap before start
	const int w = ircPlace.Width() - xa - 1;

	const bool xStraight = isEndMarker;  // x-mirrored symbol for start marker

	const int x0 = xStraight ? ircPlace.left : ircPlace.right - 1;
	const int y0 = ircPlace.top;

	const int dy = ircPlace.Height() / 5;
	const int y = ircPlace.Height() / 2 + dy;

	struct Relative {
		Surface *surface;
		int xBase;
		int xDir;
		int yBase;
		int yDir;
		void MoveTo(int xRelative, int yRelative) {
			surface->MoveTo(xBase + xDir * xRelative, yBase + yDir * yRelative);
		}
		void LineTo(int xRelative, int yRelative) {
			surface->LineTo(xBase + xDir * xRelative, yBase + yDir * yRelative);
		}
	};
	Relative rel = { surface, x0, xStraight ? 1 : -1, y0, 1 };

	// arrow head
	rel.MoveTo(xa, y);
	rel.LineTo(xa + 2 * w / 3, y - dy);
	rel.MoveTo(xa, y);
	rel.LineTo(xa + 2 * w / 3, y + dy);

	// arrow body
	rel.MoveTo(xa, y);
	rel.LineTo(xa + w, y);
	rel.LineTo(xa + w, y - 2 * dy);
	rel.LineTo(xa - 1,   // on windows lineto is exclusive endpoint, perhaps GTK not...
		y - 2 * dy);
}

MarginView::MarginView() noexcept {
	wrapMarkerPaddingRight = 3;
	customDrawWrapMarker = nullptr;
}

void MarginView::DropGraphics(bool freeObjects) {
	if (freeObjects) {
		pixmapSelMargin.reset();
		pixmapSelPattern.reset();
		pixmapSelPatternOffset1.reset();
	} else {
		if (pixmapSelMargin)
			pixmapSelMargin->Release();
		if (pixmapSelPattern)
			pixmapSelPattern->Release();
		if (pixmapSelPatternOffset1)
			pixmapSelPatternOffset1->Release();
	}
}

void MarginView::AllocateGraphics(const ViewStyle &vsDraw) {
	if (!pixmapSelMargin)
		pixmapSelMargin.reset(Surface::Allocate(vsDraw.technology));
	if (!pixmapSelPattern)
		pixmapSelPattern.reset(Surface::Allocate(vsDraw.technology));
	if (!pixmapSelPatternOffset1)
		pixmapSelPatternOffset1.reset(Surface::Allocate(vsDraw.technology));
}

void MarginView::RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw) {
	if (!pixmapSelPattern->Initialised()) {
		const int patternSize = 8;
		pixmapSelPattern->InitPixMap(patternSize, patternSize, surfaceWindow, wid);
		pixmapSelPatternOffset1->InitPixMap(patternSize, patternSize, surfaceWindow, wid);
		// This complex procedure is to reproduce the checkerboard dithered pattern used by windows
		// for scroll bars and Visual Studio for its selection margin. The colour of this pattern is half
		// way between the chrome colour and the chrome highlight colour making a nice transition
		// between the window chrome and the content area. And it works in low colour depths.
		const PRectangle rcPattern = PRectangle::FromInts(0, 0, patternSize, patternSize);

		// Initialize default colours based on the chrome colour scheme.  Typically the highlight is white.
		ColourDesired colourFMFill = vsDraw.selbar;
		ColourDesired colourFMStripes = vsDraw.selbarlight;

		if (!(vsDraw.selbarlight == ColourDesired(0xff, 0xff, 0xff))) {
			// User has chosen an unusual chrome colour scheme so just use the highlight edge colour.
			// (Typically, the highlight colour is white.)
			colourFMFill = vsDraw.selbarlight;
		}

		if (vsDraw.foldmarginColour.isSet) {
			// override default fold margin colour
			colourFMFill = vsDraw.foldmarginColour;
		}
		if (vsDraw.foldmarginHighlightColour.isSet) {
			// override default fold margin highlight colour
			colourFMStripes = vsDraw.foldmarginHighlightColour;
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
	}
}

static int SubstituteMarkerIfEmpty(int markerCheck, int markerDefault, const ViewStyle &vs) noexcept {
	if (vs.markers[markerCheck].markType == SC_MARK_EMPTY)
		return markerDefault;
	return markerCheck;
}

void MarginView::PaintMargin(Surface *surface, Sci::Line topLine, PRectangle rc, PRectangle rcMargin,
	const EditModel &model, const ViewStyle &vs) {

	PRectangle rcSelMargin = rcMargin;
	rcSelMargin.right = rcMargin.left;
	if (rcSelMargin.bottom < rc.bottom)
		rcSelMargin.bottom = rc.bottom;

	const Point ptOrigin = model.GetVisibleOriginInMain();
	FontAlias fontLineNumber = vs.styles[STYLE_LINENUMBER].font;
	for (size_t margin = 0; margin < vs.ms.size(); margin++) {
		if (vs.ms[margin].width > 0) {

			rcSelMargin.left = rcSelMargin.right;
			rcSelMargin.right = rcSelMargin.left + vs.ms[margin].width;

			if (vs.ms[margin].style != SC_MARGIN_NUMBER) {
				if (vs.ms[margin].mask & SC_MASK_FOLDERS) {
					// Required because of special way brush is created for selection margin
					// Ensure patterns line up when scrolling with separate margin view
					// by choosing correctly aligned variant.
					const bool invertPhase = static_cast<int>(ptOrigin.y) & 1;
					surface->FillRectangle(rcSelMargin,
						invertPhase ? *pixmapSelPattern : *pixmapSelPatternOffset1);
				} else {
					ColourDesired colour;
					switch (vs.ms[margin].style) {
					case SC_MARGIN_BACK:
						colour = vs.styles[STYLE_DEFAULT].back;
						break;
					case SC_MARGIN_FORE:
						colour = vs.styles[STYLE_DEFAULT].fore;
						break;
					case SC_MARGIN_COLOUR:
						colour = vs.ms[margin].back;
						break;
					default:
						colour = vs.styles[STYLE_LINENUMBER].back;
						break;
					}
					surface->FillRectangle(rcSelMargin, colour);
				}
			} else {
				surface->FillRectangle(rcSelMargin, vs.styles[STYLE_LINENUMBER].back);
			}

			const int lineStartPaint = static_cast<int>(rcMargin.top + ptOrigin.y) / vs.lineHeight;
			Sci::Line visibleLine = model.TopLineOfMain() + lineStartPaint;
			Sci::Position yposScreen = lineStartPaint * vs.lineHeight - static_cast<Sci::Position>(ptOrigin.y);
			// Work out whether the top line is whitespace located after a
			// lessening of fold level which implies a 'fold tail' but which should not
			// be displayed until the last of a sequence of whitespace.
			bool needWhiteClosure = false;
			if (vs.ms[margin].mask & SC_MASK_FOLDERS) {
				const int level = model.pdoc->GetLevel(model.pcs->DocFromDisplay(visibleLine));
				if (level & SC_FOLDLEVELWHITEFLAG) {
					Sci::Line lineBack = model.pcs->DocFromDisplay(visibleLine);
					int levelPrev = level;
					while ((lineBack > 0) && (levelPrev & SC_FOLDLEVELWHITEFLAG)) {
						lineBack--;
						levelPrev = model.pdoc->GetLevel(lineBack);
					}
					if (!(levelPrev & SC_FOLDLEVELHEADERFLAG)) {
						if (LevelNumber(level) < LevelNumber(levelPrev))
							needWhiteClosure = true;
					}
				}
				if (highlightDelimiter.isEnabled) {
					const Sci::Line lastLine = model.pcs->DocFromDisplay(topLine + model.LinesOnScreen()) + 1;
					model.pdoc->GetHighlightDelimiters(highlightDelimiter,
						model.pdoc->SciLineFromPosition(model.sel.MainCaret()), lastLine);
				}
			}

			// Old code does not know about new markers needed to distinguish all cases
			const int folderOpenMid = SubstituteMarkerIfEmpty(SC_MARKNUM_FOLDEROPENMID,
				SC_MARKNUM_FOLDEROPEN, vs);
			const int folderEnd = SubstituteMarkerIfEmpty(SC_MARKNUM_FOLDEREND,
				SC_MARKNUM_FOLDER, vs);

			while ((visibleLine < model.pcs->LinesDisplayed()) && yposScreen < rc.bottom) {

				PLATFORM_ASSERT(visibleLine < model.pcs->LinesDisplayed());
				const Sci::Line lineDoc = model.pcs->DocFromDisplay(visibleLine);
				PLATFORM_ASSERT(model.pcs->GetVisible(lineDoc));
				const Sci::Line firstVisibleLine = model.pcs->DisplayFromDoc(lineDoc);
				const Sci::Line lastVisibleLine = model.pcs->DisplayLastFromDoc(lineDoc);
				const bool firstSubLine = visibleLine == firstVisibleLine;
				const bool lastSubLine = visibleLine == lastVisibleLine;

				int marks = model.pdoc->GetMark(lineDoc);
				if (!firstSubLine)
					marks = 0;

				bool headWithTail = false;

				if (vs.ms[margin].mask & SC_MASK_FOLDERS) {
					// Decide which fold indicator should be displayed
					const int level = model.pdoc->GetLevel(lineDoc);
					const int levelNext = model.pdoc->GetLevel(lineDoc + 1);
					const int levelNum = LevelNumber(level);
					const int levelNextNum = LevelNumber(levelNext);
					if (level & SC_FOLDLEVELHEADERFLAG) {
						if (firstSubLine) {
							if (levelNum < levelNextNum) {
								if (model.pcs->GetExpanded(lineDoc)) {
									if (levelNum == SC_FOLDLEVELBASE)
										marks |= 1 << SC_MARKNUM_FOLDEROPEN;
									else
										marks |= 1 << folderOpenMid;
								} else {
									if (levelNum == SC_FOLDLEVELBASE)
										marks |= 1 << SC_MARKNUM_FOLDER;
									else
										marks |= 1 << folderEnd;
								}
							} else if (levelNum > SC_FOLDLEVELBASE) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							}
						} else {
							if (levelNum < levelNextNum) {
								if (model.pcs->GetExpanded(lineDoc)) {
									marks |= 1 << SC_MARKNUM_FOLDERSUB;
								} else if (levelNum > SC_FOLDLEVELBASE) {
									marks |= 1 << SC_MARKNUM_FOLDERSUB;
								}
							} else if (levelNum > SC_FOLDLEVELBASE) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							}
						}
						needWhiteClosure = false;
						const Sci::Line firstFollowupLine = model.pcs->DocFromDisplay(model.pcs->DisplayFromDoc(lineDoc + 1));
						const int firstFollowupLineLevel = model.pdoc->GetLevel(firstFollowupLine);
						const int secondFollowupLineLevelNum = LevelNumber(model.pdoc->GetLevel(firstFollowupLine + 1));
						if (!model.pcs->GetExpanded(lineDoc)) {
							if ((firstFollowupLineLevel & SC_FOLDLEVELWHITEFLAG) &&
								(levelNum > secondFollowupLineLevelNum))
								needWhiteClosure = true;

							if (highlightDelimiter.IsFoldBlockHighlighted(firstFollowupLine))
								headWithTail = true;
						}
					} else if (level & SC_FOLDLEVELWHITEFLAG) {
						if (needWhiteClosure) {
							if (levelNext & SC_FOLDLEVELWHITEFLAG) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							} else if (levelNextNum > SC_FOLDLEVELBASE) {
								marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
								needWhiteClosure = false;
							} else {
								marks |= 1 << SC_MARKNUM_FOLDERTAIL;
								needWhiteClosure = false;
							}
						} else if (levelNum > SC_FOLDLEVELBASE) {
							if (levelNextNum < levelNum) {
								if (levelNextNum > SC_FOLDLEVELBASE) {
									marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
								} else {
									marks |= 1 << SC_MARKNUM_FOLDERTAIL;
								}
							} else {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							}
						}
					} else if (levelNum > SC_FOLDLEVELBASE) {
						if (levelNextNum < levelNum) {
							needWhiteClosure = false;
							if (levelNext & SC_FOLDLEVELWHITEFLAG) {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
								needWhiteClosure = true;
							} else if (lastSubLine) {
								if (levelNextNum > SC_FOLDLEVELBASE) {
									marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
								} else {
									marks |= 1 << SC_MARKNUM_FOLDERTAIL;
								}
							} else {
								marks |= 1 << SC_MARKNUM_FOLDERSUB;
							}
						} else {
							marks |= 1 << SC_MARKNUM_FOLDERSUB;
						}
					}
				}

				marks &= vs.ms[margin].mask;

				PRectangle rcMarker(
					rcSelMargin.left,
					static_cast<XYPOSITION>(yposScreen),
					rcSelMargin.right,
					static_cast<XYPOSITION>(yposScreen + vs.lineHeight));
				if (vs.ms[margin].style == SC_MARGIN_NUMBER) {
					if (firstSubLine) {
						std::string sNumber;
						if (lineDoc >= 0) {
							sNumber = std::to_string(lineDoc + 1);
						}
						if (model.foldFlags & (SC_FOLDFLAG_LEVELNUMBERS | SC_FOLDFLAG_LINESTATE)) {
							char number[100] = "";
							if (model.foldFlags & SC_FOLDFLAG_LEVELNUMBERS) {
								const int lev = model.pdoc->GetLevel(lineDoc);
								sprintf(number, "%c%c %03X %03X",
									(lev & SC_FOLDLEVELHEADERFLAG) ? 'H' : '_',
									(lev & SC_FOLDLEVELWHITEFLAG) ? 'W' : '_',
									LevelNumber(lev),
									lev >> 16
									);
							} else {
								const int state = model.pdoc->GetLineState(lineDoc);
								sprintf(number, "%0X", state);
							}
							sNumber = number;
						}
						PRectangle rcNumber = rcMarker;
						// Right justify
						const XYPOSITION width = surface->WidthText(fontLineNumber, sNumber);
						const XYPOSITION xpos = rcNumber.right - width - vs.marginNumberPadding;
						rcNumber.left = xpos;
						DrawTextNoClipPhase(surface, rcNumber, vs.styles[STYLE_LINENUMBER],
							rcNumber.top + vs.maxAscent, sNumber, drawAll);
					} else if (vs.wrapVisualFlags & SC_WRAPVISUALFLAG_MARGIN) {
						PRectangle rcWrapMarker = rcMarker;
						rcWrapMarker.right -= wrapMarkerPaddingRight;
						rcWrapMarker.left = rcWrapMarker.right - vs.styles[STYLE_LINENUMBER].aveCharWidth;
						if (!customDrawWrapMarker) {
							DrawWrapMarker(surface, rcWrapMarker, false, vs.styles[STYLE_LINENUMBER].fore);
						} else {
							customDrawWrapMarker(surface, rcWrapMarker, false, vs.styles[STYLE_LINENUMBER].fore);
						}
					}
				} else if (vs.ms[margin].style == SC_MARGIN_TEXT || vs.ms[margin].style == SC_MARGIN_RTEXT) {
					const StyledText stMargin = model.pdoc->MarginStyledText(lineDoc);
					if (stMargin.text && ValidStyledText(vs, vs.marginStyleOffset, stMargin)) {
						if (firstSubLine) {
							surface->FillRectangle(rcMarker,
								vs.styles[stMargin.StyleAt(0) + vs.marginStyleOffset].back);
							PRectangle rcText = rcMarker;
							if (vs.ms[margin].style == SC_MARGIN_RTEXT) {
								const int width = WidestLineWidth(surface, vs, vs.marginStyleOffset, stMargin);
								rcText.left = rcText.right - width - 3;
							}
							DrawStyledText(surface, vs, vs.marginStyleOffset, rcText,
								stMargin, 0, stMargin.length, drawAll);
						} else {
							// if we're displaying annotation lines, colour the margin to match the associated document line
							const int annotationLines = model.pdoc->AnnotationLines(lineDoc);
							if (annotationLines && (visibleLine > lastVisibleLine - annotationLines)) {
								surface->FillRectangle(rcMarker, vs.styles[stMargin.StyleAt(0) + vs.marginStyleOffset].back);
							}
						}
					}
				}

				if (marks) {
					for (int markBit = 0; (markBit < 32) && marks; markBit++) {
						if (marks & 1) {
							LineMarker::FoldPart part = LineMarker::FoldPart::undefined;
							if ((vs.ms[margin].mask & SC_MASK_FOLDERS) && highlightDelimiter.IsFoldBlockHighlighted(lineDoc)) {
								if (highlightDelimiter.IsBodyOfFoldBlock(lineDoc)) {
									part = LineMarker::FoldPart::body;
								} else if (highlightDelimiter.IsHeadOfFoldBlock(lineDoc)) {
									if (firstSubLine) {
										part = headWithTail ? LineMarker::FoldPart::headWithTail : LineMarker::FoldPart::head;
									} else {
										if (model.pcs->GetExpanded(lineDoc) || headWithTail) {
											part = LineMarker::FoldPart::body;
										} else {
											part = LineMarker::FoldPart::undefined;
										}
									}
								} else if (highlightDelimiter.IsTailOfFoldBlock(lineDoc)) {
									part = LineMarker::FoldPart::tail;
								}
							}
							vs.markers[markBit].Draw(surface, rcMarker, fontLineNumber, part, vs.ms[margin].style);
						}
						marks >>= 1;
					}
				}

				visibleLine++;
				yposScreen += vs.lineHeight;
			}
		}
	}

	PRectangle rcBlankMargin = rcMargin;
	rcBlankMargin.left = rcSelMargin.right;
	surface->FillRectangle(rcBlankMargin, vs.styles[STYLE_DEFAULT].back);
}

}

