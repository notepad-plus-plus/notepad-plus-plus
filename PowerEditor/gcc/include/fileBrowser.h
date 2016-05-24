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


#ifndef FILEBROWSER_H
#define  FILEBROWSER_H

#warning FileBrowser stub used for MinGW/GCC because ATL is not supported

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "TreeView.h"
#include "fileBrowser_rc.h"

#define FB_PANELTITLE     TEXT("Folder as Workspace")
#define FB_REMOVEROOT              TEXT("Remove")

class FileBrowser : public DockingDlgInterface {
public:
	FileBrowser(): DockingDlgInterface(IDD_FILEBROWSER) {};
	~FileBrowser() {};
	void addRootFolder(generic_string) {};

	std::vector<generic_string> getRoots() const {
		std::vector<generic_string> roots;
		return roots;
	};
	generic_string getSelectedItemPath() const {
		generic_string itemPath;
		return itemPath;
	};
};

#endif // FILEBROWSER_H
