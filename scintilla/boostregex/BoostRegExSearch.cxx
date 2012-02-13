/**
 * Copyright (c) since 2009 Simon Steele - http://untidy.net/
 * Based on the work of Simon Steele for Programmer's Notepad 2 (http://untidy.net)
 * Converted from boost::xpressive to boost::regex and performance improvements 
 * (principally caching the compiled regex), and support for UTF8 encoded text
 * (c) 2012 Dave Brotherstone - Changes for boost::regex
 *
 * 
 */
#include <stdlib.h>
#include <iterator> 
#include "scintilla.h"
#include "Platform.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "CellBuffer.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "ILexer.h"
#include "Document.h"
#include "UniConversion.h"
#include "UTF8DocumentIterator.h"
#include "AnsiDocumentIterator.h"
#include "BoostRegexSearch.h"
#include <boost/regex.hpp>
#define CP_UTF8 65001
#define SC_CP_UTF8 65001



#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

using namespace boost;


typedef basic_regex<char> charregex_t;
typedef boost::wregex wcharregex_t;
// , std::vector<boost::sub_match<DocumentIterator> >::allocator_type
typedef match_results<UTF8DocumentIterator> utf8match_t;
typedef match_results<AnsiDocumentIterator> ansimatch_t;

class BoostRegexSearch : public RegexSearchBase
{
public:
	BoostRegexSearch() : substituted(NULL), lastCompileFlags(-1) {}
	
	virtual ~BoostRegexSearch()
	{
		if (substituted)
		{
			delete [] substituted;
			substituted = NULL;
		}
	}

	
	virtual long FindText(Document* doc, int minPos, int maxPos, const char *s,
                        bool caseSensitive, bool word, bool wordStart, int flags, int *length);
	

	
	virtual const char *SubstituteByPosition(Document* doc, const char *text, int *length);

private:
	wchar_t *utf8ToWchar(const char *utf8);
	char    *wcharToUtf8(const wchar_t *w);

	charregex_t m_charre;
	wcharregex_t m_wcharre;
	
	utf8match_t m_utf8match;
	ansimatch_t m_ansimatch;
	
	char *substituted;
	std::string m_lastRegexString;
	std::string m_lastRegexUtf8string;
	int lastCompileFlags;
};

#ifdef SCI_NAMESPACE
namespace Scintilla
{
#endif

RegexSearchBase *CreateRegexSearch(CharClassify* /* charClassTable */)
{
	return new BoostRegexSearch();
}

#ifdef SCI_NAMESPACE
}
#endif

/**
 * Find text in document, supporting both forward and backward
 * searches (just pass minPos > maxPos to do a backward search)
 */
 

long BoostRegexSearch::FindText(Document* doc, int minPos, int maxPos, const char *s,
                        bool caseSensitive, bool /*word*/, bool /*wordStart*/, int searchFlags, int *length) 
{
	int startPos, endPos, increment;

	if (minPos > maxPos)
	{
		startPos = maxPos;
		endPos = minPos;
		increment = -1;
	}
	else
	{
		startPos = minPos;
		endPos = maxPos;
		increment = 1;
	}

	// Range endpoints should not be inside DBCS characters, but just in case, move them.
	startPos = doc->MovePositionOutsideChar(startPos, 1, false);
	endPos = doc->MovePositionOutsideChar(endPos, 1, false);

	
	int compileFlags(regex_constants::ECMAScript);
	if (!caseSensitive)
	{
		compileFlags |= regex_constants::icase;
	}
	bool isUtf8 = (doc->CodePage() == SC_CP_UTF8);
	
	try
	{
		
		if (compileFlags != lastCompileFlags  
			|| (isUtf8 && m_lastRegexUtf8string != s)
			|| (!isUtf8 && m_lastRegexString != s)) // Test to see if we're called with the same
			                      // regex as last time, if we are, then we don't need to recompile it
		{
			if (isUtf8)
			{
				const wchar_t* wchars = utf8ToWchar(s);
				m_wcharre = wcharregex_t(wchars, static_cast<regex_constants::syntax_option_type>(compileFlags));
				delete [] wchars;
				m_lastRegexUtf8string = s;
			}
			else
			{ // Ansi
				m_charre = charregex_t(s, static_cast<regex_constants::syntax_option_type>(compileFlags));
				m_lastRegexString = s;
			}
			lastCompileFlags = compileFlags;
		}
	}
	
	catch(regex_error& /*ex*/)
	{
		// -1 is normally used for not found, -2 is used here for invalid regex
		return -2;
	}

	// Work out the range of lines we're searching across, moving beyond an empty end-of-line
	int lineRangeStart = doc->LineFromPosition(startPos);
	int lineRangeEnd = doc->LineFromPosition(endPos);
	if ((increment == 1) &&
		(startPos >= doc->LineEnd(lineRangeStart)) &&
		(lineRangeStart < lineRangeEnd)) 
	{
		// the start position is at end of line or between line end characters.
		lineRangeStart++;
		startPos = doc->LineStart(lineRangeStart);
	}

	regex_constants::match_flag_type flags(regex_constants::match_default);

	
	
	// Work out the flags:
	if (startPos != doc->LineStart(lineRangeStart))
	{
		flags |= regex_constants::match_not_bol;
	}

	if (endPos != doc->LineEnd(lineRangeEnd))
	{
		flags |= regex_constants::match_not_eol;
	}

	if (0 == (searchFlags & SCFIND_REGEXP_DOTMATCHESNL))
	{
		flags |= regex_constants::match_not_dot_newline;
	}
	
	int pos(-1);
	int lenRet(0);
	
	
	if (doc->CodePage() == SC_CP_UTF8)
	{
		UTF8DocumentIterator end(doc, endPos, endPos);
		bool success = boost::regex_search(UTF8DocumentIterator(doc, startPos, endPos), end, m_utf8match, m_wcharre, flags);
		if (success)
		{
			pos = m_utf8match[0].first.pos();
			lenRet = m_utf8match[0].second.pos() - pos;
			
			if (increment == -1)
			{
				// Check for the last match on this line.
				int repetitions = 100;	// Break out of infinite loop
				int previousPos = pos;
				while (success && ((pos + lenRet) <= endPos)) 
				{
					if (previousPos >= pos && 0 >= (--repetitions))
						break;
					previousPos = pos;
					success = regex_search(UTF8DocumentIterator(doc, pos + 1, endPos), end, m_utf8match, m_wcharre, flags);
					// success = regex_search(DocumentIterator(doc, pos + 1, endPos), end, match, re, static_cast<regex_constants::match_flag_type>(flags));
					if (success) 
					{
						if ((pos + lenRet) <= minPos) 
						{
							pos = m_utf8match[0].first.pos();
							lenRet = m_utf8match[0].second.pos() - pos;
						} 
						else 
						{
							success = 0;
						}
					}
				}
			}
			
			*length = lenRet;
		}
	}
	else
	{
		AnsiDocumentIterator end(doc, endPos, endPos);
		
		bool success = boost::regex_search(AnsiDocumentIterator(doc, startPos, endPos), end, m_ansimatch, m_charre, flags);
		if (success)
		{
			pos = m_ansimatch[0].first.pos();
			lenRet = m_ansimatch.length();
			
			if (increment == -1)
			{
				// Check for the last match on this line.
				int repetitions = 100;	// Break out of infinite loop
				int previousPos = pos;
				while (success && ((pos + lenRet) <= endPos)) 
				{
					if (previousPos >= pos && 0 >= (--repetitions))
						break;
					previousPos = pos;
					success = regex_search(AnsiDocumentIterator(doc, pos + 1, endPos), end, m_ansimatch, m_charre, flags);
					// success = regex_search(DocumentIterator(doc, pos + 1, endPos), end, match, re, static_cast<regex_constants::match_flag_type>(flags));
					if (success) 
					{
						if ((pos + lenRet) <= minPos) 
						{
							pos = m_ansimatch[0].first.pos();
							lenRet = m_ansimatch[0].length();
						} 
						else 
						{
							success = 0;
						}
					}
				}
			}
			
			*length = lenRet;
		}
	}
	
	return pos;
}


const char *BoostRegexSearch::SubstituteByPosition(Document* doc, const char *text, int *length) {
	delete []substituted;
	substituted = NULL;
	if (doc->CodePage() == SC_CP_UTF8)
	{
		const wchar_t* wtext = utf8ToWchar(text);
		std::wstring replaced = m_utf8match.format(wtext, boost::format_all);
		delete[] wtext;
		substituted = wcharToUtf8(replaced.c_str());
		*length = strlen(substituted);
	}
	else
	{
		std::string replaced = m_ansimatch.format(text, boost::format_all);
		*length = replaced.size();
		substituted = new char[*length + 1];
		strcpy(substituted, replaced.c_str());
	}
	return substituted;
}

wchar_t *BoostRegexSearch::utf8ToWchar(const char *utf8)
{
	int utf8Size = strlen(utf8);
	int wcharSize = UTF16Length(utf8, utf8Size);
	wchar_t *w = new wchar_t[wcharSize + 1];
	UTF16FromUTF8(utf8, utf8Size, w, wcharSize + 1);
	w[wcharSize] = 0;
	
	return w;
}

char* BoostRegexSearch::wcharToUtf8(const wchar_t *w)
{
	int wcharSize = wcslen(w);
	int charSize = UTF8Length(w, wcharSize);
	char *c = new char[charSize + 1];
	UTF8FromUTF16(w, wcharSize, c, charSize);
	c[charSize] = 0;
	return c;
}