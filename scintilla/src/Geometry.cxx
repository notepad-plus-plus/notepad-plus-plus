// Scintilla source code edit control
/** @file Geometry.cxx
 ** Helper functions for geometric calculations.
 **/
// Copyright 2020 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdint>
#include <cmath>

#include <algorithm>

#include "Geometry.h"

namespace {

constexpr unsigned int Mixed(unsigned char a, unsigned char b, double proportion) noexcept {
	return static_cast<unsigned int>(a + proportion * (b - a));
}

}

namespace Scintilla::Internal {

PRectangle Clamp(PRectangle rc, Edge edge, XYPOSITION position) noexcept {
	switch (edge) {
	case Edge::left:
		return PRectangle(std::clamp(position, rc.left, rc.right), rc.top, rc.right, rc.bottom);
	case Edge::top:
		return PRectangle(rc.left, std::clamp(position, rc.top, rc.bottom), rc.right, rc.bottom);
	case Edge::right:
		return PRectangle(rc.left, rc.top, std::clamp(position, rc.left, rc.right), rc.bottom);
	case Edge::bottom:
	default:
		return PRectangle(rc.left, rc.top, rc.right, std::clamp(position, rc.top, rc.bottom));
	}
}

PRectangle Side(PRectangle rc, Edge edge, XYPOSITION size) noexcept {
	switch (edge) {
	case Edge::left:
		return PRectangle(rc.left, rc.top, std::min(rc.left + size, rc.right), rc.bottom);
	case Edge::top:
		return PRectangle(rc.left, rc.top, rc.right, std::min(rc.top + size, rc.bottom));
	case Edge::right:
		return PRectangle(std::max(rc.left, rc.right - size), rc.top, rc.right, rc.bottom);
	case Edge::bottom:
	default:
		return PRectangle(rc.left, std::max(rc.top, rc.bottom - size), rc.right, rc.bottom);
	}
}

Interval Intersection(Interval a, Interval b) noexcept {
	const XYPOSITION leftMax = std::max(a.left, b.left);
	const XYPOSITION rightMin = std::min(a.right, b.right);
	// If the result would have a negative width. make empty instead.
	const XYPOSITION rightResult = (rightMin >= leftMax) ? rightMin : leftMax;
	return { leftMax, rightResult };
}

PRectangle Intersection(PRectangle rc, Interval horizontalBounds) noexcept {
	const Interval intersection = Intersection(HorizontalBounds(rc), horizontalBounds);
	return PRectangle(intersection.left, rc.top, intersection.right, rc.bottom);
}

Interval HorizontalBounds(PRectangle rc) noexcept {
	return { rc.left, rc.right };
}

XYPOSITION PixelAlign(XYPOSITION xy, int pixelDivisions) noexcept {
	return std::round(xy * pixelDivisions) / pixelDivisions;
}

XYPOSITION PixelAlignFloor(XYPOSITION xy, int pixelDivisions) noexcept {
	return std::floor(xy * pixelDivisions) / pixelDivisions;
}

XYPOSITION PixelAlignCeil(XYPOSITION xy, int pixelDivisions) noexcept {
	return std::ceil(xy * pixelDivisions) / pixelDivisions;
}

Point PixelAlign(const Point &pt, int pixelDivisions) noexcept {
	return Point(
		PixelAlign(pt.x, pixelDivisions),
		PixelAlign(pt.y, pixelDivisions));
}

PRectangle PixelAlign(const PRectangle &rc, int pixelDivisions) noexcept {
	// Move left and right side to nearest pixel to avoid blurry visuals.
	// The top and bottom should be integers but floor them to make sure.
	// `pixelDivisions` is commonly 1 except for 'retina' displays where it is 2.
	// On retina displays, the positions should be moved to the nearest device
	// pixel which is the nearest half logical pixel.
	return PRectangle(
		PixelAlign(rc.left, pixelDivisions),
		PixelAlignFloor(rc.top, pixelDivisions),
		PixelAlign(rc.right, pixelDivisions),
		PixelAlignFloor(rc.bottom, pixelDivisions));
}

PRectangle PixelAlignOutside(const PRectangle &rc, int pixelDivisions) noexcept {
	// Move left and right side to extremes (floor(left) ceil(right)) to avoid blurry visuals.
	return PRectangle(
		PixelAlignFloor(rc.left, pixelDivisions),
		PixelAlignFloor(rc.top, pixelDivisions),
		PixelAlignCeil(rc.right, pixelDivisions),
		PixelAlignFloor(rc.bottom, pixelDivisions));
}

ColourRGBA ColourRGBA::MixedWith(ColourRGBA other) const noexcept {
	const unsigned int red = (GetRed() + other.GetRed()) / 2;
	const unsigned int green = (GetGreen() + other.GetGreen()) / 2;
	const unsigned int blue = (GetBlue() + other.GetBlue()) / 2;
	const unsigned int alpha = (GetAlpha() + other.GetAlpha()) / 2;
	return ColourRGBA(red, green, blue, alpha);
}

ColourRGBA ColourRGBA::MixedWith(ColourRGBA other, double proportion) const noexcept {
	return ColourRGBA(
		Mixed(GetRed(), other.GetRed(), proportion),
		Mixed(GetGreen(), other.GetGreen(), proportion),
		Mixed(GetBlue(), other.GetBlue(), proportion),
		Mixed(GetAlpha(), other.GetAlpha(), proportion));
}

}
