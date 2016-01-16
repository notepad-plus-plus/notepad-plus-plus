// Scintilla source code edit control
/** @file AutoComplete.cxx
 ** Defines the auto completion list box.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

#include "Platform.h"

#include "Scintilla.h"
#include "CharacterSet.h"
#include "Position.h"
#include "AutoComplete.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

AutoComplete::AutoComplete() :
	active(false),
	separator(' '),
	typesep('?'),
	ignoreCase(false),
	chooseSingle(false),
	lb(0),
	posStart(0),
	startLen(0),
	cancelAtStartPos(true),
	autoHide(true),
	dropRestOfWord(false),
	ignoreCaseBehaviour(SC_CASEINSENSITIVEBEHAVIOUR_RESPECTCASE),
	widthLBDefault(100),
	heightLBDefault(100),
	autoSort(SC_ORDER_PRESORTED) {
	lb = ListBox::Allocate();
}

AutoComplete::~AutoComplete() {
	if (lb) {
		lb->Destroy();
		delete lb;
		lb = 0;
	}
}

bool AutoComplete::Active() const {
	return active;
}

void AutoComplete::Start(Window &parent, int ctrlID,
	int position, Point location, int startLen_,
	int lineHeight, bool unicodeMode, int technology) {
	if (active) {
		Cancel();
	}
	lb->Create(parent, ctrlID, location, lineHeight, unicodeMode, technology);
	lb->Clear();
	active = true;
	startLen = startLen_;
	posStart = position;
}

void AutoComplete::SetStopChars(const char *stopChars_) {
	stopChars = stopChars_;
}

bool AutoComplete::IsStopChar(char ch) {
	return ch && (stopChars.find(ch) != std::string::npos);
}

void AutoComplete::SetFillUpChars(const char *fillUpChars_) {
	fillUpChars = fillUpChars_;
}

bool AutoComplete::IsFillUpChar(char ch) {
	return ch && (fillUpChars.find(ch) != std::string::npos);
}

void AutoComplete::SetSeparator(char separator_) {
	separator = separator_;
}

char AutoComplete::GetSeparator() const {
	return separator;
}

void AutoComplete::SetTypesep(char separator_) {
	typesep = separator_;
}

char AutoComplete::GetTypesep() const {
	return typesep;
}

struct Sorter {
	AutoComplete *ac;
	const char *list;
	std::vector<int> indices;

	Sorter(AutoComplete *ac_, const char *list_) : ac(ac_), list(list_) {
		int i = 0;
		while (list[i]) {
			indices.push_back(i); // word start
			while (list[i] != ac->GetTypesep() && list[i] != ac->GetSeparator() && list[i])
				++i;
			indices.push_back(i); // word end
			if (list[i] == ac->GetTypesep()) {
				while (list[i] != ac->GetSeparator() && list[i])
					++i;
			}
			if (list[i] == ac->GetSeparator()) {
				++i;
				// preserve trailing separator as blank entry
				if (!list[i]) {
					indices.push_back(i);
					indices.push_back(i);
				}
			}
		}
		indices.push_back(i); // index of last position
	}

	bool operator()(int a, int b) {
		int lenA = indices[a * 2 + 1] - indices[a * 2];
		int lenB = indices[b * 2 + 1] - indices[b * 2];
		int len  = std::min(lenA, lenB);
		int cmp;
		if (ac->ignoreCase)
			cmp = CompareNCaseInsensitive(list + indices[a * 2], list + indices[b * 2], len);
		else
			cmp = strncmp(list + indices[a * 2], list + indices[b * 2], len);
		if (cmp == 0)
			cmp = lenA - lenB;
		return cmp < 0;
	}
};

void AutoComplete::SetList(const char *list) {
	if (autoSort == SC_ORDER_PRESORTED) {
		lb->SetList(list, separator, typesep);
		sortMatrix.clear();
		for (int i = 0; i < lb->Length(); ++i)
			sortMatrix.push_back(i);
		return;
	}

	Sorter IndexSort(this, list);
	sortMatrix.clear();
	for (int i = 0; i < (int)IndexSort.indices.size() / 2; ++i)
		sortMatrix.push_back(i);
	std::sort(sortMatrix.begin(), sortMatrix.end(), IndexSort);
	if (autoSort == SC_ORDER_CUSTOM || sortMatrix.size() < 2) {
		lb->SetList(list, separator, typesep);
		PLATFORM_ASSERT(lb->Length() == static_cast<int>(sortMatrix.size()));
		return;
	}

	std::string sortedList;
	char item[maxItemLen];
	for (size_t i = 0; i < sortMatrix.size(); ++i) {
		int wordLen = IndexSort.indices[sortMatrix[i] * 2 + 2] - IndexSort.indices[sortMatrix[i] * 2];
		if (wordLen > maxItemLen-2)
			wordLen = maxItemLen - 2;
		memcpy(item, list + IndexSort.indices[sortMatrix[i] * 2], wordLen);
		if ((i+1) == sortMatrix.size()) {
			// Last item so remove separator if present
			if ((wordLen > 0) && (item[wordLen-1] == separator))
				wordLen--;
		} else {
			// Item before last needs a separator
			if ((wordLen == 0) || (item[wordLen-1] != separator)) {
				item[wordLen] = separator;
				wordLen++;
			}
		}
		item[wordLen] = '\0';
		sortedList += item;
	}
	for (int i = 0; i < (int)sortMatrix.size(); ++i)
		sortMatrix[i] = i;
	lb->SetList(sortedList.c_str(), separator, typesep);
}

int AutoComplete::GetSelection() const {
	return lb->GetSelection();
}

std::string AutoComplete::GetValue(int item) const {
	char value[maxItemLen];
	lb->GetValue(item, value, sizeof(value));
	return std::string(value);
}

void AutoComplete::Show(bool show) {
	lb->Show(show);
	if (show)
		lb->Select(0);
}

void AutoComplete::Cancel() {
	if (lb->Created()) {
		lb->Clear();
		lb->Destroy();
		active = false;
	}
}


void AutoComplete::Move(int delta) {
	int count = lb->Length();
	int current = lb->GetSelection();
	current += delta;
	if (current >= count)
		current = count - 1;
	if (current < 0)
		current = 0;
	lb->Select(current);
}

void AutoComplete::Select(const char *word) {
	size_t lenWord = strlen(word);
	int location = -1;
	int start = 0; // lower bound of the api array block to search
	int end = lb->Length() - 1; // upper bound of the api array block to search
	while ((start <= end) && (location == -1)) { // Binary searching loop
		int pivot = (start + end) / 2;
		char item[maxItemLen];
		lb->GetValue(sortMatrix[pivot], item, maxItemLen);
		int cond;
		if (ignoreCase)
			cond = CompareNCaseInsensitive(word, item, lenWord);
		else
			cond = strncmp(word, item, lenWord);
		if (!cond) {
			// Find first match
			while (pivot > start) {
				lb->GetValue(sortMatrix[pivot-1], item, maxItemLen);
				if (ignoreCase)
					cond = CompareNCaseInsensitive(word, item, lenWord);
				else
					cond = strncmp(word, item, lenWord);
				if (0 != cond)
					break;
				--pivot;
			}
			location = pivot;
			if (ignoreCase
				&& ignoreCaseBehaviour == SC_CASEINSENSITIVEBEHAVIOUR_RESPECTCASE) {
				// Check for exact-case match
				for (; pivot <= end; pivot++) {
					lb->GetValue(sortMatrix[pivot], item, maxItemLen);
					if (!strncmp(word, item, lenWord)) {
						location = pivot;
						break;
					}
					if (CompareNCaseInsensitive(word, item, lenWord))
						break;
				}
			}
		} else if (cond < 0) {
			end = pivot - 1;
		} else if (cond > 0) {
			start = pivot + 1;
		}
	}
	if (location == -1) {
		if (autoHide)
			Cancel();
		else
			lb->Select(-1);
	} else {
		if (autoSort == SC_ORDER_CUSTOM) {
			// Check for a logically earlier match
			char item[maxItemLen];
			for (int i = location + 1; i <= end; ++i) {
				lb->GetValue(sortMatrix[i], item, maxItemLen);
				if (CompareNCaseInsensitive(word, item, lenWord))
					break;
				if (sortMatrix[i] < sortMatrix[location] && !strncmp(word, item, lenWord))
					location = i;
			}
		}
		lb->Select(sortMatrix[location]);
	}
}

