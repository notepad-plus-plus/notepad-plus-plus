// this file is part of docking functionality for Notepad++
// Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
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


#ifndef DOCKINGSPLITTER_H
#define DOCKINGSPLITTER_H

#include "Window.h"

#ifndef DOCKING_H
#include "Docking.h"
#endif //DOCKING_H

#ifndef DOCKING_RESOURCE_H
#include "dockingResource.h"
#endif //DOCKING_RESOURCE_H

#define	DMS_VERTICAL		0x00000001
#define	DMS_HORIZONTAL		0x00000002

class DockingSplitter : public Window
{
public :
	DockingSplitter() : _isLeftButtonDown(FALSE), _hMessage(NULL) {};
	~DockingSplitter(){};

	virtual void destroy() {};

public:
	void init(HINSTANCE hInst, HWND hWnd, HWND hMessage, UINT flags);

protected:

	static LRESULT CALLBACK staticWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT runProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	HWND				_hMessage;

	BOOL				_isLeftButtonDown;
	POINT				_ptOldPos;
	UINT				_flags;

	static BOOL			_isVertReg;
	static BOOL			_isHoriReg;
};

#endif // DOCKINGSPLITTER_H
