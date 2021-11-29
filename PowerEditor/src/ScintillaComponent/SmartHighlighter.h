// This file is part of Notepad++ project
// Copyright (C)2008 Harry Bruin <harrybharry@users.sourceforge.net>

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

#include "Common.h"

class ScintillaEditView;
class FindReplaceDlg;

class SmartHighlighter {
public:
	explicit SmartHighlighter(FindReplaceDlg * pFRDlg);
	void highlightView(ScintillaEditView * pHighlightView, ScintillaEditView * unfocusView);
	void highlightViewWithWord(ScintillaEditView * pHighlightView, const generic_string & word2Hilite);

private:
	FindReplaceDlg * _pFRDlg = nullptr;
};
