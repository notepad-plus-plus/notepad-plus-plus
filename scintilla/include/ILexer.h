// Scintilla source code edit control
/** @file ILexer.h
 ** Interface between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ILEXER_H
#define ILEXER_H

#include "Sci_Position.h"

namespace Scintilla {

enum { dvRelease4=2 };

class IDocument {
public:
	virtual int SCI_METHOD Version() const = 0;
	virtual void SCI_METHOD SetErrorStatus(int status) = 0;
	virtual Sci_Position SCI_METHOD Length() const = 0;
	virtual void SCI_METHOD GetCharRange(char *buffer, Sci_Position position, Sci_Position lengthRetrieve) const = 0;
	virtual char SCI_METHOD StyleAt(Sci_Position position) const = 0;
	virtual Sci_Position SCI_METHOD LineFromPosition(Sci_Position position) const = 0;
	virtual Sci_Position SCI_METHOD LineStart(Sci_Position line) const = 0;
	virtual int SCI_METHOD GetLevel(Sci_Position line) const = 0;
	virtual int SCI_METHOD SetLevel(Sci_Position line, int level) = 0;
	virtual int SCI_METHOD GetLineState(Sci_Position line) const = 0;
	virtual int SCI_METHOD SetLineState(Sci_Position line, int state) = 0;
	virtual void SCI_METHOD StartStyling(Sci_Position position) = 0;
	virtual bool SCI_METHOD SetStyleFor(Sci_Position length, char style) = 0;
	virtual bool SCI_METHOD SetStyles(Sci_Position length, const char *styles) = 0;
	virtual void SCI_METHOD DecorationSetCurrentIndicator(int indicator) = 0;
	virtual void SCI_METHOD DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength) = 0;
	virtual void SCI_METHOD ChangeLexerState(Sci_Position start, Sci_Position end) = 0;
	virtual int SCI_METHOD CodePage() const = 0;
	virtual bool SCI_METHOD IsDBCSLeadByte(char ch) const = 0;
	virtual const char * SCI_METHOD BufferPointer() = 0;
	virtual int SCI_METHOD GetLineIndentation(Sci_Position line) = 0;
	virtual Sci_Position SCI_METHOD LineEnd(Sci_Position line) const = 0;
	virtual Sci_Position SCI_METHOD GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const = 0;
	virtual int SCI_METHOD GetCharacterAndWidth(Sci_Position position, Sci_Position *pWidth) const = 0;
};

enum { lvRelease4=2, lvRelease5=3 };

class ILexer4 {
public:
	virtual int SCI_METHOD Version() const = 0;
	virtual void SCI_METHOD Release() = 0;
	virtual const char * SCI_METHOD PropertyNames() = 0;
	virtual int SCI_METHOD PropertyType(const char *name) = 0;
	virtual const char * SCI_METHOD DescribeProperty(const char *name) = 0;
	virtual Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) = 0;
	virtual const char * SCI_METHOD DescribeWordListSets() = 0;
	virtual Sci_Position SCI_METHOD WordListSet(int n, const char *wl) = 0;
	virtual void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) = 0;
	virtual void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument *pAccess) = 0;
	virtual void * SCI_METHOD PrivateCall(int operation, void *pointer) = 0;
	virtual int SCI_METHOD LineEndTypesSupported() = 0;
	virtual int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) = 0;
	virtual int SCI_METHOD SubStylesStart(int styleBase) = 0;
	virtual int SCI_METHOD SubStylesLength(int styleBase) = 0;
	virtual int SCI_METHOD StyleFromSubStyle(int subStyle) = 0;
	virtual int SCI_METHOD PrimaryStyleFromStyle(int style) = 0;
	virtual void SCI_METHOD FreeSubStyles() = 0;
	virtual void SCI_METHOD SetIdentifiers(int style, const char *identifiers) = 0;
	virtual int SCI_METHOD DistanceToSecondaryStyles() = 0;
	virtual const char * SCI_METHOD GetSubStyleBases() = 0;
	virtual int SCI_METHOD NamedStyles() = 0;
	virtual const char * SCI_METHOD NameOfStyle(int style) = 0;
	virtual const char * SCI_METHOD TagsOfStyle(int style) = 0;
	virtual const char * SCI_METHOD DescriptionOfStyle(int style) = 0;
};

class ILexer5 : public ILexer4 {
public:
	virtual const char * SCI_METHOD GetName() = 0;
	virtual int SCI_METHOD  GetIdentifier() = 0;
	virtual const char * SCI_METHOD PropertyGet(const char *key) = 0;
};

}

#endif
