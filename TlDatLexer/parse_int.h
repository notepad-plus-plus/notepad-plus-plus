#ifndef TLDATLEXER_PARSE_INT_H
#define TLDATLEXER_PARSE_INT_H

enum class ParseError {
	none,
	outOfRange,
	badFormat
};

ParseError isValidInt32(char* begin, char* end);
ParseError isValidInt64(char* begin, char* end);
ParseError isValidUInt32(char* begin, char* end);

#endif
