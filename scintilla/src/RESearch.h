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
	virtual Sci::Position MovePositionOutsideChar(Sci::Position pos, [[maybe_unused]] Sci::Position moveDir) const noexcept=0;
};

class RESearch {

public:
	explicit RESearch(CharClassify *charClassTable);
	// No dynamic allocation so default copy constructor and assignment operator are OK.
	void Clear();
	const char *Compile(const char *pattern, Sci::Position length, bool caseSensitive, bool posix);
	int Execute(const CharacterIndexer &ci, Sci::Position lp, Sci::Position endp);
	void SetLineRange(Sci::Position startPos, Sci::Position endPos) noexcept {
		lineStartPos = startPos;
		lineEndPos = endPos;
	}

	static constexpr int MAXTAG = 10;
	static constexpr int NOTFOUND = -1;

	using MatchPositions = std::array<Sci::Position, MAXTAG>;
	MatchPositions bopat;
	MatchPositions eopat;

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

	Sci::Position PMatch(const CharacterIndexer &ci, Sci::Position lp, Sci::Position endp, const char *ap);

	// positions to match line start and line end
	Sci::Position lineStartPos;
	Sci::Position lineEndPos;
	char nfa[MAXNFA];    /* automaton */
	int sta;
	int failure;
	std::array<unsigned char, BITBLK> bittab {}; /* bit table for CCL pre-set bits */
	CharClassify *charClass;
	bool iswordc(unsigned char x) const noexcept {
		return charClass->IsWord(x);
	}
};

}

#endif

