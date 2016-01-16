// Scintilla source code edit control
/** @file LexAsn1.cxx
 ** Lexer for ASN.1
 **/
// Copyright 2004 by Herr Pfarrer rpfarrer <at> yahoo <dot> de
// Last Updated: 20/07/2004
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

// Some char test functions
static bool isAsn1Number(int ch)
{
	return (ch >= '0' && ch <= '9');
}

static bool isAsn1Letter(int ch)
{
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

static bool isAsn1Char(int ch)
{
	return (ch == '-' ) || isAsn1Number(ch) || isAsn1Letter (ch);
}

//
//	Function determining the color of a given code portion
//	Based on a "state"
//
static void ColouriseAsn1Doc(Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordLists[], Accessor &styler)
{
	// The keywords
	WordList &Keywords = *keywordLists[0];
	WordList &Attributes = *keywordLists[1];
	WordList &Descriptors = *keywordLists[2];
	WordList &Types = *keywordLists[3];

	// Parse the whole buffer character by character using StyleContext
	StyleContext sc(startPos, length, initStyle, styler);
	for (; sc.More(); sc.Forward())
	{
		// The state engine
		switch (sc.state)
		{
		case SCE_ASN1_DEFAULT:		// Plain characters
asn1_default:
			if (sc.ch == '-' && sc.chNext == '-')
				// A comment begins here
				sc.SetState(SCE_ASN1_COMMENT);
			else if (sc.ch == '"')
				// A string begins here
				sc.SetState(SCE_ASN1_STRING);
			else if (isAsn1Number (sc.ch))
				// A number starts here (identifier should start with a letter in ASN.1)
				sc.SetState(SCE_ASN1_SCALAR);
			else if (isAsn1Char (sc.ch))
				// An identifier starts here (identifier always start with a letter)
				sc.SetState(SCE_ASN1_IDENTIFIER);
			else if (sc.ch == ':')
				// A ::= operator starts here
				sc.SetState(SCE_ASN1_OPERATOR);
			break;
		case SCE_ASN1_COMMENT:		// A comment
			if (sc.ch == '\r' || sc.ch == '\n')
				// A comment ends here
				sc.SetState(SCE_ASN1_DEFAULT);
			break;
		case SCE_ASN1_IDENTIFIER:	// An identifier (keyword, attribute, descriptor or type)
			if (!isAsn1Char (sc.ch))
			{
				// The end of identifier is here: we can look for it in lists by now and change its state
				char s[100];
				sc.GetCurrent(s, sizeof(s));
				if (Keywords.InList(s))
					// It's a keyword, change its state
					sc.ChangeState(SCE_ASN1_KEYWORD);
				else if (Attributes.InList(s))
					// It's an attribute, change its state
					sc.ChangeState(SCE_ASN1_ATTRIBUTE);
				else if (Descriptors.InList(s))
					// It's a descriptor, change its state
					sc.ChangeState(SCE_ASN1_DESCRIPTOR);
				else if (Types.InList(s))
					// It's a type, change its state
					sc.ChangeState(SCE_ASN1_TYPE);

				// Set to default now
				sc.SetState(SCE_ASN1_DEFAULT);
			}
			break;
		case SCE_ASN1_STRING:		// A string delimited by ""
			if (sc.ch == '"')
			{
				// A string ends here
				sc.ForwardSetState(SCE_ASN1_DEFAULT);

				// To correctly manage a char sticking to the string quote
				goto asn1_default;
			}
			break;
		case SCE_ASN1_SCALAR:		// A plain number
			if (!isAsn1Number (sc.ch))
				// A number ends here
				sc.SetState(SCE_ASN1_DEFAULT);
			break;
		case SCE_ASN1_OPERATOR:		// The affectation operator ::= and wath follows (eg: ::= { org 6 } OID or ::= 12 trap)
			if (sc.ch == '{')
			{
				// An OID definition starts here: enter the sub loop
				for (; sc.More(); sc.Forward())
				{
					if (isAsn1Number (sc.ch) && (!isAsn1Char (sc.chPrev) || isAsn1Number (sc.chPrev)))
						// The OID number is highlighted
						sc.SetState(SCE_ASN1_OID);
					else if (isAsn1Char (sc.ch))
						// The OID parent identifier is plain
						sc.SetState(SCE_ASN1_IDENTIFIER);
					else
						sc.SetState(SCE_ASN1_DEFAULT);

					if (sc.ch == '}')
						// Here ends the OID and the operator sub loop: go back to main loop
						break;
				}
			}
			else if (isAsn1Number (sc.ch))
			{
				// A trap number definition starts here: enter the sub loop
				for (; sc.More(); sc.Forward())
				{
					if (isAsn1Number (sc.ch))
						// The trap number is highlighted
						sc.SetState(SCE_ASN1_OID);
					else
					{
						// The number ends here: go back to main loop
						sc.SetState(SCE_ASN1_DEFAULT);
						break;
					}
				}
			}
			else if (sc.ch != ':' && sc.ch != '=' && sc.ch != ' ')
				// The operator doesn't imply an OID definition nor a trap, back to main loop
				goto asn1_default; // To be sure to handle actually the state change
			break;
		}
	}
	sc.Complete();
}

static void FoldAsn1Doc(Sci_PositionU, Sci_Position, int, WordList *[], Accessor &styler)
{
	// No folding enabled, no reason to continue...
	if( styler.GetPropertyInt("fold") == 0 )
		return;

	// No folding implemented: doesn't make sense for ASN.1
}

static const char * const asn1WordLists[] = {
	"Keywords",
	"Attributes",
	"Descriptors",
	"Types",
	0, };


LexerModule lmAsn1(SCLEX_ASN1, ColouriseAsn1Doc, "asn1", FoldAsn1Doc, asn1WordLists);
