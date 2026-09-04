// Scintilla source code edit control
/** @file InList.cxx
 ** Check if a string is in a list.
 **/
// Copyright 2024 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>

#include <string>
#include <string_view>
#include <initializer_list>

#include "InList.h"
#include "CharacterSet.h"

namespace Lexilla {

bool InList(std::string_view value, std::initializer_list<std::string_view> list) noexcept {
	for (const std::string_view element : list) {
		if (value == element) {
			return true;
		}
	}
	return false;
}

bool InListCaseInsensitive(std::string_view value, std::initializer_list<std::string_view> list) noexcept {
	for (const std::string_view element : list) {
		if (EqualCaseInsensitive(value, element)) {
			return true;
		}
	}
	return false;
}

}
