// Scintilla source code edit control
/** @file RESearch.h
 ** Interface to the regular expression search library.
 **/
// Written by Neil Hodgson <neilh@scintilla.org>
// Based on the work of Ozan S. Yigit.
// This file is in the public domain.

#ifndef RESEARCH_H
#define RESEARCH_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/*
 * The following defines are not meant to be changeable.
 * They are for readability only.
 */
#define MAXCHR	256
#define CHRBIT	8
#define BITBLK	MAXCHR/CHRBIT

class CharacterIndexer {
public:
	virtual char CharAt(int index)=0;
	virtual ~CharacterIndexer() {
	}
};

class RESearch {

public:
	RESearch(CharClassify *charClassTable);
	~RESearch();
	bool GrabMatches(CharacterIndexer &ci);
	const char *Compile(const char *pattern, int length, bool caseSensitive, bool posix);
	int Execute(CharacterIndexer &ci, int lp, int endp);
	int Substitute(CharacterIndexer &ci, char *src, char *dst);

	enum { MAXTAG=10 };
	enum { MAXNFA=2048 };
	enum { NOTFOUND=-1 };

	int bopat[MAXTAG];
	int eopat[MAXTAG];
	char *pat[MAXTAG];

private:
	void Init();
	void Clear();
	void ChSet(unsigned char c);
	void ChSetWithCase(unsigned char c, bool caseSensitive);
	int GetBackslashExpression(const char *pattern, int &incr);

	int PMatch(CharacterIndexer &ci, int lp, int endp, char *ap);

	int bol;
	int tagstk[MAXTAG];  /* subpat tag stack */
	char nfa[MAXNFA];    /* automaton */
	int sta;
	unsigned char bittab[BITBLK]; /* bit table for CCL pre-set bits */
	int failure;
	CharClassify *charClass;
	bool iswordc(unsigned char x) {
		return charClass->IsWord(x);
	}
};

#ifdef SCI_NAMESPACE
}
#endif

#endif

