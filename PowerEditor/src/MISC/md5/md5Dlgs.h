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

#include "StaticDialog.h"

enum hashType {hash_md5 = 16, hash_sha1 = 20, hash_sha256 = 32, hash_sha512 = 64};

#define HASH_MAX_LENGTH hash_sha512
#define HASH_STR_MAX_LENGTH (hash_sha512 * 2 + 1)

LRESULT run_textEditProc(WNDPROC oldEditProc, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class HashFromFilesDlg : public StaticDialog
{
public :
	HashFromFilesDlg() = default;

	void doDialog(bool isRTL = false);
	void destroy() override;
	void setHashType(hashType hashType2set);

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	hashType _ht = hash_md5;

	static LRESULT CALLBACK HashPathEditStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		const auto dlg = (HashFromFilesDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return (run_textEditProc(dlg->_oldHashPathEditProc, hwnd, message, wParam, lParam));
	};

	static LRESULT CALLBACK HashResultStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		const auto dlg = (HashFromFilesDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return (run_textEditProc(dlg->_oldHashResultProc, hwnd, message, wParam, lParam));
	};

private :
	WNDPROC _oldHashPathEditProc = nullptr;
	WNDPROC _oldHashResultProc = nullptr;
	HFONT _hFont = nullptr;
};

class HashFromTextDlg : public StaticDialog
{
public :
	HashFromTextDlg() = default;

	void doDialog(bool isRTL = false);
	void destroy() override;
	void generateHash();
	void generateHashPerLine();
	void setHashType(hashType hashType2set);

protected :
	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	hashType _ht = hash_md5;

	static LRESULT CALLBACK HashTextEditStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		const auto dlg = (HashFromTextDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return (run_textEditProc(dlg->_oldHashTextEditProc, hwnd, message, wParam, lParam));
	};

	static LRESULT CALLBACK HashResultStaticProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		const auto dlg = (HashFromTextDlg *)(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return (run_textEditProc(dlg->_oldHashResultProc, hwnd, message, wParam, lParam));
	};

private :
	WNDPROC _oldHashTextEditProc = nullptr;
	WNDPROC _oldHashResultProc = nullptr;
	HFONT _hFont = nullptr;
};
