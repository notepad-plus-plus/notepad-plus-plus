//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "precompiledHeaders.h"
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
	for (size_t i = 0 ; i < _scintVector.size() ; i++)
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

	vector<ScintillaEditView *>::iterator it2delete = _scintVector.begin()+ i;
	_scintVector.erase(it2delete);
	return true;
}

void ScintillaCtrls::destroy() 
{
	for (size_t i = 0 ; i < _scintVector.size() ; i++)
	{
		_scintVector[i]->destroy();
		delete _scintVector[i];
	}
}
