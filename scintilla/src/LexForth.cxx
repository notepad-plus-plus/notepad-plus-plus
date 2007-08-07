// Scintilla source code edit control
/** @file LexCrontab.cxx
 ** Lexer to use with extended crontab files used by a powerful
 ** Windows scheduler/event monitor/automation manager nnCron.
 ** (http://nemtsev.eserv.ru/)
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

bool is_whitespace(int ch){
    return ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ';
}

bool is_blank(int ch){
    return ch == '\t' || ch == ' ';
}
//#define FORTH_DEBUG
#ifdef FORTH_DEBUG
static FILE *f_debug;
#define log(x)  fputs(f_debug,x);
#else
#define log(x)
#endif

#define STATE_LOCALE
#define BL ' '

static Accessor *st;
static int cur_pos,pos1,pos2,pos0,lengthDoc;
char *buffer;

char getChar(bool is_bl){
    char ch=st->SafeGetCharAt(cur_pos);
    if(is_bl) if(is_whitespace(ch)) ch=BL;
    return ch;
}

char getCharBL(){
    char ch=st->SafeGetCharAt(cur_pos);
    return ch;
}
bool is_eol(char ch){
    return ch=='\n' || ch=='\r';
}

int parse(char ch, bool skip_eol){
// pos1 - start pos of word
// pos2 - pos after of word
// pos0 - start pos
    char c=0;
    int len;
    bool is_bl=ch==BL;
    pos0=pos1=pos2=cur_pos;
    for(;cur_pos<lengthDoc && (c=getChar(is_bl))==ch; cur_pos++){
        if(is_eol(c) && !skip_eol){
            pos2=pos1;
            return 0;
        }
    }
    pos1=cur_pos;
    pos2=pos1;
    if(cur_pos==lengthDoc) return 0;
    for(len=0;cur_pos<lengthDoc && (c=getChar(is_bl))!=ch; cur_pos++){
        if(is_eol(c) && !skip_eol) break;
        pos2++;
        buffer[len++]=c;
    }
    if(c==ch) pos2--;
    buffer[len]='\0';
#ifdef FORTH_DEBUG
    fprintf(f_debug,"parse: %c %s\n",ch,buffer);
#endif
    return len;
}

bool _is_number(char *s,int base){
    for(;*s;s++){
        int digit=((int)*s)-(int)'0';
#ifdef FORTH_DEBUG
    fprintf(f_debug,"digit: %c %d\n",*s,digit);
#endif
        if(digit>9 && base>10) digit-=7;
        if(digit<0) return false;
        if(digit>=base) return false;
    }
    return true;
}

bool is_number(char *s){
    if(strncmp(s,"0x",2)==0) return _is_number(s+2,16);
    return _is_number(s,10);
}

static void ColouriseForthDoc(unsigned int startPos, int length, int, WordList *keywordLists[], Accessor &styler)
{
    st=&styler;
    cur_pos=startPos;
    lengthDoc = startPos + length;
    buffer = new char[length];

#ifdef FORTH_DEBUG
    f_debug=fopen("c:\\sci.log","at");
#endif

    WordList &control = *keywordLists[0];
    WordList &keyword = *keywordLists[1];
    WordList &defword = *keywordLists[2];
    WordList &preword1 = *keywordLists[3];
    WordList &preword2 = *keywordLists[4];
    WordList &strings = *keywordLists[5];

    // go through all provided text segment
    // using the hand-written state machine shown below
    styler.StartAt(startPos);
    styler.StartSegment(startPos);
    while(parse(BL,true)!=0){
        if(pos0!=pos1){
            styler.ColourTo(pos0,SCE_FORTH_DEFAULT);
            styler.ColourTo(pos1-1,SCE_FORTH_DEFAULT);
        }
        if(strcmp("\\",buffer)==0){
            styler.ColourTo(pos1,SCE_FORTH_COMMENT);
            parse(1,false);
            styler.ColourTo(pos2,SCE_FORTH_COMMENT);
        }else if(strcmp("(",buffer)==0){
            styler.ColourTo(pos1,SCE_FORTH_COMMENT);
            parse(')',true);
            if(cur_pos<lengthDoc) cur_pos++;
            styler.ColourTo(cur_pos,SCE_FORTH_COMMENT);
        }else if(strcmp("[",buffer)==0){
            styler.ColourTo(pos1,SCE_FORTH_STRING);
            parse(']',true);
            if(cur_pos<lengthDoc) cur_pos++;
            styler.ColourTo(cur_pos,SCE_FORTH_STRING);
        }else if(strcmp("{",buffer)==0){
            styler.ColourTo(pos1,SCE_FORTH_LOCALE);
            parse('}',false);
            if(cur_pos<lengthDoc) cur_pos++;
            styler.ColourTo(cur_pos,SCE_FORTH_LOCALE);
        }else if(strings.InList(buffer)) {
            styler.ColourTo(pos1,SCE_FORTH_STRING);
            parse('"',false);
            if(cur_pos<lengthDoc) cur_pos++;
            styler.ColourTo(cur_pos,SCE_FORTH_STRING);
        }else if(control.InList(buffer)) {
            styler.ColourTo(pos1,SCE_FORTH_CONTROL);
            styler.ColourTo(pos2,SCE_FORTH_CONTROL);
        }else if(keyword.InList(buffer)) {
            styler.ColourTo(pos1,SCE_FORTH_KEYWORD);
            styler.ColourTo(pos2,SCE_FORTH_KEYWORD);
        }else if(defword.InList(buffer)) {
            styler.ColourTo(pos1,SCE_FORTH_KEYWORD);
            styler.ColourTo(pos2,SCE_FORTH_KEYWORD);
            parse(BL,false);
            styler.ColourTo(pos1-1,SCE_FORTH_DEFAULT);
            styler.ColourTo(pos1,SCE_FORTH_DEFWORD);
            styler.ColourTo(pos2,SCE_FORTH_DEFWORD);
        }else if(preword1.InList(buffer)) {
            styler.ColourTo(pos1,SCE_FORTH_PREWORD1);
            parse(BL,false);
            styler.ColourTo(pos2,SCE_FORTH_PREWORD1);
        }else if(preword2.InList(buffer)) {
            styler.ColourTo(pos1,SCE_FORTH_PREWORD2);
            parse(BL,false);
            styler.ColourTo(pos2,SCE_FORTH_PREWORD2);
            parse(BL,false);
            styler.ColourTo(pos1,SCE_FORTH_STRING);
            styler.ColourTo(pos2,SCE_FORTH_STRING);
        }else if(is_number(buffer)){
            styler.ColourTo(pos1,SCE_FORTH_NUMBER);
            styler.ColourTo(pos2,SCE_FORTH_NUMBER);
        }
    }
#ifdef FORTH_DEBUG
    fclose(f_debug);
#endif
    delete []buffer;
    return;
/*
                        if(control.InList(buffer)) {
                            styler.ColourTo(i,SCE_FORTH_CONTROL);
                        } else if(keyword.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_KEYWORD );
                        } else if(defword.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_DEFWORD );
//                            prev_state=SCE_FORTH_DEFWORD
                        } else if(preword1.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_PREWORD1 );
//                            state=SCE_FORTH_PREWORD1;
                        } else if(preword2.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_PREWORD2 );
                         } else {
                            styler.ColourTo(i-1,SCE_FORTH_DEFAULT);
                        }
*/
/*
    chPrev=' ';
    for (int i = startPos; i < lengthDoc; i++) {
        char ch = chNext;
        chNext = styler.SafeGetCharAt(i + 1);
        if(i!=startPos) chPrev=styler.SafeGetCharAt(i - 1);

        if (styler.IsLeadByte(ch)) {
            chNext = styler.SafeGetCharAt(i + 2);
            i++;
            continue;
        }
#ifdef FORTH_DEBUG
        fprintf(f_debug,"%c %d ",ch,state);
#endif
        switch(state) {
            case SCE_FORTH_DEFAULT:
                if(is_whitespace(ch)) {
                    // whitespace is simply ignored here...
                    styler.ColourTo(i,SCE_FORTH_DEFAULT);
                    break;
                } else if( ch == '\\' && is_blank(chNext)) {
                    // signals the start of an one line comment...
                    state = SCE_FORTH_COMMENT;
                    styler.ColourTo(i,SCE_FORTH_COMMENT);
                } else if( is_whitespace(chPrev) &&  ch == '(' &&  is_whitespace(chNext)) {
                    // signals the start of a plain comment...
                    state = SCE_FORTH_COMMENT_ML;
                    styler.ColourTo(i,SCE_FORTH_COMMENT_ML);
                } else if( isdigit(ch) ) {
                    // signals the start of a number
                    bufferCount = 0;
                    buffer[bufferCount++] = ch;
                    state = SCE_FORTH_NUMBER;
                } else if( !is_whitespace(ch)) {
                    // signals the start of an identifier
                    bufferCount = 0;
                    buffer[bufferCount++] = ch;
                    state = SCE_FORTH_IDENTIFIER;
                } else {
                    // style it the default style..
                    styler.ColourTo(i,SCE_FORTH_DEFAULT);
                }
                break;

            case SCE_FORTH_COMMENT:
                // if we find a newline here,
                // we simply go to default state
                // else continue to work on it...
                if( ch == '\n' || ch == '\r' ) {
                    state = SCE_FORTH_DEFAULT;
                } else {
                    styler.ColourTo(i,SCE_FORTH_COMMENT);
                }
                break;

            case SCE_FORTH_COMMENT_ML:
                if( ch == ')') {
                    state = SCE_FORTH_DEFAULT;
                } else {
                    styler.ColourTo(i+1,SCE_FORTH_COMMENT_ML);
                }
                break;

            case SCE_FORTH_IDENTIFIER:
                // stay  in CONF_IDENTIFIER state until we find a non-alphanumeric
                if( !is_whitespace(ch) ) {
                    buffer[bufferCount++] = ch;
                } else {
                    state = SCE_FORTH_DEFAULT;
                    buffer[bufferCount] = '\0';
#ifdef FORTH_DEBUG
        fprintf(f_debug,"\nid %s\n",buffer);
#endif

                    // check if the buffer contains a keyword,
                    // and highlight it if it is a keyword...
//                    switch(prev_state)
//                    case SCE_FORTH_DEFAULT:
                        if(control.InList(buffer)) {
                            styler.ColourTo(i,SCE_FORTH_CONTROL);
                        } else if(keyword.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_KEYWORD );
                        } else if(defword.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_DEFWORD );
//                            prev_state=SCE_FORTH_DEFWORD
                        } else if(preword1.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_PREWORD1 );
//                            state=SCE_FORTH_PREWORD1;
                        } else if(preword2.InList(buffer)) {
                            styler.ColourTo(i-1,SCE_FORTH_PREWORD2 );
                         } else {
                            styler.ColourTo(i-1,SCE_FORTH_DEFAULT);
                        }
//                        break;
//                    case

                    // push back the faulty character
                    chNext = styler[i--];
                }
                break;

            case SCE_FORTH_NUMBER:
                // stay  in CONF_NUMBER state until we find a non-numeric
                if( isdigit(ch) ) {
                    buffer[bufferCount++] = ch;
                } else {
                    state = SCE_FORTH_DEFAULT;
                    buffer[bufferCount] = '\0';
                    // Colourize here... (normal number)
                    styler.ColourTo(i-1,SCE_FORTH_NUMBER);
                    // push back a character
                    chNext = styler[i--];
                }
                break;
        }
    }
#ifdef FORTH_DEBUG
    fclose(f_debug);
#endif
    delete []buffer;
*/
}

static void FoldForthDoc(unsigned int, int, int, WordList *[],
                       Accessor &) {
}

static const char * const forthWordLists[] = {
            "control keywords",
            "keywords",
            "definition words",
            "prewords with one argument",
            "prewords with two arguments",
            "string definition keywords",
            0,
        };

LexerModule lmForth(SCLEX_FORTH, ColouriseForthDoc, "forth",FoldForthDoc,forthWordLists);
