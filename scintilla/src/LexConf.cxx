// Scintilla source code edit control
/** @file LexConf.cxx
 ** Lexer for Apache Configuration Files.
 **
 ** First working version contributed by Ahmad Zawawi <zeus_go64@hotmail.com> on October 28, 2000.
 ** i created this lexer because i needed something pretty when dealing
 ** when Apache Configuration files...
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

static void ColouriseConfDoc(unsigned int startPos, int length, int, WordList *keywordLists[], Accessor &styler)
{
	int state = SCE_CONF_DEFAULT;
	char chNext = styler[startPos];
	int lengthDoc = startPos + length;
	// create a buffer large enough to take the largest chunk...
	char *buffer = new char[length];
	int bufferCount = 0;

	// this assumes that we have 2 keyword list in conf.properties
	WordList &directives = *keywordLists[0];
	WordList &params = *keywordLists[1];

	// go through all provided text segment
	// using the hand-written state machine shown below
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	for (int i = startPos; i < lengthDoc; i++) {
		char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);

		if (styler.IsLeadByte(ch)) {
			chNext = styler.SafeGetCharAt(i + 2);
			i++;
			continue;
		}
		switch(state) {
			case SCE_CONF_DEFAULT:
				if( ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ') {
					// whitespace is simply ignored here...
					styler.ColourTo(i,SCE_CONF_DEFAULT);
					break;
				} else if( ch == '#' ) {
					// signals the start of a comment...
					state = SCE_CONF_COMMENT;
					styler.ColourTo(i,SCE_CONF_COMMENT);
				} else if( ch == '.' /*|| ch == '/'*/) {
					// signals the start of a file...
					state = SCE_CONF_EXTENSION;
					styler.ColourTo(i,SCE_CONF_EXTENSION);
				} else if( ch == '"') {
					state = SCE_CONF_STRING;
					styler.ColourTo(i,SCE_CONF_STRING);
				} else if( ispunct(ch) ) {
					// signals an operator...
					// no state jump necessary for this
					// simple case...
					styler.ColourTo(i,SCE_CONF_OPERATOR);
				} else if( isalpha(ch) ) {
					// signals the start of an identifier
					bufferCount = 0;
					buffer[bufferCount++] = static_cast<char>(tolower(ch));
					state = SCE_CONF_IDENTIFIER;
				} else if( isdigit(ch) ) {
					// signals the start of a number
					bufferCount = 0;
					buffer[bufferCount++] = ch;
					//styler.ColourTo(i,SCE_CONF_NUMBER);
					state = SCE_CONF_NUMBER;
				} else {
					// style it the default style..
					styler.ColourTo(i,SCE_CONF_DEFAULT);
				}
				break;

			case SCE_CONF_COMMENT:
				// if we find a newline here,
				// we simply go to default state
				// else continue to work on it...
				if( ch == '\n' || ch == '\r' ) {
					state = SCE_CONF_DEFAULT;
				} else {
					styler.ColourTo(i,SCE_CONF_COMMENT);
				}
				break;

			case SCE_CONF_EXTENSION:
				// if we find a non-alphanumeric char,
				// we simply go to default state
				// else we're still dealing with an extension...
				if( isalnum(ch) || (ch == '_') ||
					(ch == '-') || (ch == '$') ||
					(ch == '/') || (ch == '.') || (ch == '*') )
				{
					styler.ColourTo(i,SCE_CONF_EXTENSION);
				} else {
					state = SCE_CONF_DEFAULT;
					chNext = styler[i--];
				}
				break;

			case SCE_CONF_STRING:
				// if we find the end of a string char, we simply go to default state
				// else we're still dealing with an string...
				if( (ch == '"' && styler.SafeGetCharAt(i-1)!='\\') || (ch == '\n') || (ch == '\r') ) {
					state = SCE_CONF_DEFAULT;
				}
				styler.ColourTo(i,SCE_CONF_STRING);
				break;

			case SCE_CONF_IDENTIFIER:
				// stay  in CONF_IDENTIFIER state until we find a non-alphanumeric
				if( isalnum(ch) || (ch == '_') || (ch == '-') || (ch == '/') || (ch == '$') || (ch == '.') || (ch == '*')) {
					buffer[bufferCount++] = static_cast<char>(tolower(ch));
				} else {
					state = SCE_CONF_DEFAULT;
					buffer[bufferCount] = '\0';

					// check if the buffer contains a keyword, and highlight it if it is a keyword...
					if(directives.InList(buffer)) {
						styler.ColourTo(i-1,SCE_CONF_DIRECTIVE );
					} else if(params.InList(buffer)) {
						styler.ColourTo(i-1,SCE_CONF_PARAMETER );
					} else if(strchr(buffer,'/') || strchr(buffer,'.')) {
						styler.ColourTo(i-1,SCE_CONF_EXTENSION);
					} else {
						styler.ColourTo(i-1,SCE_CONF_DEFAULT);
					}

					// push back the faulty character
					chNext = styler[i--];

				}
				break;

			case SCE_CONF_NUMBER:
				// stay  in CONF_NUMBER state until we find a non-numeric
				if( isdigit(ch) || ch == '.') {
					buffer[bufferCount++] = ch;
				} else {
					state = SCE_CONF_DEFAULT;
					buffer[bufferCount] = '\0';

					// Colourize here...
					if( strchr(buffer,'.') ) {
						// it is an IP address...
						styler.ColourTo(i-1,SCE_CONF_IP);
					} else {
						// normal number
						styler.ColourTo(i-1,SCE_CONF_NUMBER);
					}

					// push back a character
					chNext = styler[i--];
				}
				break;

		}
	}
	delete []buffer;
}

static const char * const confWordListDesc[] = {
	"Directives",
	"Parameters",
	0
};

LexerModule lmConf(SCLEX_CONF, ColouriseConfDoc, "conf", 0, confWordListDesc);
