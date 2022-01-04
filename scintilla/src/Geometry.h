// Scintilla source code edit control
/** @file Geometry.h
 ** Classes and functions for geometric and colour calculations.
 **/
// Copyright 2020 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef GEOMETRY_H
#define GEOMETRY_H

namespace Scintilla::Internal {

typedef double XYPOSITION;
typedef double XYACCUMULATOR;

/**
 * A geometric point class.
 * Point is similar to the Win32 POINT and GTK+ GdkPoint types.
 */
class Point {
public:
	XYPOSITION x;
	XYPOSITION y;

	constexpr explicit Point(XYPOSITION x_=0, XYPOSITION y_=0) noexcept : x(x_), y(y_) {
	}

	static constexpr Point FromInts(int x_, int y_) noexcept {
		return Point(static_cast<XYPOSITION>(x_), static_cast<XYPOSITION>(y_));
	}

	constexpr bool operator==(Point other) const noexcept {
		return (x == other.x) && (y == other.y);
	}

	constexpr bool operator!=(Point other) const noexcept {
		return (x != other.x) || (y != other.y);
	}

	constexpr Point operator+(Point other) const noexcept {
		return Point(x + other.x, y + other.y);
	}

	constexpr Point operator-(Point other) const noexcept {
		return Point(x - other.x, y - other.y);
	}

	// Other automatically defined methods (assignment, copy constructor, destructor) are fine
};


/**
 * A geometric interval class.
 */
class Interval {
public:
	XYPOSITION left;
	XYPOSITION right;
	constexpr bool operator==(const Interval &other) const noexcept {
		return (left == other.left) && (right == other.right);
	}
	constexpr XYPOSITION Width() const noexcept { return right - left; }
	constexpr bool Empty() const noexcept {
		return Width() <= 0;
	}
	constexpr bool Intersects(Interval other) const noexcept {
		return (right > other.left) && (left < other.right);
	}
};

/**
 * A geometric rectangle class.
 * PRectangle is similar to Win32 RECT.
 * PRectangles contain their top and left sides, but not their right and bottom sides.
 */
class PRectangle {
public:
	XYPOSITION left;
	XYPOSITION top;
	XYPOSITION right;
	XYPOSITION bottom;

	constexpr explicit PRectangle(XYPOSITION left_=0, XYPOSITION top_=0, XYPOSITION right_=0, XYPOSITION bottom_ = 0) noexcept :
		left(left_), top(top_), right(right_), bottom(bottom_) {
	}

	static constexpr PRectangle FromInts(int left_, int top_, int right_, int bottom_) noexcept {
		return PRectangle(static_cast<XYPOSITION>(left_), static_cast<XYPOSITION>(top_),
			static_cast<XYPOSITION>(right_), static_cast<XYPOSITION>(bottom_));
	}

	// Other automatically defined methods (assignment, copy constructor, destructor) are fine

	constexpr bool operator==(const PRectangle &rc) const noexcept {
		return (rc.left == left) && (rc.right == right) &&
			(rc.top == top) && (rc.bottom == bottom);
	}
	constexpr bool Contains(Point pt) const noexcept {
		return (pt.x >= left) && (pt.x <= right) &&
			(pt.y >= top) && (pt.y <= bottom);
	}
	constexpr bool ContainsWholePixel(Point pt) const noexcept {
		// Does the rectangle contain all of the pixel to left/below the point
		return (pt.x >= left) && ((pt.x+1) <= right) &&
			(pt.y >= top) && ((pt.y+1) <= bottom);
	}
	constexpr bool Contains(PRectangle rc) const noexcept {
		return (rc.left >= left) && (rc.right <= right) &&
			(rc.top >= top) && (rc.bottom <= bottom);
	}
	constexpr bool Intersects(PRectangle other) const noexcept {
		return (right > other.left) && (left < other.right) &&
			(bottom > other.top) && (top < other.bottom);
	}
	void Move(XYPOSITION xDelta, XYPOSITION yDelta) noexcept {
		left += xDelta;
		top += yDelta;
		right += xDelta;
		bottom += yDelta;
	}

	constexpr PRectangle Inset(XYPOSITION delta) const noexcept {
		return PRectangle(left + delta, top + delta, right - delta, bottom - delta);
	}

	constexpr PRectangle Inset(Point delta) const noexcept {
		return PRectangle(left + delta.x, top + delta.y, right - delta.x, bottom - delta.y);
	}

	constexpr Point Centre() const noexcept {
		return Point((left + right) / 2, (top + bottom) / 2);
	}

	constexpr XYPOSITION Width() const noexcept { return right - left; }
	constexpr XYPOSITION Height() const noexcept { return bottom - top; }
	constexpr bool Empty() const noexcept {
		return (Height() <= 0) || (Width() <= 0);
	}
};

enum class Edge { left, top, bottom, right };

PRectangle Clamp(PRectangle rc, Edge edge, XYPOSITION position) noexcept;
PRectangle Side(PRectangle rc, Edge edge, XYPOSITION size) noexcept;

Interval Intersection(Interval a, Interval b) noexcept;
PRectangle Intersection(PRectangle rc, Interval horizontalBounds) noexcept;
Interval HorizontalBounds(PRectangle rc) noexcept;

XYPOSITION PixelAlign(XYPOSITION xy, int pixelDivisions) noexcept;
XYPOSITION PixelAlignFloor(XYPOSITION xy, int pixelDivisions) noexcept;

Point PixelAlign(const Point &pt, int pixelDivisions) noexcept;

PRectangle PixelAlign(const PRectangle &rc, int pixelDivisions) noexcept;
PRectangle PixelAlignOutside(const PRectangle &rc, int pixelDivisions) noexcept;

/**
* Holds an RGBA colour with 8 bits for each component.
*/
constexpr const float componentMaximum = 255.0f;
class ColourRGBA {
	int co;
public:
	constexpr explicit ColourRGBA(int co_ = 0) noexcept : co(co_) {
	}

	constexpr ColourRGBA(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha=0xff) noexcept :
		ColourRGBA(red | (green << 8) | (blue << 16) | (alpha << 24)) {
	}

	constexpr ColourRGBA(ColourRGBA cd, unsigned int alpha) noexcept :
		ColourRGBA(cd.OpaqueRGB() | (alpha << 24)) {
	}

	static constexpr ColourRGBA FromRGB(int co_) noexcept {
		return ColourRGBA(co_ | (0xffu << 24));
	}

	static constexpr ColourRGBA FromIpRGB(intptr_t co_) noexcept {
		return ColourRGBA(static_cast<int>(co_) | (0xffu << 24));
	}

	constexpr ColourRGBA WithoutAlpha() const noexcept {
		return ColourRGBA(co & 0xffffff);
	}

	constexpr ColourRGBA Opaque() const noexcept {
		return ColourRGBA(co | (0xffu << 24));
	}

	constexpr int AsInteger() const noexcept {
		return co;
	}

	constexpr int OpaqueRGB() const noexcept {
		return co & 0xffffff;
	}

	// Red, green and blue values as bytes 0..255
	constexpr unsigned char GetRed() const noexcept {
		return co & 0xff;
	}
	constexpr unsigned char GetGreen() const noexcept {
		return (co >> 8) & 0xff;
	}
	constexpr unsigned char GetBlue() const noexcept {
		return (co >> 16) & 0xff;
	}
	constexpr unsigned char GetAlpha() const noexcept {
		return (co >> 24) & 0xff;
	}

	// Red, green, blue, and alpha values as float 0..1.0
	constexpr float GetRedComponent() const noexcept {
		return GetRed() / componentMaximum;
	}
	constexpr float GetGreenComponent() const noexcept {
		return GetGreen() / componentMaximum;
	}
	constexpr float GetBlueComponent() const noexcept {
		return GetBlue() / componentMaximum;
	}
	constexpr float GetAlphaComponent() const noexcept {
		return GetAlpha() / componentMaximum;
	}

	constexpr bool operator==(const ColourRGBA &other) const noexcept {
		return co == other.co;
	}

	constexpr bool IsOpaque() const noexcept {
		return GetAlpha() == 0xff;
	}

	ColourRGBA MixedWith(ColourRGBA other) const noexcept;
	ColourRGBA MixedWith(ColourRGBA other, double proportion) const noexcept;
};

/**
* Holds an RGBA colour and stroke width to stroke a shape.
*/
class Stroke {
public:
	ColourRGBA colour;
	XYPOSITION width;
	constexpr Stroke(ColourRGBA colour_, XYPOSITION width_=1.0) noexcept :
		colour(colour_), width(width_) {
	}
	constexpr float WidthF() const noexcept {
		return static_cast<float>(width);
	}
};

/**
* Holds an RGBA colour to fill a shape.
*/
class Fill {
public:
	ColourRGBA colour;
	constexpr Fill(ColourRGBA colour_) noexcept :
		colour(colour_) {
	}
};

/**
* Holds a pair of RGBA colours and stroke width to fill and stroke a shape.
*/
class FillStroke {
public:
	Fill fill;
	Stroke stroke;
	constexpr FillStroke(ColourRGBA colourFill_, ColourRGBA colourStroke_, XYPOSITION widthStroke_=1.0) noexcept :
		fill(colourFill_), stroke(colourStroke_, widthStroke_) {
	}
	constexpr FillStroke(ColourRGBA colourBoth, XYPOSITION widthStroke_=1.0) noexcept :
		fill(colourBoth), stroke(colourBoth, widthStroke_) {
	}
};

/**
* Holds an element of a gradient with an RGBA colour and a relative position.
*/
class ColourStop {
public:
	XYPOSITION position;
	ColourRGBA colour;
	constexpr ColourStop(XYPOSITION position_, ColourRGBA colour_) noexcept :
		position(position_), colour(colour_) {
	}
};

}

#endif
