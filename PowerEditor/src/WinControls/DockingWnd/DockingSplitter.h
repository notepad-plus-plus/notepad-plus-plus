// This file is part of Notepad++ project
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#include "Window.h"
#include "Docking.h"
#include "dockingResource.h"

#define	DMS_VERTICAL		0x00000001
#define	DMS_HORIZONTAL		0x00000002

class DockingSplitter : public Window
{
public :
	DockingSplitter() = default;
	~DockingSplitter() = default;

	virtual void destroy() {};

public:
	void init(HINSTANCE hInst, HWND hWnd, HWND hMessage, UINT flags);

protected:

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND _hMessage = nullptr;

	BOOL _isLeftButtonDown = FALSE;
	POINT _ptOldPos = {0, 0};
	UINT _flags = 0;

	static BOOL _isVertReg;
	static BOOL _isHoriReg;

	// get layout direction
	bool _isRTL = false;
};

