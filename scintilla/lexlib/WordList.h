// Scintilla source code edit control
/** @file WordList.h
 ** Hold a list of words.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WORDLIST_H
#define WORDLIST_H

namespace Scintilla {

/**
 */
class WordList {
	// Each word contains at least one character - a empty word acts as sentinel at the end.
	char **words;
	char *list;
	int len;
	bool onlyLineEnds;	///< Delimited by any white space or only line ends
	int starts[256];
public:
	explicit WordList(bool onlyLineEnds_ = false);
	~WordList();
	operator bool() const noexcept;
	bool operator!=(const WordList &other) const noexcept;
	int Length() const noexcept;
	void Clear() noexcept;
	bool Set(const char *s);
	bool InList(const char *s) const noexcept;
	bool InListAbbreviated(const char *s, const char marker) const noexcept;
	bool InListAbridged(const char *s, const char marker) const noexcept;
	const char *WordAt(int n) const noexcept;

	void SetWordAt(int n, const char *word2Set) {
		words[n] = (char *)word2Set;
	};

	int StartAt(int n) const {
		return starts[n];
	};
};

}

#endif
