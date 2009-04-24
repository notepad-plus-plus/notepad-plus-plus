//this file is part of notepad++
//Copyright (C)2003 Harry <harrybharry@users.sourceforge.net>
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

#ifndef SMARTHIGHLIGHTER_H
#define SMARTHIGHLIGHTER_H

#include "ScintillaEditView.h"
#include "FindReplaceDlg.h"

class SmartHighlighter {
public:
	SmartHighlighter(FindReplaceDlg * pFRDlg);
	void highlightView(ScintillaEditView * pHighlightView);
private:
	FindReplaceDlg * _pFRDlg;

	bool isQualifiedWord(const char *str) const;
	bool isWordChar(char ch) const;
};

#endif //SMARTHIGHLIGHTER_H