#include "stdafx.h"

#include "lexer.h"
#include "parse_int.h"

const char* TlDatLexer::name = "TlDat";
const wchar_t* TlDatLexer::wname = L"TlDat";
const wchar_t* TlDatLexer::statusText = L"Torchlight data file";

namespace {

enum class ValType {
	unknown,
	int32,
	int64,
	uint32,
	bin32,
	bin64,
	bool_,
	string,
	translate,
	note
};

bool isLower(char c) { return c >= 'a' && c <= 'z'; }
char toUpper(char c) { return isLower(c)? c & ~0x20: c; }
bool isWhitespace(char c) { return c == ' ' || c == '\t'; }
bool isEol(char c) { return c == '\n' || c == '\r'; }
bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isSign(char c) { return c == '-' || c == '+'; }
bool isExponent(char c) { return c == 'e' || c == 'E'; }

bool nameEquals(unsigned int start, LexAccessor& doc, const char* val) {
	for(; *val != '\0'; ++start, ++val) {
		assert(!isLower(*val));
		if(toUpper(doc[start]) != *val) return false;
	}
	return true;
}

ValType getType(unsigned int start, unsigned int end, LexAccessor& doc) {
	switch(end - start)  {
	case sizeof("BOOL") - 1:
		return 
			nameEquals(start, doc, "BOOL")? ValType::bool_:
			nameEquals(start, doc, "NOTE")? ValType::note:
			ValType::unknown; 
	case sizeof("FLOAT") - 1:
		return nameEquals(start, doc, "FLOAT")? ValType::bin32: ValType::unknown;
	case sizeof("DOUBLE") - 1:
		return
			nameEquals(start, doc, "DOUBLE")? ValType::bin64:
			nameEquals(start, doc, "STRING")? ValType::string:
			ValType::unknown;
	case sizeof("INTEGER") - 1:
		return nameEquals(start, doc, "INTEGER")? ValType::int32: ValType::unknown;
	case sizeof("INTEGER64") - 1:
		return
			nameEquals(start, doc, "INTEGER64")? ValType::int64:
			nameEquals(start, doc, "TRANSLATE")? ValType::translate:
			ValType::unknown;
	case sizeof("UNSIGNED INT") - 1:
		return nameEquals(start, doc, "UNSIGNED INT")? ValType::uint32: ValType::unknown;
	}

	return ValType::unknown;
}

bool find(char c, unsigned int& pos, unsigned int end, LexAccessor& doc) {
	assert(pos <= end);
	for(;; ++pos) {
		if(pos == end) return false;
		if(doc[pos] == c) return true; 
	}
}

unsigned int findEol(unsigned int start, LexAccessor& doc) {
	for(;; ++start)
		if(isEol(doc.SafeGetCharAt(start, '\n'))) return start;
}

bool readInt(unsigned int& pos, unsigned int end, LexAccessor& doc) {
	assert(pos <= end);
	if(pos == end) return false;
	if(!isDigit(doc[pos])) return false;

	while(++pos != end && isDigit(doc[pos])) {}
	return true;
}

bool readUInt32(unsigned int start, unsigned int end, LexAccessor& doc) {
	static const std::size_t digits = 10;
	
	assert(start < end);
	if(end - start > digits) return false;
	char num[digits];

	char* i = num;
	for(; start != end; ++i, ++start)
		*i = doc[start];

	return isValidUInt32(num, i) == ParseError::none;
}

bool readInt32(unsigned int start, unsigned int end, LexAccessor& doc) {
	static const std::size_t digits = 11;
	
	assert(start < end);
	if(end - start > digits) return false;
	char num[digits];

	char* i = num;
	for(; start != end; ++i, ++start)
		*i = doc[start];

	return isValidInt32(num, i) == ParseError::none;
}

bool readInt64(unsigned int start, unsigned int end, LexAccessor& doc) {
	static const std::size_t digits = 20;
	
	assert(start < end);
	if(end - start > digits) return false;
	char num[digits];

	char* i = num;
	for(; start != end; ++i, ++start)
		*i = doc[start];

	return isValidInt64(num, i) == ParseError::none;
}

bool isValidBool(unsigned int start, unsigned int end, LexAccessor& doc) {
	assert(start <= end);
	switch(end - start) {
	default: return false;
	case 1: return doc[start] == '1' || doc[start] == '0';
	case 4: return nameEquals(start, doc, "TRUE");
	case 5: return nameEquals(start, doc, "FALSE");
	}
}

void LexLine(unsigned int start, unsigned int end, int line, LexAccessor& doc) {
	assert(start < end);
	char c;
	
	for(;;) {
		c = doc[start++];
		if(start == end) return;
		if(!isWhitespace(c)) break;
		doc.ColourTo(start - 1, TextStyle::default_);
	}
	
	if(c == '[') {
		bool isCloseTag = false;
		if(doc[start] == '/') isCloseTag = true;

		if(!find(']', start, end, doc)) goto MarkDefaultLine;

		if(isCloseTag) {
			doc.ColourTo(start, TextStyle::closeTag);
			doc.SetLineState(line, TextStyle::closeTag);
		} else {
			doc.ColourTo(start, TextStyle::openTag);
			doc.SetLineState(line, TextStyle::openTag);
		}

		if(++start != end) {
			do {
				if(!isWhitespace(doc[start])) {
					unsigned int indicEnd = end;
					for(; --indicEnd != start && isWhitespace(doc[indicEnd]);) {}

					doc.IndicatorFill(start, indicEnd + 1, IndicatorStyle::error, 1);
					break;
				}
			} while(++start != end);
			doc.ColourTo(end - 1, TextStyle::default_);
		}

		return;
	} else if(c == '<') {
		unsigned int old = start;
		if(!find('>', start, end, doc)) goto MarkDefaultLine;

		ValType type = getType(old, start, doc);
		doc.ColourTo(start++, TextStyle::fieldType);
		
		if(type == ValType::unknown)
			doc.IndicatorFill(old - 1, start, IndicatorStyle::error, 1); 

		if(start == end) return;
		if(!find(':', start, end, doc)) goto MarkDefaultLine;
		doc.ColourTo(start, TextStyle::fieldName);

		if(++start == end) return;
		if(type == ValType::string || type == ValType::translate || type == ValType::unknown) {
			doc.ColourTo(end - 1, TextStyle::fieldValue);
			return;
		} else if(type == ValType::note) {
			doc.ColourTo(end - 1, TextStyle::fieldValueNote);
			return;
		} else if(type == ValType::bool_)
			doc.ColourTo(end - 1, TextStyle::fieldValueBool);
		else
			doc.ColourTo(end - 1, TextStyle::fieldValueNumber);

		for(;;) {
			if(!isWhitespace(doc[start])) break;
			if(++start == end) return;
		}

		old = start;
		while(++start != end && !isWhitespace(doc[start])) {}

		if(start != end) {
			for(++start; end-- != start;)
				if(!isWhitespace(doc[end])) {
					doc.IndicatorFill(old, end + 1, IndicatorStyle::error, 1);
					return;
				}
		}
		start = old;

		if(type == ValType::bool_)
			if(isValidBool(start, end, doc)) return;
			else goto MarkInvalidValue;
		else if(type == ValType::int32)
			if(readInt32(start, end, doc)) return;
			else goto MarkInvalidValue;
		else if(type == ValType::uint32)
			if(readUInt32(start, end, doc)) return;
			else goto MarkInvalidValue;
		else if(type == ValType::int64)
			if(readInt64(start, end, doc)) return;
			else goto MarkInvalidValue;

		if(isSign(doc[start])) ++start;
		if(!readInt(start, end, doc)) goto MarkInvalidValue;
		if(start == end) return;
		if(type == ValType::int32 || type == ValType::int64 || type == ValType::uint32) goto MarkInvalidValue;

		if(doc[start] == '.') {
			if(!readInt(++start, end, doc)) goto MarkInvalidValue;
			if(start == end) return;
		}

		if(isExponent(doc[start])) {
			if(++start == end) goto MarkInvalidValue;
			if(isSign(doc[start])) ++start;
			if(!readInt(start, end, doc)) goto MarkInvalidValue;
		}

		if(start == end) return;

	MarkInvalidValue:
		doc.IndicatorFill(old, end, IndicatorStyle::error, 1);
		return;
	}
	
MarkDefaultLine:
	doc.ColourTo(end - 1, TextStyle::default_);
}

unsigned int findTagStart(int line, sptr_t scintilla, SciFnDirect message) {
	unsigned int begin = message(scintilla, SCI_POSITIONFROMLINE, line, 0);
	while(message(scintilla, SCI_GETCHARAT, begin, 0) != '[') ++begin;
	return begin;
}

unsigned int findTagEnd(int line, sptr_t scintilla, SciFnDirect message) {
	unsigned int rbegin = message(scintilla, SCI_GETLINEENDPOSITION, line, 0);
	while(message(scintilla, SCI_GETCHARAT, --rbegin, 0) != ']');
	return rbegin + 1;
}

}

void SCI_METHOD TlDatLexer::Lex(unsigned int start, int length, int, IDocument* pDoc) {
	if(!doneOnce_)
		length = pDoc->Length();

	try {
		LexAccessor doc(pDoc);
		doc.StartAt(start);
		doc.StartSegment(start);

		pDoc->DecorationSetCurrentIndicator(IndicatorStyle::error);

		for(unsigned int end = start + static_cast<unsigned int>(length);;) {
			unsigned int eol = findEol(start, doc);
			int line = doc.GetLine(start);
			doc.SetLineState(line, TextStyle::default_);

			if(start != eol) {
				pDoc->DecorationFillRange(start, 0, end - start);
				LexLine(start, eol, line, doc);
			}
			
			if(eol == end) break;
			if(doc[eol] == '\r' && eol + 1 != end && doc[eol + 1] == '\n')
				++eol;
			doc.ColourTo(eol, TextStyle::default_);
			if(++eol == end) break;
			start = eol;
		}
		
		doc.Flush();
	} catch (...) {
		pDoc->SetErrorStatus(SC_STATUS_FAILURE);
	}
}

void SCI_METHOD TlDatLexer::Fold(unsigned int start, int length, int, IDocument* doc) {
	if(!doneOnce_) {
		doneOnce_ = true;
		length = doc->Length();
	}

	int line = doc->LineFromPosition(start);
	int end = doc->LineFromPosition(start + length);
	int level = SC_FOLDLEVELBASE;

	if(line > 0) {
		level = doc->GetLevel(line - 1);
		if(doc->GetLineState(line - 1) == TextStyle::openTag)
			level = (level & ~SC_FOLDLEVELHEADERFLAG) + 1;
	}

	for(; line <= end; ++line) {
		int state = doc->GetLineState(line);
		if(state == TextStyle::closeTag && level > SC_FOLDLEVELBASE)
			doc->SetLevel(line, --level);
		else if(state == TextStyle::openTag) {
			doc->SetLevel(line, level | SC_FOLDLEVELHEADERFLAG);
			level++;
		} else
			doc->SetLevel(line, level);
	}
}

void matchTags(unsigned int pos, sptr_t scintilla, SciFnDirect message) {
	int curLine = message(scintilla, SCI_LINEFROMPOSITION, pos, 0);
	int state = message(scintilla, SCI_GETLINESTATE, curLine, 0);
	if(state == 0) return;

	unsigned int curBegin = findTagStart(curLine, scintilla, message);
	unsigned int curEnd = findTagEnd(curLine, scintilla, message);
	if(pos < curBegin || pos > curEnd) return;

	int opLine = curLine;

	int level = 1;
	if(state == TextStyle::openTag) {
		int end = message(scintilla, SCI_GETLINECOUNT, 0, 0);
		do {
			if(++opLine == end) {
				message(scintilla, SCI_BRACEBADLIGHT, curBegin, 0);
				return;
			}

			switch(message(scintilla, SCI_GETLINESTATE, opLine, 0)) {
			case TextStyle::openTag: ++level; break;
			case TextStyle::closeTag: --level; break;
			}
		} while(level != 0);
	} else {
		int end = -1;
		do {
			if(--opLine == end) {
				message(scintilla, SCI_BRACEBADLIGHT, curEnd - 1, 0);
				return;
			}

			switch(message(scintilla, SCI_GETLINESTATE, opLine, 0)) {
			case TextStyle::openTag: --level; break;
			case TextStyle::closeTag: ++level; break;
			}
		} while(level != 0);
	}

	unsigned int opBegin = findTagStart(opLine, scintilla, message);
	unsigned int opEnd = findTagEnd(opLine, scintilla, message);

	message(scintilla, SCI_SETINDICATORCURRENT, SCE_UNIVERSAL_TAGMATCH, 0);
	message(scintilla, SCI_INDICATORFILLRANGE, curBegin, curEnd - curBegin);
	message(scintilla, SCI_INDICATORFILLRANGE, opBegin, opEnd - opBegin);

	if(state == TextStyle::openTag)
		message(scintilla, SCI_BRACEHIGHLIGHT, curBegin, opEnd - 1);
	else
		message(scintilla, SCI_BRACEHIGHLIGHT, opBegin, curEnd - 1);

	int curColumn = message(scintilla, SCI_GETCOLUMN, curBegin, 0);
	int opColumn = message(scintilla, SCI_GETCOLUMN, opBegin, 0);
	message(scintilla, SCI_SETHIGHLIGHTGUIDE, curColumn < opColumn? curColumn: opColumn, 0);
}
