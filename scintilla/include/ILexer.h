// Scintilla source code edit control
/** @file ILexer.h
 ** Interface between Scintilla and lexers.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ILEXER_H
#define ILEXER_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

#ifdef _WIN32
	#define SCI_METHOD __stdcall
#else
	#define SCI_METHOD
#endif

enum { dvOriginal=0 };

class IDocument {
public:
	virtual int SCI_METHOD Version() const = 0;
	virtual void SCI_METHOD SetErrorStatus(int status) = 0;
	virtual int SCI_METHOD Length() const = 0;
	virtual void SCI_METHOD GetCharRange(char *buffer, int position, int lengthRetrieve) const = 0;
	virtual char SCI_METHOD StyleAt(int position) const = 0;
	virtual int SCI_METHOD LineFromPosition(int position) const = 0;
	virtual int SCI_METHOD LineStart(int line) const = 0;
	virtual int SCI_METHOD GetLevel(int line) const = 0;
	virtual int SCI_METHOD SetLevel(int line, int level) = 0;
	virtual int SCI_METHOD GetLineState(int line) const = 0;
	virtual int SCI_METHOD SetLineState(int line, int state) = 0;
	virtual void SCI_METHOD StartStyling(int position, char mask) = 0;
	virtual bool SCI_METHOD SetStyleFor(int length, char style) = 0;
	virtual bool SCI_METHOD SetStyles(int length, const char *styles) = 0;
	virtual void SCI_METHOD DecorationSetCurrentIndicator(int indicator) = 0;
	virtual void SCI_METHOD DecorationFillRange(int position, int value, int fillLength) = 0;
	virtual void SCI_METHOD ChangeLexerState(int start, int end) = 0;
	virtual int SCI_METHOD CodePage() const = 0;
	virtual bool SCI_METHOD IsDBCSLeadByte(char ch) const = 0;
	virtual const char * SCI_METHOD BufferPointer() = 0;
	virtual int SCI_METHOD GetLineIndentation(int line) = 0;
};

enum { lvOriginal=0 };

class ILexer {
public:
	virtual int SCI_METHOD Version() const = 0;
	virtual void SCI_METHOD Release() = 0;
	virtual const char * SCI_METHOD PropertyNames() = 0;
	virtual int SCI_METHOD PropertyType(const char *name) = 0;
	virtual const char * SCI_METHOD DescribeProperty(const char *name) = 0;
	virtual int SCI_METHOD PropertySet(const char *key, const char *val) = 0;
	virtual const char * SCI_METHOD DescribeWordListSets() = 0;
	virtual int SCI_METHOD WordListSet(int n, const char *wl) = 0;
	virtual void SCI_METHOD Lex(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess) = 0;
	virtual void SCI_METHOD Fold(unsigned int startPos, int lengthDoc, int initStyle, IDocument *pAccess) = 0;
	virtual void * SCI_METHOD PrivateCall(int operation, void *pointer) = 0;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
