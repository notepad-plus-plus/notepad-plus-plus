// Scintilla source code edit control
/** @file AutoComplete.cxx
 ** Defines the auto completion list box.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "CharacterType.h"
#include "Position.h"
#include "AutoComplete.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

AutoComplete::AutoComplete() :
	active(false),
	separator(' '),
	typesep('?'),
	ignoreCase(false),
	chooseSingle(false),
	options(AutoCompleteOption::Normal),
	imageScale(1.0),
	posStart(0),
	startLen(0),
	cancelAtStartPos(true),
	autoHide(true),
	dropRestOfWord(false),
	ignoreCaseBehaviour(CaseInsensitiveBehaviour::RespectCase),
	widthLBDefault(100),
	heightLBDefault(100),
	autoSort(Ordering::PreSorted) {
	lb = ListBox::Allocate();
}

AutoComplete::~AutoComplete() {
	if (lb) {
		lb->Destroy();
	}
}

bool AutoComplete::Active() const noexcept {
	return active;
}

void AutoComplete::Start(Window &parent, int ctrlID,
	Sci::Position position, Point location, Sci::Position startLen_,
	int lineHeight, bool unicodeMode, Technology technology, ListOptions listOptions) {
	if (active) {
		Cancel();
	}
	lb->SetOptions(listOptions);
	lb->Create(parent, ctrlID, location, lineHeight, unicodeMode, technology);
	lb->Clear();
	active = true;
	startLen = startLen_;
	posStart = position;
}

void AutoComplete::SetStopChars(const char *stopChars_) {
	stopChars = stopChars_;
}

bool AutoComplete::IsStopChar(char ch) const noexcept {
	return ch && (stopChars.find(ch) != std::string::npos);
}

void AutoComplete::SetFillUpChars(const char *fillUpChars_) {
	fillUpChars = fillUpChars_;
}

bool AutoComplete::IsFillUpChar(char ch) const noexcept {
	return ch && (fillUpChars.find(ch) != std::string::npos);
}

void AutoComplete::SetSeparator(char separator_) noexcept {
	separator = separator_;
}

char AutoComplete::GetSeparator() const noexcept {
	return separator;
}

void AutoComplete::SetTypesep(char separator_) noexcept {
	typesep = separator_;
}

char AutoComplete::GetTypesep() const noexcept {
	return typesep;
}

namespace {

struct Sorter {
	const bool ignoreCase;
	const char *list;
	std::vector<int> indices;

	Sorter(const AutoComplete *ac, const char *list_) : ignoreCase(ac->ignoreCase), list(list_) {
		int i = 0;
		if (!list[i]) {
			// Empty list has a single empty member
			indices.push_back(i); // word start
			indices.push_back(i); // word end
		}
		const char separator = ac->GetSeparator();
		const char typesep = ac->GetTypesep();
		while (list[i]) {
			indices.push_back(i); // word start
			while (list[i] != typesep && list[i] != separator && list[i])
				++i;
			indices.push_back(i); // word end
			if (list[i] == typesep) {
				while (list[i] != separator && list[i])
					++i;
			}
			if (list[i] == separator) {
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

	bool operator()(int a, int b) const noexcept {
		const unsigned indexA = a * 2;
		const unsigned indexB = b * 2;
		const int lenA = indices[indexA + 1] - indices[indexA];
		const int lenB = indices[indexB + 1] - indices[indexB];
		const int len  = std::min(lenA, lenB);
		int cmp;
		if (ignoreCase)
			cmp = CompareNCaseInsensitive(list + indices[indexA], list + indices[indexB], len);
		else
			cmp = strncmp(list + indices[indexA], list + indices[indexB], len);
		if (cmp == 0)
			cmp = lenA - lenB;
		return cmp < 0;
	}
};

void FillSortMatrix(std::vector<int> &sortMatrix, int itemCount) {
	sortMatrix.clear();
	for (int i = 0; i < itemCount; i++) {
		sortMatrix.push_back(i);
	}
}

}

void AutoComplete::SetList(const char *list) {
	if (autoSort == Ordering::PreSorted) {
		lb->SetList(list, separator, typesep);
		FillSortMatrix(sortMatrix, lb->Length());
		return;
	}

	const Sorter IndexSort(this, list);
	FillSortMatrix(sortMatrix, static_cast<int>(IndexSort.indices.size() / 2));
	std::sort(sortMatrix.begin(), sortMatrix.end(), IndexSort);
	if (autoSort == Ordering::Custom || sortMatrix.size() < 2) {
		lb->SetList(list, separator, typesep);
		PLATFORM_ASSERT(lb->Length() == static_cast<int>(sortMatrix.size()));
		return;
	}

	std::string sortedList;
	for (size_t i = 0; i < sortMatrix.size(); ++i) {
		const unsigned index = sortMatrix[i] * 2;
		sortMatrix[i] = static_cast<int>(i);
		// word length include trailing typesep and separator
		const int wordLen = IndexSort.indices[index + 2] - IndexSort.indices[index];
		const std::string_view item(list + IndexSort.indices[index], wordLen);
		sortedList += item;
		if ((i + 1) == sortMatrix.size()) {
			// Last item so remove separator if present
			if (!item.empty() && item.back() == separator) {
				sortedList.pop_back();
			}
		} else {
			// Item before last needs a separator
			if (item.empty() || item.back() != separator) {
				sortedList += separator;
			}
		}
	}
	lb->SetList(sortedList.c_str(), separator, typesep);
}

int AutoComplete::GetSelection() const {
	return lb->GetSelection();
}

std::string AutoComplete::GetValue(int item) const {
	return lb->GetValue(item);
}

void AutoComplete::Show(bool show) {
	lb->Show(show);
	if (show)
		lb->Select(0);
}

void AutoComplete::Cancel() noexcept {
	if (lb->Created()) {
		lb->Clear();
		lb->Destroy();
		active = false;
	}
}


void AutoComplete::Move(int delta) {
	const int count = lb->Length();
	int current = lb->GetSelection();
	current += delta;
	if (current >= count)
		current = count - 1;
	if (current < 0)
		current = 0;
	lb->Select(current);
}

void AutoComplete::Select(const char *word) {
	const size_t lenWord = strlen(word);
	int location = -1;
	int start = 0; // lower bound of the api array block to search
	int end = lb->Length() - 1; // upper bound of the api array block to search
	while ((start <= end) && (location == -1)) { // Binary searching loop
		int pivot = (start + end) / 2;
		std::string item = GetValue(sortMatrix[pivot]);
		int cond;
		if (ignoreCase)
			cond = CompareNCaseInsensitive(word, item.c_str(), lenWord);
		else
			cond = strncmp(word, item.c_str(), lenWord);
		if (!cond) {
			// Find first match
			while (pivot > start) {
				item = lb->GetValue(sortMatrix[pivot-1]);
				if (ignoreCase)
					cond = CompareNCaseInsensitive(word, item.c_str(), lenWord);
				else
					cond = strncmp(word, item.c_str(), lenWord);
				if (0 != cond)
					break;
				--pivot;
			}
			location = pivot;
			if (ignoreCase
				&& ignoreCaseBehaviour == CaseInsensitiveBehaviour::RespectCase) {
				// Check for exact-case match
				for (; pivot <= end; pivot++) {
					item = lb->GetValue(sortMatrix[pivot]);
					if (!strncmp(word, item.c_str(), lenWord)) {
						location = pivot;
						break;
					}
					if (CompareNCaseInsensitive(word, item.c_str(), lenWord))
						break;
				}
			}
		} else if (cond < 0) {
			end = pivot - 1;
		} else { // cond > 0
			start = pivot + 1;
		}
	}
	if (location == -1) {
		if (autoHide)
			Cancel();
		else
			lb->Select(-1);
	} else {
		if (autoSort == Ordering::Custom) {
			// Check for a logically earlier match
			for (int i = location + 1; i <= end; ++i) {
				const std::string item = lb->GetValue(sortMatrix[i]);
				if (CompareNCaseInsensitive(word, item.c_str(), lenWord))
					break;
				if (sortMatrix[i] < sortMatrix[location] && !strncmp(word, item.c_str(), lenWord))
					location = i;
			}
		}
		lb->Select(sortMatrix[location]);
	}
}

