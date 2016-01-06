// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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


#include "ScintillaCtrls.h"
#include "ScintillaEditView.h"

HWND ScintillaCtrls::createSintilla(HWND hParent) 
{
	_hParent = hParent;
	ScintillaEditView *scint = new ScintillaEditView;
	scint->init(_hInst, _hParent);
	_scintVector.push_back(scint);
	return scint->getHSelf();
}

int ScintillaCtrls::getIndexFrom(HWND handle2Find)
{
	for (size_t i = 0, len = _scintVector.size(); i < len ; ++i)
	{
		if (_scintVector[i]->getHSelf() == handle2Find)
		{
			return i;
		}
	}
	return -1;
}

ScintillaEditView * ScintillaCtrls::getScintillaEditViewFrom(HWND handle2Find)
{
	int i = getIndexFrom(handle2Find);
	if (i == -1 || size_t(i) >= _scintVector.size())
		return NULL;
	return _scintVector[i];
}

bool ScintillaCtrls::destroyScintilla(HWND handle2Destroy)
{
	int i = getIndexFrom(handle2Destroy);
	if (i == -1)
		return false;
	
	_scintVector[i]->destroy();
	delete _scintVector[i];

	std::vector<ScintillaEditView *>::iterator it2delete = _scintVector.begin()+ i;
	_scintVector.erase(it2delete);
	return true;
}

void ScintillaCtrls::destroy() 
{
	for (size_t i = 0, len = _scintVector.size(); i < len ; ++i)
	{
		_scintVector[i]->destroy();
		delete _scintVector[i];
	}
}
