// Scintilla source code edit control
/** @file Indicator.cxx
 ** Defines the style of indicators which are text decorations such as underlining.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>
#include <cmath>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Indicator.h"
#include "XPM.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

void Indicator::Draw(Surface *surface, const PRectangle &rc, const PRectangle &rcLine, const PRectangle &rcCharacter, State state, int value) const {
	StyleAndColour sacDraw = sacNormal;
	if (FlagSet(Flags(), IndicFlag::ValueFore)) {
		sacDraw.fore = ColourRGBA::FromRGB(value & static_cast<int>(IndicValue::Mask));
	}
	if (state == State::hover) {
		sacDraw = sacHover;
	}

	const int pixelDivisions = surface->PixelDivisions();

	const XYPOSITION halfWidth = strokeWidth / 2.0f;

	const PRectangle rcAligned(PixelAlignOutside(rc, pixelDivisions));
	PRectangle rcFullHeightAligned = PixelAlignOutside(rcLine, pixelDivisions);
	rcFullHeightAligned.left = rcAligned.left;
	rcFullHeightAligned.right = rcAligned.right;

	const XYPOSITION ymid = PixelAlign(rc.Centre().y, pixelDivisions);

	// This is a reasonable clip for indicators beneath text like underlines
	PRectangle rcClip = rcAligned;
	rcClip.bottom = rcFullHeightAligned.bottom;

	switch (sacDraw.style) {
	case IndicatorStyle::Squiggle: {
			surface->SetClip(rcClip);
			XYPOSITION x = rcAligned.left + halfWidth;
			const XYPOSITION top = rcAligned.top + halfWidth;
			const XYPOSITION xLast = rcAligned.right + halfWidth;
			XYPOSITION y = 0;
			std::vector<Point> pts;
			const XYPOSITION pitch = 1 + strokeWidth;
			pts.emplace_back(x, top + y);
			while (x < xLast) {
				x += pitch;
				y = pitch - y;
				pts.emplace_back(x, top + y);
				}
			surface->PolyLine(pts.data(), std::size(pts), Stroke(sacDraw.fore, strokeWidth));
			surface->PopClip();
		}
		break;

	case IndicatorStyle::SquigglePixmap: {
			const PRectangle rcSquiggle = PixelAlign(rc, 1);

			const int width = std::min(4000, static_cast<int>(rcSquiggle.Width()));
			RGBAImage image(width, 3, 1.0, nullptr);
			constexpr unsigned int alphaFull = 0xff;
			constexpr unsigned int alphaSide = 0x2f;
			constexpr unsigned int alphaSide2 = 0x5f;
			for (int x = 0; x < width; x++) {
				if (x%2) {
					// Two halfway columns have a full pixel in middle flanked by light pixels
					image.SetPixel(x, 0, ColourRGBA(sacDraw.fore, alphaSide));
					image.SetPixel(x, 1, ColourRGBA(sacDraw.fore, alphaFull));
					image.SetPixel(x, 2, ColourRGBA(sacDraw.fore, alphaSide));
				} else {
					// Extreme columns have a full pixel at bottom or top and a mid-tone pixel in centre
					image.SetPixel(x, (x % 4) ? 0 : 2, ColourRGBA(sacDraw.fore, alphaFull));
					image.SetPixel(x, 1, ColourRGBA(sacDraw.fore, alphaSide2));
				}
			}
			surface->DrawRGBAImage(rcSquiggle, image.GetWidth(), image.GetHeight(), image.Pixels());
		}
		break;

	case IndicatorStyle::SquiggleLow: {
			std::vector<Point> pts;
			const XYPOSITION top = rcAligned.top + halfWidth;
			int y = 0;
			XYPOSITION x = std::round(rcAligned.left) + halfWidth;
			pts.emplace_back(x, top + y);
			const XYPOSITION pitch = 2 + strokeWidth;
			x += pitch;
			while (x < rcAligned.right) {
				pts.emplace_back(x - 1, top + y);
				y = 1 - y;
				pts.emplace_back(x, top + y);
				x += pitch;
			}
			pts.emplace_back(rcAligned.right, top + y);
			surface->PolyLine(pts.data(), std::size(pts), Stroke(sacDraw.fore, strokeWidth));
		}
		break;

	case IndicatorStyle::TT: {
			surface->SetClip(rcClip);
			const XYPOSITION yLine = ymid;
			XYPOSITION x = rcAligned.left + 5.0f;
			const XYPOSITION pitch = 4 + strokeWidth;
			while (x < rc.right + pitch) {
				const PRectangle line(x-pitch, yLine, x, yLine + strokeWidth);
				surface->FillRectangle(line, sacDraw.fore);
				const PRectangle tail(x - 2 - strokeWidth, yLine + strokeWidth, x - 2, yLine + strokeWidth * 2);
				surface->FillRectangle(tail, sacDraw.fore);
				x++;
				x += pitch;
			}
			surface->PopClip();
		}
		break;

	case IndicatorStyle::Diagonal: {
			surface->SetClip(rcClip);
			XYPOSITION x = rcAligned.left + halfWidth;
			const XYPOSITION top = rcAligned.top + halfWidth;
			const XYPOSITION pitch = 3 + strokeWidth;
			while (x < rc.right) {
				const XYPOSITION endX = x+3;
				const XYPOSITION endY = top - 1;
				surface->LineDraw(Point(x, top + 2), Point(endX, endY), Stroke(sacDraw.fore, strokeWidth));
				x += pitch;
			}
			surface->PopClip();
		}
		break;

	case IndicatorStyle::Strike: {
			const XYPOSITION yStrike = std::round(rcLine.Centre().y);
			const PRectangle rcStrike(
				rcAligned.left, yStrike, rcAligned.right, yStrike + strokeWidth);
			surface->FillRectangle(rcStrike, sacDraw.fore);
		}
		break;

	case IndicatorStyle::Hidden:
	case IndicatorStyle::TextFore:
		// Draw nothing
		break;

	case IndicatorStyle::Box: {
			PRectangle rcBox = rcFullHeightAligned;
			rcBox.top = rcBox.top + 1.0f;
			rcBox.bottom = ymid + 1.0f;
			surface->RectangleFrame(rcBox, Stroke(ColourRGBA(sacDraw.fore, outlineAlpha), strokeWidth));
		}
		break;

	case IndicatorStyle::RoundBox:
	case IndicatorStyle::StraightBox:
	case IndicatorStyle::FullBox: {
			PRectangle rcBox = rcFullHeightAligned;
			if (sacDraw.style != IndicatorStyle::FullBox)
				rcBox.top = rcBox.top + 1;
			surface->AlphaRectangle(rcBox, (sacDraw.style == IndicatorStyle::RoundBox) ? 1.0f : 0.0f,
						FillStroke(ColourRGBA(sacDraw.fore, fillAlpha), ColourRGBA(sacDraw.fore, outlineAlpha), strokeWidth));
		}
		break;

	case IndicatorStyle::Gradient:
	case IndicatorStyle::GradientCentre: {
			PRectangle rcBox = rcFullHeightAligned;
			rcBox.top = rcBox.top + 1;
			const Surface::GradientOptions options = Surface::GradientOptions::topToBottom;
			const ColourRGBA start(sacDraw.fore, fillAlpha);
			const ColourRGBA end(sacDraw.fore, 0);
			std::vector<ColourStop> stops;
			switch (sacDraw.style) {
			case IndicatorStyle::Gradient:
				stops.push_back(ColourStop(0.0, start));
				stops.push_back(ColourStop(1.0, end));
				break;
			case IndicatorStyle::GradientCentre:
				stops.push_back(ColourStop(0.0, end));
				stops.push_back(ColourStop(0.5, start));
				stops.push_back(ColourStop(1.0, end));
				break;
			default:
				break;
			}
			surface->GradientRectangle(rcBox, stops, options);
		}
		break;

	case IndicatorStyle::DotBox: {
			PRectangle rcBox = rcFullHeightAligned;
			rcBox.top = rcBox.top + 1;
			// Cap width at 4000 to avoid large allocations when mistakes made
			const int width = std::min(static_cast<int>(rcBox.Width()), 4000);
			const int height = static_cast<int>(rcBox.Height());
			RGBAImage image(width, height, 1.0, nullptr);
			// Draw horizontal lines top and bottom
			for (int x=0; x<width; x++) {
				for (int y = 0; y< height; y += height - 1) {
					image.SetPixel(x, y, ColourRGBA(sacDraw.fore, ((x + y) % 2) ? outlineAlpha : fillAlpha));
				}
			}
			// Draw vertical lines left and right
			for (int y = 1; y<height; y++) {
				for (int x=0; x<width; x += width-1) {
					image.SetPixel(x, y, ColourRGBA(sacDraw.fore, ((x + y) % 2) ? outlineAlpha : fillAlpha));
				}
			}
			surface->DrawRGBAImage(rcBox, image.GetWidth(), image.GetHeight(), image.Pixels());
		}
		break;

	case IndicatorStyle::Dash: {
			XYPOSITION x = std::floor(rc.left);
			const XYPOSITION widthDash = 3 + std::round(strokeWidth);
			while (x < rc.right) {
				const PRectangle rcDash = PRectangle(x, ymid,
					x + widthDash, ymid + std::round(strokeWidth));
				surface->FillRectangle(rcDash, sacDraw.fore);
				x += 3 + widthDash;
			}
		}
		break;

	case IndicatorStyle::Dots: {
			const XYPOSITION widthDot = std::round(strokeWidth);
			XYPOSITION x = std::floor(rc.left);
			while (x < rc.right) {
				const PRectangle rcDot = PRectangle(x, ymid,
					x + widthDot, ymid + widthDot);
				surface->FillRectangle(rcDot, sacDraw.fore);
				x += widthDot * 2;
			}
		}
		break;

	case IndicatorStyle::CompositionThick: {
			const PRectangle rcComposition(rc.left+1, rcLine.bottom-2, rc.right-1, rcLine.bottom);
			surface->FillRectangle(rcComposition, ColourRGBA(sacDraw.fore, outlineAlpha));
		}
		break;

	case IndicatorStyle::CompositionThin: {
			const PRectangle rcComposition(rc.left+1, rcLine.bottom-2, rc.right-1, rcLine.bottom-1);
			surface->FillRectangle(rcComposition, sacDraw.fore);
		}
		break;

	case IndicatorStyle::Point:
	case IndicatorStyle::PointCharacter:
		if (rcCharacter.Width() >= 0.1) {
			const XYPOSITION pixelHeight = std::floor(rc.Height());	// 1 pixel onto next line if multiphase
			const XYPOSITION x = (sacDraw.style == IndicatorStyle::Point) ? (rcCharacter.left) : ((rcCharacter.right + rcCharacter.left) / 2);
			// 0.5f is to hit midpoint of pixels:
			const XYPOSITION ix = std::round(x) + 0.5f;
			const XYPOSITION iy = std::ceil(rc.bottom) + 0.5f;
			const Point pts[] = {
				Point(ix - pixelHeight, iy),	// Left
				Point(ix + pixelHeight, iy),	// Right
				Point(ix, iy - pixelHeight)								// Top
			};
			surface->Polygon(pts, std::size(pts), FillStroke(sacDraw.fore));
		}
		break;

	case IndicatorStyle::PointTop:
		if (rcCharacter.Width() >= 0.1) {
			const XYPOSITION pixelHeight = std::floor(rc.Height());	// 1 pixel onto previous line if multiphase
			const XYPOSITION x = rcCharacter.left;
			// 0.5f is to hit midpoint of pixels:
			const XYPOSITION ix = std::round(x) + 0.5f;
			const XYPOSITION iy = std::floor(rcLine.top) - 0.5f;
			const Point pts[] = {
				Point(ix - pixelHeight, iy),	// Left
				Point(ix + pixelHeight, iy),	// Right
				Point(ix, iy + pixelHeight)		// Bottom
			};
			surface->Polygon(pts, std::size(pts), FillStroke(sacDraw.fore));
		}
		break;

	default:
		// Either IndicatorStyle::Plain, IndicatorStyle::ExplorerLink or unknown
		surface->FillRectangle(PRectangle(rcAligned.left, ymid,
			rcAligned.right, ymid + std::round(strokeWidth)), sacDraw.fore);
	}
}

void Indicator::SetFlags(IndicFlag attributes_) noexcept {
	attributes = attributes_;
}
