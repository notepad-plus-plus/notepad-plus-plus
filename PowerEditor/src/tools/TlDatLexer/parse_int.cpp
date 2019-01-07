#include "stdafx.h"
#include "parse_int.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

#undef max

namespace {

bool isDigit(char c) {
	return c >= '0' && c <= '9';
}

template<typename T>
ParseError isValidSigned(char* begin, char* end) {
	typedef typename std::make_unsigned<T>::type U;
	assert(begin <= end);
	if(begin == end) return ParseError::badFormat;

	int negate = 0;
	if(*begin == '-') {
		negate = 1;
		++begin;
	} else if(*begin == '+')
		++begin;

	U val = 0;
	for(; begin != end; ++begin) {
		if(!isDigit(*begin)) return ParseError::badFormat;
		if(val > static_cast<U>(std::numeric_limits<T>::max()) / 10) return ParseError::outOfRange;
		val *= 10;
		if(val + *begin - '0' > static_cast<U>(std::numeric_limits<T>::max()) + negate) return ParseError::outOfRange;
		val += *begin - '0';
	}

	return ParseError::none;
}

template<typename T>
ParseError isValidUnsigned(char* begin, char* end) {
	assert(begin <= end);
	if(begin == end) return ParseError::badFormat;

	T val = 0;
	for(; begin != end; ++begin) {
		if(!isDigit(*begin)) return ParseError::badFormat;
		if(val > std::numeric_limits<T>::max() / 10) return ParseError::outOfRange;
		val *= 10;
		if(val + *begin - '0' < val) return ParseError::outOfRange;
		val += *begin - '0';
	}

	return ParseError::none;
}

}

ParseError isValidInt32(char* begin, char* end) {
	return isValidSigned<std::int32_t>(begin, end);
}

ParseError isValidInt64(char* begin, char* end) {
	return isValidSigned<std::int64_t>(begin, end);
}

ParseError isValidUInt32(char* begin, char* end) {
	return isValidUnsigned<std::uint32_t>(begin, end);
}
