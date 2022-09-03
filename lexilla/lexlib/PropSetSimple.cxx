// Scintilla source code edit control
/** @file PropSetSimple.cxx
 ** A basic string to string map.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Maintain a dictionary of properties

#include <cstdlib>
#include <cassert>
#include <cstring>

#include <string>
#include <string_view>
#include <map>
#include <functional>

#include "PropSetSimple.h"

using namespace Lexilla;

namespace {

typedef std::map<std::string, std::string, std::less<>> mapss;

mapss *PropsFromPointer(void *impl) noexcept {
	return static_cast<mapss *>(impl);
}

}

PropSetSimple::PropSetSimple() {
	mapss *props = new mapss;
	impl = static_cast<void *>(props);
}

PropSetSimple::~PropSetSimple() {
	mapss *props = PropsFromPointer(impl);
	delete props;
	impl = nullptr;
}

bool PropSetSimple::Set(std::string_view key, std::string_view val) {
	mapss *props = PropsFromPointer(impl);
	if (!props)
		return false;
	mapss::iterator it = props->find(key);
	if (it != props->end()) {
		if (val == it->second)
			return false;
		it->second = val;
	} else {
		props->emplace(key, val);
	}
	return true;
}

const char *PropSetSimple::Get(std::string_view key) const {
	mapss *props = PropsFromPointer(impl);
	if (props) {
		mapss::const_iterator keyPos = props->find(key);
		if (keyPos != props->end()) {
			return keyPos->second.c_str();
		}
	}
	return "";
}

int PropSetSimple::GetInt(std::string_view key, int defaultValue) const {
	const char *val = Get(key);
	assert(val);
	if (*val) {
		return atoi(val);
	}
	return defaultValue;
}
