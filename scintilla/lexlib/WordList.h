// Scintilla source code edit control
/** @file WordList.h
 ** Hold a list of words.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WORDLIST_H
#define WORDLIST_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

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
	WordList(bool onlyLineEnds_ = false);
	~WordList();
	operator bool() const;
	bool operator!=(const WordList &other) const;
	int Length() const;
	void Clear();
	void Set(const char *s);
	bool InList(const char *s) const;
	bool InListAbbreviated(const char *s, const char marker) const;
	const char *WordAt(int n) const;
	void WordList::SetWordAt(int n, const char *word2Set) {
		words[n] = (char *)word2Set;
	};
	int StartAt(int n) const {
		return starts[n];
	};
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
