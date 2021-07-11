// This file is part of Notepad++ project
// Copyright (C) 2021 The Notepad++ Contributors.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "pch.h"
#include "FS.h"

TEST(TestFS, GetFirstExtension)
{
	EXPECT_EQ(get1stExt(_T("")), _T(""));
	EXPECT_EQ(get1stExt(_T("abc")), _T(""));
	EXPECT_EQ(get1stExt(_T(".c")), _T(".c"));
	EXPECT_EQ(get1stExt(_T(".c;.cpp;.h")), _T(".c"));
}

TEST(TestFS, ReplaceExtension)
{
	generic_string name;
	EXPECT_FALSE(replaceExt(name, _T("")));
	EXPECT_EQ(name, _T(""));

	EXPECT_FALSE(replaceExt(name, _T("a")));
	EXPECT_EQ(name, _T(""));

	name = _T("a");
	EXPECT_TRUE(replaceExt(name, _T("b")));
	EXPECT_EQ(name, _T("ab"));

	name = _T("a.");
	EXPECT_TRUE(replaceExt(name, _T("b")));
	EXPECT_EQ(name, _T("ab"));

	name = _T("a");
	EXPECT_TRUE(replaceExt(name, _T(".b")));
	EXPECT_EQ(name, _T("a.b"));

	name = _T("a.txt");
	EXPECT_TRUE(replaceExt(name, _T(".b")));
	EXPECT_EQ(name, _T("a.b"));

	name = _T("a.txt");
	EXPECT_TRUE(replaceExt(name, _T("b")));
	EXPECT_EQ(name, _T("ab"));
}

TEST(TestFS, HasExtension)
{
	EXPECT_FALSE(hasExt(_T("")));
	EXPECT_FALSE(hasExt(_T("a")));
	EXPECT_FALSE(hasExt(_T("abc")));
	EXPECT_FALSE(hasExt(_T("abc_d")));

	EXPECT_TRUE(hasExt(_T(".a")));
	EXPECT_TRUE(hasExt(_T("a.b")));
	EXPECT_TRUE(hasExt(_T("a.b.tmp")));
}