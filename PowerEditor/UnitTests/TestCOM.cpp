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
#include "COM.h"

class TestCOM : public ::testing::Test
{
public:
	// Sets up the stuff shared by all tests in this test case.
	static void SetUpTestCase()
	{
		HRESULT hr = CoInitialize(NULL);
		EXPECT_EQ(hr, S_OK);
	}

	// Tears down the stuff shared by all tests in this test case.
	static void TearDownTestCase()
	{
		CoUninitialize();
	}
};

TEST_F(TestCOM, ShellGetFileName)
{
	com_ptr<IShellItem> shellItem;
	EXPECT_EQ(getFilename(shellItem), _T(""));

	TCHAR path[MAX_PATH] = {};
	EXPECT_NE(::GetCurrentDirectory(static_cast<DWORD>(std::size(path)), path), 0U);
	EXPECT_GT(_tcslen(path), 0U);

	HRESULT hr = SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&shellItem));
	EXPECT_EQ(hr, S_OK);
	ASSERT_TRUE(shellItem);

	const generic_string name = getFilename(shellItem);
	EXPECT_EQ(_tcsicmp(name.data(), path), 0) << name << " != " << path;
}
