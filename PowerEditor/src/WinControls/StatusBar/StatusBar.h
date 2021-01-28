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

#ifndef _WIN32_IE
#define _WIN32_IE	0x0600
#endif //_WIN32_IE

#include "Window.h"
#include "Common.h"
#include <vector>



class StatusBar final : public Window
{
public:
	virtual ~StatusBar();

	void init(HINSTANCE hInst, HWND hPere, int nbParts);

	bool setPartWidth(int whichPart, int width);

	virtual void destroy() override;
    virtual void reSizeTo(const RECT& rc);

	int getHeight() const;

    bool setText(const TCHAR* str, int whichPart);
	bool setOwnerDrawText(const TCHAR* str);
	void adjustParts(int clientWidth);


private:
	virtual void init(HINSTANCE hInst, HWND hPere) override;

private:
    std::vector<int> _partWidthArray;
	int *_lpParts = nullptr;
	generic_string _lastSetText;
};