// Scintilla source code edit control
/** @file WordList.h
 ** Hold a list of words.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WORDLIST_H
#define WORDLIST_H

namespace Lexilla {

/**
 */
class WordList {
	// Each word contains at least one character - an empty word acts as sentinel at the end.
	char **words;
	char *list;
	size_t len;
	bool onlyLineEnds;	///< Delimited by any white space or only line ends
	int starts[256];
public:
	explicit WordList(bool onlyLineEnds_ = false) noexcept;
	// Deleted so WordList objects can not be copied.
	WordList(const WordList &) = delete;
	WordList(WordList &&) = delete;
	WordList &operator=(const WordList &) = delete;
	WordList &operator=(WordList &&) = delete;
	~WordList();
	operator bool() const noexcept;
	bool operator!=(const WordList &other) const noexcept;
	int Length() const noexcept;
	void Clear() noexcept;
	bool Set(const char *s, bool lowerCase=false);
	bool InList(const char *s) const noexcept;
	bool InList(std::string_view sv) const noexcept;
	bool InListAbbreviated(const char *s, const char marker) const noexcept;
	bool InListAbridged(const char *s, const char marker) const noexcept;
	const char *WordAt(int n) const noexcept;

	void SetWordAt(int n, const char* word2Set) {
		words[n] = (char*)word2Set;
};

	int StartAt(int n) const {
		return starts[n];
	};
};

}

#endif
