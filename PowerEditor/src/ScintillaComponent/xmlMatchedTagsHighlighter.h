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


#pragma once

#include <windows.h>
#include <string>
#include <vector>

class ScintillaEditView;


class XmlMatchedTagsHighlighter {
public:
	explicit XmlMatchedTagsHighlighter(ScintillaEditView *pEditView):_pEditView(pEditView){};
	void tagMatch(bool doHiliteAttr);
	
private:
	ScintillaEditView* _pEditView = nullptr;
	
	struct XmlMatchedTagsPos {
		intptr_t tagOpenStart = 0;
		intptr_t tagNameEnd = 0;
		intptr_t tagOpenEnd = 0;

		intptr_t tagCloseStart = 0;
		intptr_t tagCloseEnd = 0;
	};

	struct FindResult {
		intptr_t start = 0;
		intptr_t end = 0;
		bool success = false;
	};
	
	bool getXmlMatchedTagsPos(XmlMatchedTagsPos & tagsPos);

	// Allowed whitespace characters in XML
	bool isWhitespace(intptr_t ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }

	FindResult findText(const char *text, intptr_t start, intptr_t end, int flags = 0);
	FindResult findOpenTag(const std::string& tagName, intptr_t start, intptr_t end);
	FindResult findCloseTag(const std::string& tagName, intptr_t start, intptr_t end);
	intptr_t findCloseAngle(intptr_t startPosition, intptr_t endPosition);
	
	std::vector< std::pair<intptr_t, intptr_t> > getAttributesPos(intptr_t start, intptr_t end);
	
};


