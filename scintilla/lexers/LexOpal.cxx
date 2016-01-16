// Scintilla source code edit control
/** @file LexOpal.cxx
 ** Lexer for OPAL (functional language similar to Haskell)
 ** Written by Sebastian Pipping <webmaster@hartwork.org>
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

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

inline static void getRange( Sci_PositionU start, Sci_PositionU end, Accessor & styler, char * s, Sci_PositionU len )
{
	Sci_PositionU i = 0;
	while( ( i < end - start + 1 ) && ( i < len - 1 ) )
	{
		s[i] = static_cast<char>( styler[ start + i ] );
		i++;
	}
	s[ i ] = '\0';
}

inline bool HandleString( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler )
{
	char ch;

	// Wait for string to close
	bool even_backslash_count = true; // Without gaps in between
	cur++; // Skip initial quote
	for( ; ; )
	{
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_STRING );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( ( ch == '\015' ) || ( ch == '\012' ) ) // Deny multi-line strings
		{
			styler.ColourTo( cur - 1, SCE_OPAL_STRING );
			styler.StartSegment( cur );
			return true;
		}
		else
		{
			if( even_backslash_count )
			{
				if( ch == '"' )
				{
					styler.ColourTo( cur, SCE_OPAL_STRING );
					cur++;
					if( cur >= one_too_much )
					{
						return false; // STOP
					}
					else
					{
						styler.StartSegment( cur );
						return true;
					}
				}
				else if( ch == '\\' )
				{
					even_backslash_count = false;
				}
			}
			else
			{
				even_backslash_count = true;
			}
		}

		cur++;
	}
}

inline bool HandleCommentBlock( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler, bool could_fail )
{
	char ch;

	if( could_fail )
	{
		cur++;
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( ch != '*' )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
			styler.StartSegment( cur );
			return true;
		}
	}

	// Wait for comment close
	cur++;
	bool star_found = false;
	for( ; ; )
	{
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_COMMENT_BLOCK );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( star_found )
		{
			if( ch == '/' )
			{
				styler.ColourTo( cur, SCE_OPAL_COMMENT_BLOCK );
				cur++;
				if( cur >= one_too_much )
				{
					return false; // STOP
				}
				else
				{
					styler.StartSegment( cur );
					return true;
				}
			}
			else if( ch != '*' )
			{
				star_found = false;
			}
		}
		else if( ch == '*' )
		{
			star_found = true;
		}
		cur++;
	}
}

inline bool HandleCommentLine( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler, bool could_fail )
{
	char ch;

	if( could_fail )
	{
		cur++;
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( ch != '-' )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
			styler.StartSegment( cur );
			return true;
		}

		cur++;
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( ( ch != ' ' ) && ( ch != '\t' ) )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
			styler.StartSegment( cur );
			return true;
		}
	}

	// Wait for end of line
	bool fifteen_found = false;

	for( ; ; )
	{
		cur++;

		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_COMMENT_LINE );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( fifteen_found )
		{
/*
			if( ch == '\012' )
			{
				// One newline on Windows (015, 012)
			}
			else
			{
				// One newline on MAC (015) and another char
			}
*/
			cur--;
			styler.ColourTo( cur - 1, SCE_OPAL_COMMENT_LINE );
			styler.StartSegment( cur );
			return true;
		}
		else
		{
			if( ch == '\015' )
			{
				fifteen_found = true;
			}
			else if( ch == '\012' )
			{
				// One newline on Linux (012)
				styler.ColourTo( cur - 1, SCE_OPAL_COMMENT_LINE );
				styler.StartSegment( cur );
				return true;
			}
		}
	}
}

inline bool HandlePar( Sci_PositionU & cur, Accessor & styler )
{
	styler.ColourTo( cur, SCE_OPAL_PAR );

	cur++;

	styler.StartSegment( cur );
	return true;
}

inline bool HandleSpace( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler )
{
	char ch;

	cur++;
	for( ; ; )
	{
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_SPACE );
			return false;
		}

		ch = styler.SafeGetCharAt( cur );
		switch( ch )
		{
		case ' ':
		case '\t':
		case '\015':
		case '\012':
			cur++;
			break;

		default:
			styler.ColourTo( cur - 1, SCE_OPAL_SPACE );
			styler.StartSegment( cur );
			return true;
		}
	}
}

inline bool HandleInteger( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler )
{
	char ch;

	for( ; ; )
	{
		cur++;
		if( cur >= one_too_much )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_INTEGER );
			return false; // STOP
		}

		ch = styler.SafeGetCharAt( cur );
		if( !( IsASCII( ch ) && isdigit( ch ) ) )
		{
			styler.ColourTo( cur - 1, SCE_OPAL_INTEGER );
			styler.StartSegment( cur );
			return true;
		}
	}
}

inline bool HandleWord( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler, WordList * keywordlists[] )
{
	char ch;
	const Sci_PositionU beg = cur;

	cur++;
	for( ; ; )
	{
		ch = styler.SafeGetCharAt( cur );
		if( ( ch != '_' ) && ( ch != '-' ) &&
			!( IsASCII( ch ) && ( islower( ch ) || isupper( ch ) || isdigit( ch ) ) ) ) break;

		cur++;
		if( cur >= one_too_much )
		{
			break;
		}
	}

	const Sci_Position ide_len = cur - beg + 1;
	char * ide = new char[ ide_len ];
	getRange( beg, cur, styler, ide, ide_len );

	WordList & keywords    = *keywordlists[ 0 ];
	WordList & classwords  = *keywordlists[ 1 ];

	if( keywords.InList( ide ) ) // Keyword
	{
		delete [] ide;

		styler.ColourTo( cur - 1, SCE_OPAL_KEYWORD );
		if( cur >= one_too_much )
		{
			return false; // STOP
		}
		else
		{
			styler.StartSegment( cur );
			return true;
		}
	}
	else if( classwords.InList( ide ) ) // Sort
	{
		delete [] ide;

		styler.ColourTo( cur - 1, SCE_OPAL_SORT );
		if( cur >= one_too_much )
		{
			return false; // STOP
		}
		else
		{
			styler.StartSegment( cur );
			return true;
		}
	}
	else if( !strcmp( ide, "true" ) || !strcmp( ide, "false" ) ) // Bool const
	{
		delete [] ide;

		styler.ColourTo( cur - 1, SCE_OPAL_BOOL_CONST );
		if( cur >= one_too_much )
		{
			return false; // STOP
		}
		else
		{
			styler.StartSegment( cur );
			return true;
		}
	}
	else // Unknown keyword
	{
		delete [] ide;

		styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
		if( cur >= one_too_much )
		{
			return false; // STOP
		}
		else
		{
			styler.StartSegment( cur );
			return true;
		}
	}

}

inline bool HandleSkip( Sci_PositionU & cur, Sci_PositionU one_too_much, Accessor & styler )
{
	cur++;
	styler.ColourTo( cur - 1, SCE_OPAL_DEFAULT );
	if( cur >= one_too_much )
	{
		return false; // STOP
	}
	else
	{
		styler.StartSegment( cur );
		return true;
	}
}

static void ColouriseOpalDoc( Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor & styler )
{
	styler.StartAt( startPos );
	styler.StartSegment( startPos );

	Sci_PositionU & cur = startPos;
	const Sci_PositionU one_too_much = startPos + length;

	int state = initStyle;

	for( ; ; )
	{
		switch( state )
		{
		case SCE_OPAL_KEYWORD:
		case SCE_OPAL_SORT:
			if( !HandleWord( cur, one_too_much, styler, keywordlists ) ) return;
			state = SCE_OPAL_DEFAULT;
			break;

		case SCE_OPAL_INTEGER:
			if( !HandleInteger( cur, one_too_much, styler ) ) return;
			state = SCE_OPAL_DEFAULT;
			break;

		case SCE_OPAL_COMMENT_BLOCK:
			if( !HandleCommentBlock( cur, one_too_much, styler, false ) ) return;
			state = SCE_OPAL_DEFAULT;
			break;

		case SCE_OPAL_COMMENT_LINE:
			if( !HandleCommentLine( cur, one_too_much, styler, false ) ) return;
			state = SCE_OPAL_DEFAULT;
			break;

		case SCE_OPAL_STRING:
			if( !HandleString( cur, one_too_much, styler ) ) return;
			state = SCE_OPAL_DEFAULT;
			break;

		default: // SCE_OPAL_DEFAULT:
			{
				char ch = styler.SafeGetCharAt( cur );

				switch( ch )
				{
				// String
				case '"':
					if( !HandleString( cur, one_too_much, styler ) ) return;
					break;

				// Comment block
				case '/':
					if( !HandleCommentBlock( cur, one_too_much, styler, true ) ) return;
					break;

				// Comment line
				case '-':
					if( !HandleCommentLine( cur, one_too_much, styler, true ) ) return;
					break;

				// Par
				case '(':
				case ')':
				case '[':
				case ']':
				case '{':
				case '}':
					if( !HandlePar( cur, styler ) ) return;
					break;

				// Whitespace
				case ' ':
				case '\t':
				case '\015':
				case '\012':
					if( !HandleSpace( cur, one_too_much, styler ) ) return;
					break;

				default:
					{
						// Integer
						if( IsASCII( ch ) && isdigit( ch ) )
						{
							if( !HandleInteger( cur, one_too_much, styler ) ) return;
						}

						// Keyword
						else if( IsASCII( ch ) && ( islower( ch ) || isupper( ch ) ) )
						{
							if( !HandleWord( cur, one_too_much, styler, keywordlists ) ) return;

						}

						// Skip
						else
						{
							if( !HandleSkip( cur, one_too_much, styler ) ) return;
						}
					}
				}

				break;
			}
		}
	}
}

static const char * const opalWordListDesc[] = {
	"Keywords",
	"Sorts",
	0
};

LexerModule lmOpal(SCLEX_OPAL, ColouriseOpalDoc, "opal", NULL, opalWordListDesc);
