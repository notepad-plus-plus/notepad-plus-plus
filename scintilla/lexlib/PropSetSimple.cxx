// Scintilla source code edit control
/** @file PropSetSimple.cxx
 ** A basic string to string map.
 **/
// Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Maintain a dictionary of properties

#include <cstdlib>
#include <cstring>

#include <string>
#include <map>

#include "PropSetSimple.h"

using namespace Scintilla;

namespace {

typedef std::map<std::string, std::string> mapss;

mapss *PropsFromPointer(void *impl) {
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
	impl = 0;
}

void PropSetSimple::Set(const char *key, const char *val, size_t lenKey, size_t lenVal) {
	mapss *props = PropsFromPointer(impl);
	if (!*key)	// Empty keys are not supported
		return;
	(*props)[std::string(key, lenKey)] = std::string(val, lenVal);
}

static bool IsASpaceCharacter(unsigned int ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

void PropSetSimple::Set(const char *keyVal) {
	while (IsASpaceCharacter(*keyVal))
		keyVal++;
	const char *endVal = keyVal;
	while (*endVal && (*endVal != '\n'))
		endVal++;
	const char *eqAt = strchr(keyVal, '=');
	if (eqAt) {
		Set(keyVal, eqAt + 1, eqAt-keyVal,
			endVal - eqAt - 1);
	} else if (*keyVal) {	// No '=' so assume '=1'
		Set(keyVal, "1", endVal-keyVal, 1);
	}
}

void PropSetSimple::SetMultiple(const char *s) {
	const char *eol = strchr(s, '\n');
	while (eol) {
		Set(s);
		s = eol + 1;
		eol = strchr(s, '\n');
	}
	Set(s);
}

const char *PropSetSimple::Get(const char *key) const {
	mapss *props = PropsFromPointer(impl);
	mapss::const_iterator keyPos = props->find(std::string(key));
	if (keyPos != props->end()) {
		return keyPos->second.c_str();
	} else {
		return "";
	}
}

// There is some inconsistency between GetExpanded("foo") and Expand("$(foo)").
// A solution is to keep a stack of variables that have been expanded, so that
// recursive expansions can be skipped.  For now I'll just use the C++ stack
// for that, through a recursive function and a simple chain of pointers.

struct VarChain {
	VarChain(const char *var_=nullptr, const VarChain *link_= nullptr): var(var_), link(link_) {}

	bool contains(const char *testVar) const {
		return (var && (0 == strcmp(var, testVar)))
			|| (link && link->contains(testVar));
	}

	const char *var;
	const VarChain *link;
};

static int ExpandAllInPlace(const PropSetSimple &props, std::string &withVars, int maxExpands, const VarChain &blankVars) {
	size_t varStart = withVars.find("$(");
	while ((varStart != std::string::npos) && (maxExpands > 0)) {
		const size_t varEnd = withVars.find(')', varStart+2);
		if (varEnd == std::string::npos) {
			break;
		}

		// For consistency, when we see '$(ab$(cde))', expand the inner variable first,
		// regardless whether there is actually a degenerate variable named 'ab$(cde'.
		size_t innerVarStart = withVars.find("$(", varStart+2);
		while ((innerVarStart != std::string::npos) && (innerVarStart > varStart) && (innerVarStart < varEnd)) {
			varStart = innerVarStart;
			innerVarStart = withVars.find("$(", varStart+2);
		}

		std::string var(withVars, varStart + 2, varEnd - varStart - 2);
		std::string val = props.Get(var.c_str());

		if (blankVars.contains(var.c_str())) {
			val = ""; // treat blankVar as an empty string (e.g. to block self-reference)
		}

		if (--maxExpands >= 0) {
			maxExpands = ExpandAllInPlace(props, val, maxExpands, VarChain(var.c_str(), &blankVars));
		}

		withVars.erase(varStart, varEnd-varStart+1);
		withVars.insert(varStart, val.c_str(), val.length());

		varStart = withVars.find("$(");
	}

	return maxExpands;
}

int PropSetSimple::GetExpanded(const char *key, char *result) const {
	std::string val = Get(key);
	ExpandAllInPlace(*this, val, 100, VarChain(key));
	const int n = static_cast<int>(val.size());
	if (result) {
		memcpy(result, val.c_str(), n+1);
	}
	return n;	// Not including NUL
}

int PropSetSimple::GetInt(const char *key, int defaultValue) const {
	std::string val = Get(key);
	ExpandAllInPlace(*this, val, 100, VarChain(key));
	if (!val.empty()) {
		return atoi(val.c_str());
	}
	return defaultValue;
}
