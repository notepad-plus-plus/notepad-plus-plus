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

#ifndef XMLMATCHEDTAGSHIGHLIGHTER_H
#define XMLMATCHEDTAGSHIGHLIGHTER_H

#include <vector>
#include <windows.h>

using namespace std;

class ScintillaEditView;

enum TagCateg {tagOpen, tagClose, inSingleTag, outOfTag, invalidTag, unknownPb};

class XmlMatchedTagsHighlighter {
public:
	XmlMatchedTagsHighlighter(ScintillaEditView *pEditView):_pEditView(pEditView){};
	void tagMatch(bool doHiliteAttr);
	
private:
	struct XmlMatchedTagsPos {
		int tagOpenStart;
		int tagNameEnd;
		int tagOpenEnd;

		int tagCloseStart;
		int tagCloseEnd;
	};
	
	ScintillaEditView *_pEditView;

	int getFirstTokenPosFrom(int targetStart, int targetEnd, const char *token, std::pair<int, int> & foundPos);
	TagCateg getTagCategory(XmlMatchedTagsPos & tagsPos, int curPos);
	bool getMatchedTagPos(int searchStart, int searchEnd, const char *tag2find, const char *oppositeTag2find, vector<int> oppositeTagFound, XmlMatchedTagsPos & tagsPos);
	bool getXmlMatchedTagsPos(XmlMatchedTagsPos & tagsPos);
	vector< pair<int, int> > getAttributesPos(int start, int end);
	bool isInList(int element, vector<int> elementList) {
		for (size_t i = 0 ; i < elementList.size() ; i++)
			if (element == elementList[i])
				return true;
		return false;
	};
};

#endif //XMLMATCHEDTAGSHIGHLIGHTER_H

