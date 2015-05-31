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


#ifndef LASTRECENTFILELIST_H
#define LASTRECENTFILELIST_H

#ifndef PARAMETERS_H
#include "Parameters.h"
#endif //PARAMETERS_H

#include <deque>

struct RecentItem {
	int _id;
	generic_string _name;
	RecentItem(const TCHAR * name) : _name(name) {};
};

typedef std::deque<RecentItem> recentList;

class LastRecentFileList
{
public:
	LastRecentFileList() : _hasSeparators(false), _size(0), _locked(false) {
		_userMax = (NppParameters::getInstance())->getNbMaxRecentFile();
	};

	void initMenu(HMENU hMenu, int idBase, int posBase, Accelerator *accelerator, bool doSubMenu = false);
	void switchMode();
	void updateMenu();

	void add(const TCHAR *fn);
	void remove(const TCHAR *fn);
	void remove(int index);
	void clear();

	int getSize() {
		return _size;
	};


	int getMaxNbLRF() const {
		return NB_MAX_LRF_FILE;
	};

	int getUserMaxNbLRF() const {
		return _userMax;
	};

	generic_string & getItem(int id);	//use menu id
	generic_string & getIndex(int index);	//use menu id

	generic_string getFirstItem() const {
		if (_lrfl.size() == 0)
			return TEXT("");
		return _lrfl.front()._name;
	};

	void setUserMaxNbLRF(int size);

	void saveLRFL();

	void setLock(bool lock) {
		_locked = lock;
	};

	void setLangEncoding(int nativeLangEncoding) {
		_nativeLangEncoding = nativeLangEncoding;
	};

	bool isSubMenuMode() const {
		return (_hParentMenu != NULL);
	};

private:
	recentList _lrfl;
	Accelerator *_pAccelerator;
	int _userMax;
	int _size;
	int _nativeLangEncoding;

	// For the menu
	HMENU _hParentMenu;
	HMENU _hMenu;
	int _posBase;
	int _idBase;
	bool _idFreeArray[NB_MAX_LRF_FILE];
	bool _hasSeparators;
	bool _locked;

	int find(const TCHAR *fn);

	int popFirstAvailableID();
	void setAvailable(int id);
};

#endif //LASTRECENTFILELIST_H
