// Scintilla source code edit control
/** @file Accessor.h
 ** Interfaces between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ACCESSOR_H
#define ACCESSOR_H

namespace Lexilla {

enum { wsSpace=1, wsTab=2, wsSpaceTab=4, wsInconsistent=8 };

class Accessor;
class WordList;
class PropSetSimple;

typedef bool (*PFNIsCommentLeader)(Accessor &styler, Sci_Position pos, Sci_Position len);

class Accessor : public LexAccessor {
public:
	PropSetSimple *pprops;
	Accessor(Scintilla::IDocument *pAccess_, PropSetSimple *pprops_);
	int GetPropertyInt(std::string_view key, int defaultValue=0) const;
	int IndentAmount(Sci_Position line, int *flags, PFNIsCommentLeader pfnIsCommentLeader = nullptr);
};

}

#endif
