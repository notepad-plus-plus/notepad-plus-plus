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
			return static_cast<int32_t>(i);
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

/*
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
*/

void ScintillaCtrls::destroy() 
{
	for (size_t i = 0, len = _scintVector.size(); i < len ; ++i)
	{
		_scintVector[i]->destroy();
		delete _scintVector[i];
	}
}
