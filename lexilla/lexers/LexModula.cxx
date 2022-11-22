//	-*- coding: utf-8 -*-
//	Scintilla source code edit control
/**
 *	@file LexModula.cxx
 *	@author Dariusz "DKnoto" Knociński
 *	@date 2011/02/03
 *	@brief Lexer for Modula-2/3 documents.
 */
//	The License.txt file describes the conditions under which this software may
//	be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

#ifdef DEBUG_LEX_MODULA
#define DEBUG_STATE( p, c )\
		fprintf( stderr, "Unknown state: currentPos = %u, char = '%c'\n", static_cast<unsigned int>(p), c );
#else
#define DEBUG_STATE( p, c )
#endif

static inline bool IsDigitOfBase( unsigned ch, unsigned base ) {
	if( ch < '0' || ch > 'f' ) return false;
	if( base <= 10 ) {
		if( ch >= ( '0' + base ) ) return false;
	} else {
		if( ch > '9' ) {
			unsigned nb = base - 10;
			if( ( ch < 'A' ) || ( ch >= ( 'A' + nb ) ) ) {
				if( ( ch < 'a' ) || ( ch >= ( 'a' + nb ) ) ) {
					return false;
				}
			}
		}
	}
	return true;
}

static inline unsigned IsOperator( StyleContext & sc, WordList & op ) {
	int i;
	char s[3];

	s[0] = sc.ch;
	s[1] = sc.chNext;
	s[2] = 0;
	for( i = 0; i < op.Length(); i++ ) {
		if( ( strlen( op.WordAt(i) ) == 2 ) &&
			( s[0] == op.WordAt(i)[0] && s[1] == op.WordAt(i)[1] ) ) {
			return 2;
		}
	}
	s[1] = 0;
	for( i = 0; i < op.Length(); i++ ) {
		if( ( strlen( op.WordAt(i) ) == 1 ) &&
			( s[0] == op.WordAt(i)[0] ) ) {
			return 1;
		}
	}
	return 0;
}

static inline bool IsEOL( Accessor &styler, Sci_PositionU curPos ) {
	unsigned ch = styler.SafeGetCharAt( curPos );
	if( ( ch == '\r' && styler.SafeGetCharAt( curPos + 1 ) == '\n' ) ||
		( ch == '\n' ) ) {
		return true;
	}
	return false;
}

static inline bool checkStatement(
	Accessor &styler,
	Sci_Position &curPos,
	const char *stt, bool spaceAfter = true ) {
	int len = static_cast<int>(strlen( stt ));
	int i;
	for( i = 0; i < len; i++ ) {
		if( styler.SafeGetCharAt( curPos + i ) != stt[i] ) {
			return false;
		}
	}
	if( spaceAfter ) {
		if( ! isspace( styler.SafeGetCharAt( curPos + i ) ) ) {
			return false;
		}
	}
	curPos += ( len - 1 );
	return true;
}

static inline bool checkEndSemicolon(
	Accessor &styler,
	Sci_Position &curPos, Sci_Position endPos )
{
	const char *stt = "END";
	int len = static_cast<int>(strlen( stt ));
	int i;
	for( i = 0; i < len; i++ ) {
		if( styler.SafeGetCharAt( curPos + i ) != stt[i] ) {
			return false;
		}
	}
	while( isspace( styler.SafeGetCharAt( curPos + i ) ) ) {
		i++;
		if( ( curPos + i ) >= endPos ) return false;
	}
	if( styler.SafeGetCharAt( curPos + i ) != ';' ) {
		return false;
	}
	curPos += ( i - 1 );
	return true;
}

static inline bool checkKeyIdentOper(

	Accessor &styler,
	Sci_Position &curPos, Sci_Position endPos,
	const char *stt, const char etk ) {
	Sci_Position newPos = curPos;
	if( ! checkStatement( styler, newPos, stt ) )
		return false;
	newPos++;
	if( newPos >= endPos )
		return false;
	if( ! isspace( styler.SafeGetCharAt( newPos ) ) )
		return false;
	newPos++;
	if( newPos >= endPos )
		return false;
	while( isspace( styler.SafeGetCharAt( newPos ) ) ) {
		newPos++;
		if( newPos >= endPos )
			return false;
	}
	if( ! isalpha( styler.SafeGetCharAt( newPos ) ) )
		return false;
	newPos++;
	if( newPos >= endPos )
		return false;
	char ch;
	ch = styler.SafeGetCharAt( newPos );
	while( isalpha( ch ) || isdigit( ch ) || ch == '_' ) {
		newPos++;
		if( newPos >= endPos ) return false;
		ch = styler.SafeGetCharAt( newPos );
	}
	while( isspace( styler.SafeGetCharAt( newPos ) ) ) {
		newPos++;
		if( newPos >= endPos ) return false;
	}
	if( styler.SafeGetCharAt( newPos ) != etk )
		return false;
	curPos = newPos;
	return true;
}

static void FoldModulaDoc( Sci_PositionU startPos,
						 Sci_Position length,
						 int , WordList *[],
						 Accessor &styler)
{
	Sci_Position curLine = styler.GetLine(startPos);
	int curLevel = SC_FOLDLEVELBASE;
	Sci_Position endPos = startPos + length;
	if( curLine > 0 )
		curLevel = styler.LevelAt( curLine - 1 ) >> 16;
	Sci_Position curPos = startPos;
	int style = styler.StyleAt( curPos );
	int visChars = 0;
	int nextLevel = curLevel;

	while( curPos < endPos ) {
		if( ! isspace( styler.SafeGetCharAt( curPos ) ) ) visChars++;

		switch( style ) {
		case SCE_MODULA_COMMENT:
			if( checkStatement( styler, curPos, "(*" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "*)" ) )
				nextLevel--;
			break;

		case SCE_MODULA_DOXYCOMM:
			if( checkStatement( styler, curPos, "(**", false ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "*)" ) )
				nextLevel--;
			break;

		case SCE_MODULA_KEYWORD:
			if( checkStatement( styler, curPos, "IF" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "BEGIN" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "TRY" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "LOOP" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "FOR" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "WHILE" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "REPEAT" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "UNTIL" ) )
				nextLevel--;
			else
			if( checkStatement( styler, curPos, "WITH" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "CASE" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "TYPECASE" ) )
				nextLevel++;
			else
			if( checkStatement( styler, curPos, "LOCK" ) )
				nextLevel++;
			else
			if( checkKeyIdentOper( styler, curPos, endPos, "PROCEDURE", '(' ) )
				nextLevel++;
			else
			if( checkKeyIdentOper( styler, curPos, endPos, "END", ';' ) ) {
				Sci_Position cln = curLine;
				int clv_old = curLevel;
				Sci_Position pos;
				char ch;
				int clv_new;
				while( cln > 0 ) {
					clv_new = styler.LevelAt( cln - 1 ) >> 16;
					if( clv_new < clv_old ) {
						nextLevel--;
						pos = styler.LineStart( cln );
						while( ( ch = styler.SafeGetCharAt( pos ) ) != '\n' ) {
							if( ch == 'P' ) {
								if( styler.StyleAt(pos) == SCE_MODULA_KEYWORD )	{
									if( checkKeyIdentOper( styler, pos, endPos,
														"PROCEDURE", '(' ) ) {
										break;
									}
								}
							}
							pos++;
						}
						clv_old = clv_new;
					}
					cln--;
				}
			}
			else
			if( checkKeyIdentOper( styler, curPos, endPos, "END", '.' ) )
				nextLevel--;
			else
			if( checkEndSemicolon( styler, curPos, endPos ) )
				nextLevel--;
			else {
				while( styler.StyleAt( curPos + 1 ) == SCE_MODULA_KEYWORD )
					curPos++;
			}
			break;

		default:
			break;
		}

		if( IsEOL( styler, curPos ) || ( curPos == endPos - 1 ) ) {
			int efectiveLevel = curLevel | nextLevel << 16;
			if( visChars == 0 )
				efectiveLevel |= SC_FOLDLEVELWHITEFLAG;
			if( curLevel < nextLevel )
				efectiveLevel |= SC_FOLDLEVELHEADERFLAG;
			if( efectiveLevel != styler.LevelAt(curLine) ) {
				styler.SetLevel(curLine, efectiveLevel );
			}
			curLine++;
			curLevel = nextLevel;
			if( IsEOL( styler, curPos ) && ( curPos == endPos - 1 ) ) {
				styler.SetLevel( curLine, ( curLevel | curLevel << 16)
								| SC_FOLDLEVELWHITEFLAG);
			}
			visChars = 0;
		}
		curPos++;
		style = styler.StyleAt( curPos );
	}
}

static inline bool skipWhiteSpaces( StyleContext & sc ) {
	while( isspace( sc.ch ) ) {
		sc.SetState( SCE_MODULA_DEFAULT );
		if( sc.More() )
			sc.Forward();
		else
			return false;
	}
	return true;
}

static void ColouriseModulaDoc(	Sci_PositionU startPos,
									Sci_Position length,
									int initStyle,
									WordList *wl[],
									Accessor &styler ) {
	WordList& keyWords		= *wl[0];
	WordList& reservedWords	= *wl[1];
	WordList& operators 	= *wl[2];
	WordList& pragmaWords 	= *wl[3];
	WordList& escapeCodes	= *wl[4];
	WordList& doxyKeys		= *wl[5];

	const int BUFLEN = 128;

	char	buf[BUFLEN];
	int		i, kl;

	Sci_Position  charPos = 0;

	StyleContext sc( startPos, length, initStyle, styler );

	while( sc.More() ) 	{
		switch( sc.state )	{
		case SCE_MODULA_DEFAULT:
			if( ! skipWhiteSpaces( sc ) ) break;

			if( sc.ch == '(' && sc.chNext == '*' ) {
				if( sc.GetRelative(2) == '*' ) {
					sc.SetState( SCE_MODULA_DOXYCOMM );
					sc.Forward();
				} else {
					sc.SetState( SCE_MODULA_COMMENT );
				}
				sc.Forward();
			}
			else
			if( isalpha( sc.ch ) ) {
				if( isupper( sc.ch ) && isupper( sc.chNext ) ) {
					for( i = 0; i < BUFLEN - 1; i++ ) {
						buf[i] = sc.GetRelative(i);
						if( !isalpha( buf[i] ) && !(buf[i] == '_') )
							break;
					}
					kl = i;
					buf[kl] = 0;

					if( keyWords.InList( buf ) ) {
						sc.SetState( SCE_MODULA_KEYWORD );
						sc.Forward( kl );
						sc.SetState( SCE_MODULA_DEFAULT );
						continue;
					}
					else
					if( reservedWords.InList( buf ) ) {
						sc.SetState( SCE_MODULA_RESERVED );
						sc.Forward( kl );
						sc.SetState( SCE_MODULA_DEFAULT );
						continue;
					} else {
						/** check procedure identifier */
					}
				} else {
					for( i = 0; i < BUFLEN - 1; i++ ) {
						buf[i] = sc.GetRelative(i);
						if( !isalpha( buf[i] ) &&
							!isdigit( buf[i] ) &&
							!(buf[i] == '_') )
							break;
					}
					kl = i;
					buf[kl] = 0;

					sc.SetState( SCE_MODULA_DEFAULT );
					sc.Forward( kl );
					continue;
				}
			}
			else
			if( isdigit( sc.ch ) ) {
				sc.SetState( SCE_MODULA_NUMBER );
				continue;
			}
			else
			if( sc.ch == '\"' ) {
				sc.SetState( SCE_MODULA_STRING );
			}
			else
			if( sc.ch == '\'' ) {
				charPos = sc.currentPos;
				sc.SetState( SCE_MODULA_CHAR );
			}
			else
			if( sc.ch == '<' && sc.chNext == '*' ) {
				sc.SetState( SCE_MODULA_PRAGMA );
				sc.Forward();
			} else {
				unsigned len = IsOperator( sc, operators );
				if( len > 0 ) {
					sc.SetState( SCE_MODULA_OPERATOR );
					sc.Forward( len );
					sc.SetState( SCE_MODULA_DEFAULT );
					continue;
				} else {
					DEBUG_STATE( sc.currentPos, sc.ch );
				}
			}
			break;

		case SCE_MODULA_COMMENT:
			if( sc.ch == '*' && sc.chNext == ')' ) {
				sc.Forward( 2 );
				sc.SetState( SCE_MODULA_DEFAULT );
				continue;
			}
			break;

		case SCE_MODULA_DOXYCOMM:
			switch( sc.ch ) {
			case '*':
				if( sc.chNext == ')' ) {
					sc.Forward( 2 );
					sc.SetState( SCE_MODULA_DEFAULT );
					continue;
				}
				break;

			case '@':
				if( islower( sc.chNext ) ) {
					for( i = 0; i < BUFLEN - 1; i++ ) {
						buf[i] = sc.GetRelative(i+1);
						if( isspace( buf[i] ) ) break;
					}
					buf[i] = 0;
					kl = i;

					if( doxyKeys.InList( buf ) ) {
						sc.SetState( SCE_MODULA_DOXYKEY );
						sc.Forward( kl + 1 );
						sc.SetState( SCE_MODULA_DOXYCOMM );
					}
				}
				break;

			default:
				break;
			}
			break;

		case SCE_MODULA_NUMBER:
			{
				buf[0] = sc.ch;
				for( i = 1; i < BUFLEN - 1; i++ ) {
					buf[i] = sc.GetRelative(i);
					if( ! isdigit( buf[i] ) )
						break;
				}
				kl = i;
				buf[kl] = 0;

				switch( sc.GetRelative(kl) ) {
				case '_':
					{
						int base = atoi( buf );
						if( base < 2 || base > 16 ) {
							sc.SetState( SCE_MODULA_BADSTR );
						} else {
							int imax;

							kl++;
							for( i = 0; i < BUFLEN - 1; i++ ) {
								buf[i] = sc.GetRelative(kl+i);
								if( ! IsDigitOfBase( buf[i], 16 ) ) {
									break;
								}
							}
							imax = i;
							for( i = 0; i < imax; i++ ) {
								if( ! IsDigitOfBase( buf[i], base ) ) {
									sc.SetState( SCE_MODULA_BADSTR );
									break;
								}
							}
							kl += imax;
						}
						sc.SetState( SCE_MODULA_BASENUM );
						for( i = 0; i < kl; i++ ) {
							sc.Forward();
						}
						sc.SetState( SCE_MODULA_DEFAULT );
						continue;
					}
					break;

				case '.':
					if( sc.GetRelative(kl+1) == '.' ) {
						kl--;
						for( i = 0; i < kl; i++ ) {
							sc.Forward();
						}
						sc.Forward();
						sc.SetState( SCE_MODULA_DEFAULT );
						continue;
					} else {
						bool doNext = false;

						kl++;

						buf[0] = sc.GetRelative(kl);
						if( isdigit( buf[0] ) ) {
							for( i = 0;; i++ ) {
								if( !isdigit(sc.GetRelative(kl+i)) )
									break;
							}
							kl += i;
							buf[0] = sc.GetRelative(kl);

							switch( buf[0] )
							{
							case 'E':
							case 'e':
							case 'D':
							case 'd':
							case 'X':
							case 'x':
								kl++;
								buf[0] = sc.GetRelative(kl);
								if( buf[0] == '-' || buf[0] == '+' ) {
									kl++;
								}
								buf[0] = sc.GetRelative(kl);
								if( isdigit( buf[0] ) ) {
									for( i = 0;; i++ ) {
										if( !isdigit(sc.GetRelative(kl+i)) ) {
											buf[0] = sc.GetRelative(kl+i);
											break;
										}
									}
									kl += i;
									doNext = true;
								} else {
									sc.SetState( SCE_MODULA_BADSTR );
								}
								break;

							default:
								doNext = true;
								break;
							}
						} else {
							sc.SetState( SCE_MODULA_BADSTR );
						}

						if( doNext ) {
							if( ! isspace( buf[0] ) &&
								buf[0] != ')' &&
								buf[0] != '>' &&
								buf[0] != '<' &&
								buf[0] != '=' &&
								buf[0] != '#' &&
								buf[0] != '+' &&
								buf[0] != '-' &&
								buf[0] != '*' &&
								buf[0] != '/' &&
								buf[0] != ',' &&
								buf[0] != ';'
								) {
								sc.SetState( SCE_MODULA_BADSTR );
							} else {
								kl--;
							}
						}
					}
					sc.SetState( SCE_MODULA_FLOAT );
					for( i = 0; i < kl; i++ ) {
						sc.Forward();
					}
					sc.SetState( SCE_MODULA_DEFAULT );
					continue;
					break;

				default:
					for( i = 0; i < kl; i++ ) {
						sc.Forward();
					}
					break;
				}
				sc.SetState( SCE_MODULA_DEFAULT );
				continue;
			}
			break;

		case SCE_MODULA_STRING:
			if( sc.ch == '\"' ) {
				sc.Forward();
				sc.SetState( SCE_MODULA_DEFAULT );
				continue;
			} else {
				if( sc.ch == '\\' ) {
					i = 1;
					if( IsDigitOfBase( sc.chNext, 8 ) ) {
						for( i = 1; i < BUFLEN - 1; i++ ) {
							if( ! IsDigitOfBase(sc.GetRelative(i+1), 8 ) )
								break;
						}
						if( i == 3 ) {
							sc.SetState( SCE_MODULA_STRSPEC );
						} else {
							sc.SetState( SCE_MODULA_BADSTR );
						}
					} else {
						buf[0] = sc.chNext;
						buf[1] = 0;

						if( escapeCodes.InList( buf ) ) {
							sc.SetState( SCE_MODULA_STRSPEC );
						} else {
							sc.SetState( SCE_MODULA_BADSTR );
						}
					}
					sc.Forward(i+1);
					sc.SetState( SCE_MODULA_STRING );
					continue;
				}
			}
			break;

		case SCE_MODULA_CHAR:
			if( sc.ch == '\'' ) {
				sc.Forward();
				sc.SetState( SCE_MODULA_DEFAULT );
				continue;
			}
			else
			if( ( sc.currentPos - charPos ) == 1 ) {
				if( sc.ch == '\\' ) {
					i = 1;
					if( IsDigitOfBase( sc.chNext, 8 ) ) {
						for( i = 1; i < BUFLEN - 1; i++ ) {
							if( ! IsDigitOfBase(sc.GetRelative(i+1), 8 ) )
								break;
						}
						if( i == 3 ) {
							sc.SetState( SCE_MODULA_CHARSPEC );
						} else {
							sc.SetState( SCE_MODULA_BADSTR );
						}
					} else {
						buf[0] = sc.chNext;
						buf[1] = 0;

						if( escapeCodes.InList( buf ) ) {
							sc.SetState( SCE_MODULA_CHARSPEC );
						} else {
							sc.SetState( SCE_MODULA_BADSTR );
						}
					}
					sc.Forward(i+1);
					sc.SetState( SCE_MODULA_CHAR );
					continue;
				}
			} else {
				sc.SetState( SCE_MODULA_BADSTR );
				sc.Forward();
				sc.SetState( SCE_MODULA_CHAR );
				continue;
			}
			break;

		case SCE_MODULA_PRAGMA:
			if( sc.ch == '*' && sc.chNext == '>' ) {
				sc.Forward();
				sc.Forward();
				sc.SetState( SCE_MODULA_DEFAULT );
				continue;
			}
			else
			if( isupper( sc.ch ) && isupper( sc.chNext ) ) {
				buf[0] = sc.ch;
				buf[1] = sc.chNext;
				for( i = 2; i < BUFLEN - 1; i++ ) {
					buf[i] = sc.GetRelative(i);
					if( !isupper( buf[i] ) )
						break;
				}
				kl = i;
				buf[kl] = 0;
				if( pragmaWords.InList( buf ) ) {
					sc.SetState( SCE_MODULA_PRGKEY );
					sc.Forward( kl );
					sc.SetState( SCE_MODULA_PRAGMA );
					continue;
				}
			}
			break;

		default:
			break;
		}
		sc.Forward();
	}
	sc.Complete();
}

static const char *const modulaWordListDesc[] =
{
	"Keywords",
	"ReservedKeywords",
	"Operators",
	"PragmaKeyswords",
	"EscapeCodes",
	"DoxygeneKeywords",
	0
};

LexerModule lmModula( SCLEX_MODULA, ColouriseModulaDoc, "modula", FoldModulaDoc,
					  modulaWordListDesc);
