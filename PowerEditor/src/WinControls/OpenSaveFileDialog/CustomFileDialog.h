// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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


#pragma once

#include "Common.h"
#include <memory>

// Customizable file dialog.
// It allows adding custom controls like checkbox to the dialog.
// This class loosely follows the interface of the FileDialog class.
// However, the implementation is different.
class CustomFileDialog
{
public:
	explicit CustomFileDialog(HWND hwnd);
	~CustomFileDialog();
	void setTitle(const TCHAR* title);
	void setExtFilter(const TCHAR* text, const TCHAR* ext);
	void setDefExt(const TCHAR* ext);
	void setFolder(const TCHAR* folder);
	void setCheckbox(const TCHAR* text, bool isActive = true);
	void setExtIndex(int extTypeIndex);

	const TCHAR* doSaveDlg();
	const TCHAR* pickFolder();

	bool getCheckboxState() const;

private:
	class Impl;
	std::unique_ptr<Impl> _impl;
};
