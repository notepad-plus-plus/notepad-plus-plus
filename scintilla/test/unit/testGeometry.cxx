/** @file testGeometry.cxx
 ** Unit Tests for Scintilla internal data structures
 **/

#include <cstdint>

#include "Geometry.h"

#include "catch.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

// Test Geometry.

TEST_CASE("Point") {

	SECTION("Point") {
		constexpr Point pt(1.0, 2.0);
		REQUIRE(pt.x == 1.0);
		REQUIRE(pt.y == 2.0);

		constexpr Point pti = Point::FromInts(1, 2);
		REQUIRE(pt == pti);

		constexpr Point pt2 = pt + pt;
		REQUIRE(pt != pt2);

		constexpr Point ptBack = pt2 - pt;
		REQUIRE(pt == ptBack);
	}

}

TEST_CASE("Interval") {

	SECTION("Interval") {
		constexpr Interval interval { 1.0, 2.0 };
		REQUIRE(interval.left == 1.0);
		REQUIRE(interval.right == 2.0);
		REQUIRE(interval.Width() == 1.0);
		REQUIRE(!interval.Empty());

		constexpr Interval empty { 4.0, 4.0 };
		REQUIRE(empty.Empty());
		REQUIRE(empty.Width() == 0.0);
		REQUIRE(!(interval == empty));
		REQUIRE(!(interval.Intersects(empty)));

		constexpr Interval inside { 1.7, 1.8 };
		REQUIRE(interval.Intersects(inside));
		constexpr Interval straddles { 1.7, 2.8 };
		REQUIRE(interval.Intersects(straddles));
	}

	SECTION("Interval") {
		constexpr Interval interval1{ 1.0, 3.0 };
		constexpr Interval interval2{ 2.0, 4.0 };
		REQUIRE(Intersection(interval1, interval2) == Interval{ 2.0, 3.0 });
	}
}

TEST_CASE("PRectangle") {

	SECTION("PRectangle") {
		constexpr PRectangle rc(1.0, 2.0, 3.0, 4.0);
		REQUIRE(rc.left == 1.0);
		REQUIRE(rc.top == 2.0);
		REQUIRE(rc.right == 3.0);
		REQUIRE(rc.bottom == 4.0);
		REQUIRE(rc.Width() == 2.0);
		REQUIRE(rc.Height() == 2.0);
		REQUIRE(!rc.Empty());

		constexpr PRectangle rci = PRectangle::FromInts(1, 2, 3, 4);
		REQUIRE(rc == rci);

		constexpr PRectangle rcEmpty;
		REQUIRE(rcEmpty.Empty());
	}

	SECTION("Contains") {
		constexpr PRectangle rc(1.0, 2.0, 3.0, 4.0);
		constexpr Point pt(1.5, 2.5);
		REQUIRE(rc.Contains(pt));
		REQUIRE(rc.ContainsWholePixel(pt));
		constexpr Point ptNearEdge(2.9, 2.5);
		REQUIRE(rc.Contains(ptNearEdge));
		REQUIRE(!rc.ContainsWholePixel(ptNearEdge));
		constexpr Point ptOutside(1.5, 20.5);
		REQUIRE(!rc.Contains(ptOutside));
		REQUIRE(!rc.ContainsWholePixel(ptOutside));

		constexpr PRectangle rcInside(1.5, 2.0, 2.5, 4.0);
		REQUIRE(rc.Contains(rcInside));
		REQUIRE(rc.Intersects(rcInside));

		constexpr PRectangle rcIntersects(1.5, 2.0, 3.5, 4.0);
		REQUIRE(!rc.Contains(rcIntersects));
		REQUIRE(rc.Intersects(rcIntersects));

		constexpr PRectangle rcSeparate(11.0, 12.0, 13.0, 14.0);
		REQUIRE(!rc.Contains(rcSeparate));
		REQUIRE(!rc.Intersects(rcSeparate));

		constexpr Point ptCentre = rc.Centre();
		REQUIRE(ptCentre == Point(2.0, 3.0));
	}

	SECTION("Move") {
		PRectangle rc(1.0, 2.0, 3.0, 4.0);
		rc.Move(1.0, 10.0);
		REQUIRE(rc == PRectangle(2.0, 12.0, 4.0, 14.0));
	}

	SECTION("Inset") {
		constexpr PRectangle rc(1.0, 2.0, 3.0, 4.0);
		constexpr PRectangle rcInset = rc.Inset(0.5);
		REQUIRE(rcInset == PRectangle(1.5, 2.5, 2.5, 3.5));

		constexpr PRectangle rcInsetPt = rc.Inset(Point(0.25, 0.5));
		REQUIRE(rcInsetPt == PRectangle(1.25, 2.5, 2.75, 3.5));
	}

	SECTION("Clamp") {
		constexpr PRectangle rc(1.0, 2.0, 3.0, 4.0);

		const PRectangle cutBottom = Clamp(rc, Edge::bottom, 3.5);
		REQUIRE(cutBottom == PRectangle(1.0, 2.0, 3.0, 3.5));

		const PRectangle justBottom = Side(rc, Edge::bottom, 0.5);
		REQUIRE(justBottom == PRectangle(1.0, 3.5, 3.0, 4.0));

		constexpr PRectangle rcInset = rc.Inset(0.5);
		REQUIRE(rcInset == PRectangle(1.5, 2.5, 2.5, 3.5));

		constexpr PRectangle rcInsetPt = rc.Inset(Point(0.25, 0.5));
		REQUIRE(rcInsetPt == PRectangle(1.25, 2.5, 2.75, 3.5));

		const Interval bounds = HorizontalBounds(rcInsetPt);
		REQUIRE(bounds == Interval{ 1.25, 2.75 });

		const PRectangle applyBounds = Intersection(rc, bounds);
		REQUIRE(applyBounds == PRectangle(1.25, 2.0, 2.75, 4.0));
	}

	SECTION("PixelAlign") {
		// Whole pixels
		REQUIRE(PixelAlign(1.0, 1) == 1.0);
		REQUIRE(PixelAlignFloor(1.0, 1) == 1.0);
		REQUIRE(PixelAlignCeil(1.0, 1) == 1.0);

		REQUIRE(PixelAlign(1.25, 1) == 1.0);
		REQUIRE(PixelAlignFloor(1.25, 1) == 1.0);
		REQUIRE(PixelAlignCeil(1.25, 1) == 2.0);

		REQUIRE(PixelAlign(1.5, 1) == 2.0);
		REQUIRE(PixelAlignFloor(1.5, 1) == 1.0);
		REQUIRE(PixelAlignCeil(1.5, 1) == 2.0);

		REQUIRE(PixelAlign(1.75, 1) == 2.0);
		REQUIRE(PixelAlignFloor(1.75, 1) == 1.0);
		REQUIRE(PixelAlignCeil(1.75, 1) == 2.0);

		REQUIRE(PixelAlign(Point(1.75, 1.25), 1) == Point(2.0, 1.0));
		REQUIRE(PixelAlign(Point(1.5, 1.0), 1) == Point(2.0, 1.0));

		// Half pixels
		REQUIRE(PixelAlign(1.0, 2) == 1.0);
		REQUIRE(PixelAlignFloor(1.0, 2) == 1.0);
		REQUIRE(PixelAlignCeil(1.0, 2) == 1.0);

		REQUIRE(PixelAlign(1.25, 2) == 1.5);
		REQUIRE(PixelAlignFloor(1.25, 2) == 1.0);
		REQUIRE(PixelAlignCeil(1.25, 2) == 1.5);

		REQUIRE(PixelAlign(1.5, 2) == 1.5);
		REQUIRE(PixelAlignFloor(1.5, 2) == 1.5);
		REQUIRE(PixelAlignCeil(1.5, 2) == 1.5);

		REQUIRE(PixelAlign(1.75, 2) == 2.0);
		REQUIRE(PixelAlignFloor(1.75, 2) == 1.5);
		REQUIRE(PixelAlignCeil(1.75, 2) == 2.0);

		REQUIRE(PixelAlign(Point(1.75, 1.25), 2) == Point(2.0, 1.5));
		REQUIRE(PixelAlign(Point(1.5, 1.0), 2) == Point(1.5, 1.0));

		// x->round, y->floored
		REQUIRE(PixelAlign(PRectangle(1.0, 1.25, 1.5, 1.75), 1) == PRectangle(1.0, 1.0, 2.0, 1.0));
		REQUIRE(PixelAlign(PRectangle(1.0, 1.25, 1.5, 1.75), 2) == PRectangle(1.0, 1.0, 1.5, 1.5));

		// x->outside(floor left, ceil right), y->floored
		REQUIRE(PixelAlignOutside(PRectangle(1.1, 1.25, 1.6, 1.75), 1) == PRectangle(1.0, 1.0, 2.0, 1.0));
		REQUIRE(PixelAlignOutside(PRectangle(1.1, 1.25, 1.6, 1.75), 2) == PRectangle(1.0, 1.0, 2.0, 1.5));
	}

}

TEST_CASE("ColourRGBA") {

	SECTION("ColourRGBA") {
		constexpr ColourRGBA colour(0x10203040);
		constexpr ColourRGBA colourFromRGB(0x40, 0x30, 0x20, 0x10);

		REQUIRE(colour == colourFromRGB);
		REQUIRE(colour.GetRed() == 0x40);
		REQUIRE(colour.GetGreen() == 0x30);
		REQUIRE(colour.GetBlue() == 0x20);
		REQUIRE(colour.GetAlpha() == 0x10);
		REQUIRE(!colour.IsOpaque());
		REQUIRE(colour.AsInteger() == 0x10203040);

		REQUIRE(ColourRGBA(colour, 0x80) == ColourRGBA(0x40, 0x30, 0x20, 0x80));
		REQUIRE(ColourRGBA::FromRGB(0x203040) == ColourRGBA(0x40, 0x30, 0x20, 0xff));
		REQUIRE(ColourRGBA::FromIpRGB(0x203040) == ColourRGBA(0x40, 0x30, 0x20, 0xff));

		constexpr ColourRGBA colour01(0x00ff00ff);
		REQUIRE(colour01.GetRedComponent() == 1.0);
		REQUIRE(colour01.GetGreenComponent() == 0.0);
		REQUIRE(colour01.GetBlueComponent() == 1.0);
		REQUIRE(colour01.GetAlphaComponent() == 0.0);

		// Opaque colours
		constexpr ColourRGBA colourRGB(0xff203040);
		REQUIRE(colourRGB.IsOpaque());
		constexpr ColourRGBA colourOpaque(0x40, 0x30, 0x20, 0xff);
		REQUIRE(colourRGB == colourOpaque);

		REQUIRE(colourRGB.OpaqueRGB() == 0x203040);
		REQUIRE(colourRGB.WithoutAlpha() == ColourRGBA(0x40, 0x30, 0x20, 0));
	}

	SECTION("Mixing") {
		constexpr ColourRGBA colourMin(0x10, 0x20, 0x30, 0x40);
		constexpr ColourRGBA colourMax(0x30, 0x60, 0x90, 0xC0);

		constexpr ColourRGBA colourMid(0x20, 0x40, 0x60, 0x80);
		REQUIRE(colourMin.MixedWith(colourMax) == colourMid);
		REQUIRE(colourMin.MixedWith(colourMax, 0.5) == colourMid);

		// 3/4 between min and max
		constexpr ColourRGBA colour75(0x28, 0x50, 0x78, 0xA0);
		REQUIRE(colourMin.MixedWith(colourMax, 0.75) == colour75);
	}

}
