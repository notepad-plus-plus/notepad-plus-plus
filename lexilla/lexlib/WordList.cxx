// Scintilla source code edit control
/** @file WordList.cxx
 ** Hold a list of words.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <string>
#include <algorithm>
#include <iterator>
#include <memory>

#include "WordList.h"
#include "CharacterSet.h"

using namespace Lexilla;

namespace {

/**
 * Creates an array that points into each word in the string and puts \0 terminators
 * after each word.
 */
std::unique_ptr<char *[]> ArrayFromWordList(char *wordlist, size_t slen, size_t *len, bool onlyLineEnds = false) {
	assert(wordlist);
	size_t words = 0;
	// For rapid determination of whether a character is a separator, build
	// a look up table.
	bool wordSeparator[256] = {};	// Initialise all to false.
	wordSeparator[static_cast<unsigned int>('\r')] = true;
	wordSeparator[static_cast<unsigned int>('\n')] = true;
	if (!onlyLineEnds) {
		wordSeparator[static_cast<unsigned int>(' ')] = true;
		wordSeparator[static_cast<unsigned int>('\t')] = true;
	}
	unsigned char prev = '\n';
	for (int j = 0; wordlist[j]; j++) {
		const unsigned char curr = wordlist[j];
		if (!wordSeparator[curr] && wordSeparator[prev])
			words++;
		prev = curr;
	}
	std::unique_ptr<char *[]> keywords = std::make_unique<char *[]>(words + 1);
	size_t wordsStore = 0;
	if (words) {
		unsigned char previous = '\0';
		for (size_t k = 0; k < slen; k++) {
			if (!wordSeparator[static_cast<unsigned char>(wordlist[k])]) {
				if (!previous) {
					keywords[wordsStore] = &wordlist[k];
					wordsStore++;
				}
			} else {
				wordlist[k] = '\0';
			}
			previous = wordlist[k];
		}
	}
	assert(wordsStore < (words + 1));
	keywords[wordsStore] = &wordlist[slen];
	*len = wordsStore;
	return keywords;
}

bool cmpWords(const char *a, const char *b) noexcept {
	return strcmp(a, b) < 0;
}

}

WordList::WordList(bool onlyLineEnds_) noexcept :
	words(nullptr), list(nullptr), len(0), onlyLineEnds(onlyLineEnds_) {
	// Prevent warnings by static analyzers about uninitialized starts.
	starts[0] = -1;
}

WordList::~WordList() {
	Clear();
}

WordList::operator bool() const noexcept {
	return len != 0;
}

bool WordList::operator!=(const WordList &other) const noexcept {
	if (len != other.len)
		return true;
	for (size_t i=0; i<len; i++) {
		if (strcmp(words[i], other.words[i]) != 0)
			return true;
	}
	return false;
}

int WordList::Length() const noexcept {
	return static_cast<int>(len);
}

void WordList::Clear() noexcept {
	delete []list;
	list = nullptr;
	delete []words;
	words = nullptr;
	len = 0;
}

bool WordList::Set(const char *s, bool lowerCase) {
	const size_t lenS = strlen(s) + 1;
	std::unique_ptr<char[]> listTemp = std::make_unique<char[]>(lenS);
	memcpy(listTemp.get(), s, lenS);
	if (lowerCase) {
		for (size_t i = 0; i < lenS; i++) {
			listTemp[i] = MakeLowerCase(listTemp[i]);
		}
	}
	size_t lenTemp = 0;
	std::unique_ptr<char *[]> wordsTemp = ArrayFromWordList(listTemp.get(), lenS - 1, &lenTemp, onlyLineEnds);
	std::sort(wordsTemp.get(), wordsTemp.get() + lenTemp, cmpWords);

	if (lenTemp == len) {
		bool changed = false;
		for (size_t i = 0; i < lenTemp; i++) {
			if (strcmp(words[i], wordsTemp[i]) != 0) {
				changed = true;
				break;
			}
		}
		if (!changed) {
			return false;
		}
	}

	Clear();
	words = wordsTemp.release();
	list = listTemp.release();
	len = lenTemp;
	std::fill(starts, std::end(starts), -1);
	for (int l = static_cast<int>(len - 1); l >= 0; l--) {
		unsigned char const indexChar = words[l][0];
		starts[indexChar] = l;
	}
	return true;
}

/** Check whether a string is in the list.
 * List elements are either exact matches or prefixes.
 * Prefix elements start with '^' and match all strings that start with the rest of the element
 * so '^GTK_' matches 'GTK_X', 'GTK_MAJOR_VERSION', and 'GTK_'.
 */
bool WordList::InList(const char *s) const noexcept {
	if (!words)
		return false;
	const char first = s[0];
	const unsigned char firstChar = first;
	int j = starts[firstChar];
	if (j >= 0) {
		while (words[j][0] == first) {
			if (s[1] == words[j][1]) {
				const char *a = words[j] + 1;
				const char *b = s + 1;
				while (*a && *a == *b) {
					a++;
					b++;
				}
				if (!*a && !*b)
					return true;
			}
			j++;
		}
	}
	j = starts[static_cast<unsigned int>('^')];
	if (j >= 0) {
		while (words[j][0] == '^') {
			const char *a = words[j] + 1;
			const char *b = s;
			while (*a && *a == *b) {
				a++;
				b++;
			}
			if (!*a)
				return true;
			j++;
		}
	}
	return false;
}

/** convenience overload so can easily call with std::string.
 */

bool WordList::InList(std::string_view sv) const noexcept {
	if (!words || sv.empty())
		return false;
	const char first = sv[0];
	const unsigned char firstChar = first;
	if (int j = starts[firstChar]; j >= 0) {
		const std::string_view after = sv.substr(1);
		for (; words[j][0] == first; j++) {
			if (std::string_view(words[j] + 1) == after) {
				return true;
			}
		}
	}
	if (int j = starts[static_cast<unsigned int>('^')]; j >= 0) {
		for (; words[j][0] == '^';j++) {
			// Use rfind with 0 position to act like C++20 starts_with for C++17
			if (sv.rfind(words[j] + 1, 0) == 0) {
				return true;
			}
		}
	}
	return false;
}

/** similar to InList, but word s can be a substring of keyword.
 * eg. the keyword define is defined as def~ine. This means the word must start
 * with def to be a keyword, but also defi, defin and define are valid.
 * The marker is ~ in this case.
 */
bool WordList::InListAbbreviated(const char *s, const char marker) const noexcept {
	if (!words)
		return false;
	const char first = s[0];
	const unsigned char firstChar = first;
	int j = starts[firstChar];
	if (j >= 0) {
		while (words[j][0] == first) {
			bool isSubword = false;
			int start = 1;
			if (words[j][1] == marker) {
				isSubword = true;
				start++;
			}
			if (s[1] == words[j][start]) {
				const char *a = words[j] + start;
				const char *b = s + 1;
				while (*a && *a == *b) {
					a++;
					if (*a == marker) {
						isSubword = true;
						a++;
					}
					b++;
				}
				if ((!*a || isSubword) && !*b)
					return true;
			}
			j++;
		}
	}
	j = starts[static_cast<unsigned int>('^')];
	if (j >= 0) {
		while (words[j][0] == '^') {
			const char *a = words[j] + 1;
			const char *b = s;
			while (*a && *a == *b) {
				a++;
				b++;
			}
			if (!*a)
				return true;
			j++;
		}
	}
	return false;
}

/** similar to InListAbbreviated, but word s can be an abridged version of a keyword.
* eg. the keyword is defined as "after.~:". This means the word must have a prefix (begins with) of
* "after." and suffix (ends with) of ":" to be a keyword, Hence "after.field:" , "after.form.item:" are valid.
* Similarly "~.is.valid" keyword is suffix only... hence "field.is.valid" , "form.is.valid" are valid.
* The marker is ~ in this case.
* No multiple markers check is done and wont work.
*/
bool WordList::InListAbridged(const char *s, const char marker) const noexcept {
	if (!words)
		return false;
	const char first = s[0];
	const unsigned char firstChar = first;
	int j = starts[firstChar];
	if (j >= 0) {
		while (words[j][0] == first) {
			const char *a = words[j];
			const char *b = s;
			while (*a && *a == *b) {
				a++;
				if (*a == marker) {
					a++;
					const size_t suffixLengthA = strlen(a);
					const size_t suffixLengthB = strlen(b);
					if (suffixLengthA >= suffixLengthB)
						break;
					b = b + suffixLengthB - suffixLengthA - 1;
				}
				b++;
			}
			if (!*a  && !*b)
				return true;
			j++;
		}
	}

	j = starts[static_cast<unsigned int>(marker)];
	if (j >= 0) {
		while (words[j][0] == marker) {
			const char *a = words[j] + 1;
			const char *b = s;
			const size_t suffixLengthA = strlen(a);
			const size_t suffixLengthB = strlen(b);
			if (suffixLengthA > suffixLengthB) {
				j++;
				continue;
			}
			b = b + suffixLengthB - suffixLengthA;

			while (*a && *a == *b) {
				a++;
				b++;
			}
			if (!*a && !*b)
				return true;
			j++;
		}
	}

	return false;
}

const char *WordList::WordAt(int n) const noexcept {
	return words[n];
}

