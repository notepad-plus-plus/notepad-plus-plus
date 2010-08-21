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
public:
	// Each word contains at least one character - a empty word acts as sentinel at the end.
	char **words;
	char *list;
	int len;
	bool onlyLineEnds;	///< Delimited by any white space or only line ends
	int starts[256];
	WordList(bool onlyLineEnds_ = false) :
		words(0), list(0), len(0), onlyLineEnds(onlyLineEnds_)
		{}
	~WordList() { Clear(); }
	operator bool() const { return len ? true : false; }
	bool operator!=(const WordList &other) const;
	void Clear();
	void Set(const char *s);
	bool InList(const char *s) const;
	bool InListAbbreviated(const char *s, const char marker) const;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
