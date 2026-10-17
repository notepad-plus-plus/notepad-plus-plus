// Scintilla source code edit control
/** @file Geometry.h
 ** Classes and functions for geometric and colour calculations.
 **/
// Copyright 2020 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef GEOMETRY_H
#define GEOMETRY_H

namespace Scintilla::Internal {

using XYPOSITION = double;
using XYACCUMULATOR = double;

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

	constexpr Point operator*(XYPOSITION multiplier) const noexcept {
		return Point(x * multiplier, y * multiplier);
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
	[[nodiscard]] constexpr XYPOSITION Width() const noexcept { return right - left; }
	[[nodiscard]] constexpr bool Empty() const noexcept {
		return Width() <= 0;
	}
	[[nodiscard]] constexpr bool Intersects(Interval other) const noexcept {
		return (right > other.left) && (left < other.right);
	}
	[[nodiscard]] constexpr Interval Offset(XYPOSITION offset) const noexcept {
		return {left + offset, right + offset};
	}
	[[nodiscard]] static constexpr Interval FromLeftAndWidth(XYPOSITION left_, XYPOSITION width) {
		return {left_, left_+width};
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
	
	constexpr PRectangle operator*(XYPOSITION multiplier) const noexcept {
		return PRectangle(left * multiplier, top * multiplier, right * multiplier, bottom * multiplier);
	}

	constexpr PRectangle operator/(XYPOSITION divisor) const noexcept {
		return PRectangle(left / divisor, top / divisor, right / divisor, bottom / divisor);
	}

	[[nodiscard]] constexpr bool Contains(Point pt) const noexcept {
		return (pt.x >= left) && (pt.x <= right) &&
			(pt.y >= top) && (pt.y <= bottom);
	}
	[[nodiscard]] constexpr bool ContainsWholePixel(Point pt) const noexcept {
		// Does the rectangle contain all of the pixel to left/below the point
		return (pt.x >= left) && ((pt.x+1) <= right) &&
			(pt.y >= top) && ((pt.y+1) <= bottom);
	}
	[[nodiscard]] constexpr bool Contains(PRectangle rc) const noexcept {
		return (rc.left >= left) && (rc.right <= right) &&
			(rc.top >= top) && (rc.bottom <= bottom);
	}
	[[nodiscard]] constexpr bool Intersects(PRectangle other) const noexcept {
		return (right > other.left) && (left < other.right) &&
			(bottom > other.top) && (top < other.bottom);
	}
	[[nodiscard]] constexpr bool Intersects(Interval horizontalBounds) const noexcept {
		return (right > horizontalBounds.left) && (left < horizontalBounds.right);
	}

	void Move(XYPOSITION xDelta, XYPOSITION yDelta) noexcept {
		left += xDelta;
		top += yDelta;
		right += xDelta;
		bottom += yDelta;
	}

	[[nodiscard]] PRectangle WithHorizontalBounds(Interval horizontal) const noexcept {
		return PRectangle(horizontal.left, top, horizontal.right, bottom);
	}

	[[nodiscard]] constexpr PRectangle Inset(XYPOSITION delta) const noexcept {
		return PRectangle(left + delta, top + delta, right - delta, bottom - delta);
	}

	[[nodiscard]] constexpr PRectangle Inset(Point delta) const noexcept {
		return PRectangle(left + delta.x, top + delta.y, right - delta.x, bottom - delta.y);
	}

	[[nodiscard]] constexpr Point Centre() const noexcept {
		return Point((left + right) / 2, (top + bottom) / 2);
	}

	[[nodiscard]] constexpr XYPOSITION Width() const noexcept { return right - left; }
	[[nodiscard]] constexpr XYPOSITION Height() const noexcept { return bottom - top; }
	[[nodiscard]] constexpr bool Empty() const noexcept {
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
XYPOSITION PixelAlignCeil(XYPOSITION xy, int pixelDivisions) noexcept;

Point PixelAlign(const Point &pt, int pixelDivisions) noexcept;

PRectangle PixelAlign(const PRectangle &rc, int pixelDivisions) noexcept;
PRectangle PixelAlignOutside(const PRectangle &rc, int pixelDivisions) noexcept;

/**
* Holds an RGBA colour with 8 bits for each component.
*/
constexpr float componentMaximum = 255.0F;
constexpr unsigned int maximumByte = 0xffU;
class ColourRGBA {
	static constexpr float ComponentAsFloat(unsigned char component) {
		return component / componentMaximum;
	}
	static constexpr int rgbMask = 0xffffff;
	// static constexpr int rShift = 0;
	static constexpr int gShift = 8;
	static constexpr int bShift = 16;
	static constexpr int aShift = 24;
	int co;
public:
	constexpr explicit ColourRGBA(int co_ = 0) noexcept : co(co_) {
	}

	constexpr ColourRGBA(unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha=maximumByte) noexcept :
		ColourRGBA(red | (green << gShift) | (blue << bShift) | (alpha << aShift)) {
	}

	constexpr ColourRGBA(ColourRGBA cd, unsigned int alpha) noexcept :
		ColourRGBA(cd.OpaqueRGB() | (alpha << aShift)) {
	}

	static constexpr ColourRGBA FromRGB(int co_) noexcept {
		return ColourRGBA(co_ | (maximumByte << aShift));
	}

	static constexpr ColourRGBA Grey(unsigned int grey, unsigned int alpha=maximumByte) noexcept {
		return ColourRGBA(grey, grey, grey, alpha);
	}

	static constexpr ColourRGBA FromIpRGB(intptr_t co_) noexcept {
		const int rgb = co_ & rgbMask;
		return ColourRGBA(rgb | (maximumByte << aShift));
	}

	[[nodiscard]] constexpr ColourRGBA WithoutAlpha() const noexcept {
		return ColourRGBA(co & rgbMask);
	}

	[[nodiscard]] constexpr ColourRGBA Opaque() const noexcept {
		return ColourRGBA(co | (maximumByte << aShift));
	}

	[[nodiscard]] constexpr int AsInteger() const noexcept {
		return co;
	}

	[[nodiscard]] constexpr int OpaqueRGB() const noexcept {
		return co & rgbMask;
	}

	// Red, green and blue values as bytes 0..255
	[[nodiscard]] constexpr unsigned char GetRed() const noexcept {
		return co & maximumByte;
	}
	[[nodiscard]] constexpr unsigned char GetGreen() const noexcept {
		return (co >> gShift) & maximumByte;
	}
	[[nodiscard]] constexpr unsigned char GetBlue() const noexcept {
		return (co >> bShift) & maximumByte;
	}
	[[nodiscard]] constexpr unsigned char GetAlpha() const noexcept {
		// Use a temporary here to prevent a 'Wconversion' warning from GCC
		const int shifted = co >> aShift;
		return shifted & maximumByte;
	}

	// Red, green, blue, and alpha values as float 0..1.0
	[[nodiscard]] constexpr float GetRedComponent() const noexcept {
		return ComponentAsFloat(GetRed());
	}
	[[nodiscard]] constexpr float GetGreenComponent() const noexcept {
		return ComponentAsFloat(GetGreen());
	}
	[[nodiscard]] constexpr float GetBlueComponent() const noexcept {
		return ComponentAsFloat(GetBlue());
	}
	[[nodiscard]] constexpr float GetAlphaComponent() const noexcept {
		return ComponentAsFloat(GetAlpha());
	}

	[[nodiscard]] constexpr bool operator==(const ColourRGBA &other) const noexcept {
		return co == other.co;
	}
	[[nodiscard]] constexpr bool operator!=(const ColourRGBA &other) const noexcept {
		return co != other.co;
	}

	[[nodiscard]] constexpr bool IsOpaque() const noexcept {
		return GetAlpha() == maximumByte;
	}

	[[nodiscard]] ColourRGBA MixedWith(ColourRGBA other) const noexcept;
	[[nodiscard]] ColourRGBA MixedWith(ColourRGBA other, double proportion) const noexcept;
};

constexpr ColourRGBA white(maximumByte, maximumByte, maximumByte);
constexpr ColourRGBA black(0x0, 0x0, 0x0);

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
	[[nodiscard]] constexpr float WidthF() const noexcept {
		return static_cast<float>(width);
	}
};

/**
* Holds an RGBA colour to fill a shape.
*/
class Fill {
public:
	ColourRGBA colour;
	// Not explicit as used very often from simple contexts so would just be noise.
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
