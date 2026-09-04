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
	void setTitle(const wchar_t* title);
	void setExtFilter(const wchar_t* text, const wchar_t* ext);
	void setExtFilter(const wchar_t* text, std::initializer_list<const wchar_t*> exts);
	void setDefExt(const wchar_t* ext);
	void setDefFileName(const wchar_t *fn);
	void setFolder(const wchar_t* folder);
	void setCheckbox(const wchar_t* text, bool isActive = true);
	void setExtIndex(int extTypeIndex);
	void setSaveAsCopy(bool isSavingAsCopy);
	bool getOpenTheCopyAfterSaveAsCopy();

	void enableFileTypeCheckbox(const std::wstring& text, bool value);
	bool getFileTypeCheckboxValue() const;

	// Empty string is not a valid file name and may signal that the dialog was canceled.
	std::wstring doSaveDlg();
	std::wstring pickFolder();
	std::wstring doOpenSingleFileDlg();
	std::vector<std::wstring> doOpenMultiFilesDlg();

	bool getCheckboxState() const;
	bool isReadOnly() const;

private:
	class Impl;
	std::unique_ptr<Impl> _impl;
};
