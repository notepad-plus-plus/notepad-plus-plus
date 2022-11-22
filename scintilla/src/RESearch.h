// Scintilla source code edit control
/** @file RESearch.h
 ** Interface to the regular expression search library.
 **/
// Written by Neil Hodgson <neilh@scintilla.org>
// Based on the work of Ozan S. Yigit.
// This file is in the public domain.

#ifndef RESEARCH_H
#define RESEARCH_H

namespace Scintilla::Internal {

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
	void Clear() noexcept;
	void GrabMatches(const CharacterIndexer &ci);
	const char *Compile(const char *pattern, Sci::Position length, bool caseSensitive, bool posix) noexcept;
	int Execute(const CharacterIndexer &ci, Sci::Position lp, Sci::Position endp);

	static constexpr int MAXTAG = 10;
	static constexpr int NOTFOUND = -1;

	Sci::Position bopat[MAXTAG];
	Sci::Position eopat[MAXTAG];
	std::string pat[MAXTAG];

private:

	static constexpr int MAXNFA = 4096;
	// The following constants are not meant to be changeable.
	// They are for readability only.
	static constexpr int MAXCHR = 256;
	static constexpr int CHRBIT = 8;
	static constexpr int BITBLK = MAXCHR / CHRBIT;

	void ChSet(unsigned char c) noexcept;
	void ChSetWithCase(unsigned char c, bool caseSensitive) noexcept;
	int GetBackslashExpression(const char *pattern, int &incr) noexcept;

	Sci::Position PMatch(const CharacterIndexer &ci, Sci::Position lp, Sci::Position endp, char *ap);

	Sci::Position bol;
	Sci::Position tagstk[MAXTAG];  /* subpat tag stack */
	char nfa[MAXNFA];    /* automaton */
	int sta;
	unsigned char bittab[BITBLK]; /* bit table for CCL pre-set bits */
	int failure;
	CharClassify *charClass;
	bool iswordc(unsigned char x) const noexcept {
		return charClass->IsWord(x);
	}
};

}

#endif

