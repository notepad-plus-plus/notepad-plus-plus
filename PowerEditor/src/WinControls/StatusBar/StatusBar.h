// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid
// misunderstandings, we consider an application to constitute a
// "derivative work" for the purpose of this license if it does any of the
// following:
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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