// Scintilla source code edit control
/** @file IntegerRectangle.h
 ** A rectangle with integer coordinates.
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef INTEGERRECTANGLE_H
#define INTEGERRECTANGLE_H

namespace Scintilla {

struct IntegerRectangle {
	int left;
	int top;
	int right;
	int bottom;

	explicit IntegerRectangle(PRectangle rc) noexcept :
		left(static_cast<int>(rc.left)), top(static_cast<int>(rc.top)),
		right(static_cast<int>(rc.right)), bottom(static_cast<int>(rc.bottom)) {
	}
	int Width() const noexcept { return right - left; }
	int Height() const noexcept { return bottom - top; }
};

}

#endif
