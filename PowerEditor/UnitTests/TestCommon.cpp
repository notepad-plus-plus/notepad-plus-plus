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
#include "Sorters.h"
#include "StringUtils.h"

TEST(TestCommon, LexSorter)
{
	const bool isDescending = false;
	const size_t fromColumn = 0;
	const size_t toColumn = 100;
	LexicographicSorter sorter(isDescending, fromColumn, toColumn);
	const std::vector<generic_string> resultLines = sorter.sort({ _T("b"), _T("c"), _T("a")});

	const std::vector<generic_string> expectedLines = { _T("a"), _T("b"), _T("c") };
	EXPECT_EQ(resultLines, expectedLines);
}

TEST(TestCommon, EndsWith)
{
	EXPECT_TRUE(endsWith(_T(""), _T("")));
	EXPECT_TRUE(endsWith(_T("a1"), _T("1")));

	EXPECT_FALSE(endsWith(_T("1"), _T("")));
	EXPECT_FALSE(endsWith(_T(""), _T("1")));
	EXPECT_FALSE(endsWith(_T("a"), _T("1")));
}

TEST(TestCommon, ExpandEnv)
{
	generic_string str = _T("%OS%");
	expandEnv(str);
	EXPECT_NE(str, _T(""));
	EXPECT_NE(str, _T("%OS%"));
	EXPECT_NE(str.find(_T("Windows")), generic_string::npos);
}
