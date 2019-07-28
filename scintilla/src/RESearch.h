// Scintilla source code edit control
/** @file RESearch.h
 ** Interface to the regular expression search library.
 **/
// Written by Neil Hodgson <neilh@scintilla.org>
// Based on the work of Ozan S. Yigit.
// This file is in the public domain.

#ifndef RESEARCH_H
#define RESEARCH_H

namespace Scintilla {

class CharacterIndexer {
public:
	virtual char CharAt(Sci::Position index) const=0;
	virtual ~CharacterIndexer() {
	}
};

class RESearch {

public:
	explicit RESearch(CharClassify *charClassTable);
	// No dynamic allocation so default copy constructor and assignment operator are OK.
	~RESearch();
	void Clear();
	void GrabMatches(const CharacterIndexer &ci);
	const char *Compile(const char *pattern, Sci::Position length, bool caseSensitive, bool posix);
	int Execute(const CharacterIndexer &ci, Sci::Position lp, Sci::Position endp);

	enum { MAXTAG=10 };
	enum { NOTFOUND=-1 };

	Sci::Position bopat[MAXTAG];
	Sci::Position eopat[MAXTAG];
	std::string pat[MAXTAG];

private:

	enum { MAXNFA = 4096 };
	// The following enums are not meant to be changeable.
	// They are for readability only.
	enum { MAXCHR = 256 };
	enum { CHRBIT = 8 };
	enum { BITBLK = MAXCHR / CHRBIT };

	void ChSet(unsigned char c);
	void ChSetWithCase(unsigned char c, bool caseSensitive);
	int GetBackslashExpression(const char *pattern, int &incr);

	Sci::Position PMatch(const CharacterIndexer &ci, Sci::Position lp, Sci::Position endp, char *ap);

	Sci::Position bol;
	Sci::Position tagstk[MAXTAG];  /* subpat tag stack */
	char nfa[MAXNFA];    /* automaton */
	int sta;
	unsigned char bittab[BITBLK]; /* bit table for CCL pre-set bits */
	int failure;
	CharClassify *charClass;
	bool iswordc(unsigned char x) const {
		return charClass->IsWord(x);
	}
};

}

#endif

