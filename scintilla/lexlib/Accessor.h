// Scintilla source code edit control
/** @file Accessor.h
 ** Interfaces between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ACCESSOR_H
#define ACCESSOR_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

enum { wsSpace = 1, wsTab = 2, wsSpaceTab = 4, wsInconsistent=8};

class Accessor;
class WordList;
class PropSetSimple;

typedef bool (*PFNIsCommentLeader)(Accessor &styler, int pos, int len);

class Accessor : public LexAccessor {
public:
	PropSetSimple *pprops;
	Accessor(IDocument *pAccess_, PropSetSimple *pprops_);
	int GetPropertyInt(const char *, int defaultValue=0) const;
	int IndentAmount(int line, int *flags, PFNIsCommentLeader pfnIsCommentLeader = 0);
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
