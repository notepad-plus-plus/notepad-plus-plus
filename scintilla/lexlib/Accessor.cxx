// Scintilla source code edit control
/** @file Accessor.cxx
 ** Interfaces between Scintilla and lexers.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"

using namespace Scintilla;

Accessor::Accessor(IDocument *pAccess_, PropSetSimple *pprops_) : LexAccessor(pAccess_), pprops(pprops_) {
}

int Accessor::GetPropertyInt(const char *key, int defaultValue) const {
	return pprops->GetInt(key, defaultValue);
}

int Accessor::IndentAmount(Sci_Position line, int *flags, PFNIsCommentLeader pfnIsCommentLeader) {
	const Sci_Position end = Length();
	int spaceFlags = 0;

	// Determines the indentation level of the current line and also checks for consistent
	// indentation compared to the previous line.
	// Indentation is judged consistent when the indentation whitespace of each line lines
	// the same or the indentation of one line is a prefix of the other.

	Sci_Position pos = LineStart(line);
	char ch = (*this)[pos];
	int indent = 0;
	bool inPrevPrefix = line > 0;
	Sci_Position posPrev = inPrevPrefix ? LineStart(line-1) : 0;
	while ((ch == ' ' || ch == '\t') && (pos < end)) {
		if (inPrevPrefix) {
			const char chPrev = (*this)[posPrev++];
			if (chPrev == ' ' || chPrev == '\t') {
				if (chPrev != ch)
					spaceFlags |= wsInconsistent;
			} else {
				inPrevPrefix = false;
			}
		}
		if (ch == ' ') {
			spaceFlags |= wsSpace;
			indent++;
		} else {	// Tab
			spaceFlags |= wsTab;
			if (spaceFlags & wsSpace)
				spaceFlags |= wsSpaceTab;
			indent = (indent / 8 + 1) * 8;
		}
		ch = (*this)[++pos];
	}

	*flags = spaceFlags;
	indent += SC_FOLDLEVELBASE;
	// if completely empty line or the start of a comment...
	if ((LineStart(line) == Length()) || (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') ||
			(pfnIsCommentLeader && (*pfnIsCommentLeader)(*this, pos, end-pos)))
		return indent | SC_FOLDLEVELWHITEFLAG;
	else
		return indent;
}
