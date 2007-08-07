// SciTE - Scintilla based Text Editor
/** @file PropSet.cxx
 ** A Java style properties file module.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// Maintain a dictionary of properties

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Platform.h"

#include "PropSet.h"

// The comparison and case changing functions here assume ASCII
// or extended ASCII such as the normal Windows code page.

static inline char MakeUpperCase(char ch) {
	if (ch < 'a' || ch > 'z')
		return ch;
	else
		return static_cast<char>(ch - 'a' + 'A');
}

static inline bool IsLetter(char ch) {
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

inline bool IsASpace(unsigned int ch) {
    return (ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d));
}

int CompareCaseInsensitive(const char *a, const char *b) {
	while (*a && *b) {
		if (*a != *b) {
			char upperA = MakeUpperCase(*a);
			char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
	}
	// Either *a or *b is nul
	return *a - *b;
}

int CompareNCaseInsensitive(const char *a, const char *b, size_t len) {
	while (*a && *b && len) {
		if (*a != *b) {
			char upperA = MakeUpperCase(*a);
			char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
		len--;
	}
	if (len == 0)
		return 0;
	else
		// Either *a or *b is nul
		return *a - *b;
}

bool EqualCaseInsensitive(const char *a, const char *b) {
	return 0 == CompareCaseInsensitive(a, b);
}

// Since the CaseInsensitive functions declared in SString
// are implemented here, I will for now put the non-inline
// implementations of the SString members here as well, so
// that I can quickly see what effect this has.

SString::SString(int i) : sizeGrowth(sizeGrowthDefault) {
	char number[32];
	sprintf(number, "%0d", i);
	s = StringAllocate(number);
	sSize = sLen = (s) ? strlen(s) : 0;
}

SString::SString(double d, int precision) : sizeGrowth(sizeGrowthDefault) {
	char number[32];
	sprintf(number, "%.*f", precision, d);
	s = StringAllocate(number);
	sSize = sLen = (s) ? strlen(s) : 0;
}

bool SString::grow(lenpos_t lenNew) {
	while (sizeGrowth * 6 < lenNew) {
		sizeGrowth *= 2;
	}
	char *sNew = new char[lenNew + sizeGrowth + 1];
	if (sNew) {
		if (s) {
			memcpy(sNew, s, sLen);
			delete []s;
		}
		s = sNew;
		s[sLen] = '\0';
		sSize = lenNew + sizeGrowth;
	}
	return sNew != 0;
}

SString &SString::assign(const char *sOther, lenpos_t sSize_) {
	if (!sOther) {
		sSize_ = 0;
	} else if (sSize_ == measure_length) {
		sSize_ = strlen(sOther);
	}
	if (sSize > 0 && sSize_ <= sSize) {	// Does not allocate new buffer if the current is big enough
		if (s && sSize_) {
			memcpy(s, sOther, sSize_);
		}
		s[sSize_] = '\0';
		sLen = sSize_;
	} else {
		delete []s;
		s = StringAllocate(sOther, sSize_);
		if (s) {
			sSize = sSize_;	// Allow buffer bigger than real string, thus providing space to grow
			sLen = sSize_;
		} else {
			sSize = sLen = 0;
		}
	}
	return *this;
}

bool SString::operator==(const SString &sOther) const {
	if ((s == 0) && (sOther.s == 0))
		return true;
	if ((s == 0) || (sOther.s == 0))
		return false;
	return strcmp(s, sOther.s) == 0;
}

bool SString::operator==(const char *sOther) const {
	if ((s == 0) && (sOther == 0))
		return true;
	if ((s == 0) || (sOther == 0))
		return false;
	return strcmp(s, sOther) == 0;
}

SString SString::substr(lenpos_t subPos, lenpos_t subLen) const {
	if (subPos >= sLen) {
		return SString();					// return a null string if start index is out of bounds
	}
	if ((subLen == measure_length) || (subPos + subLen > sLen)) {
		subLen = sLen - subPos;		// can't substr past end of source string
	}
	return SString(s, subPos, subPos + subLen);
}

SString &SString::lowercase(lenpos_t subPos, lenpos_t subLen) {
	if ((subLen == measure_length) || (subPos + subLen > sLen)) {
		subLen = sLen - subPos;		// don't apply past end of string
	}
	for (lenpos_t i = subPos; i < subPos + subLen; i++) {
		if (s[i] < 'A' || s[i] > 'Z')
			continue;
		else
			s[i] = static_cast<char>(s[i] - 'A' + 'a');
	}
	return *this;
}

SString &SString::uppercase(lenpos_t subPos, lenpos_t subLen) {
	if ((subLen == measure_length) || (subPos + subLen > sLen)) {
		subLen = sLen - subPos;		// don't apply past end of string
	}
	for (lenpos_t i = subPos; i < subPos + subLen; i++) {
		if (s[i] < 'a' || s[i] > 'z')
			continue;
		else
			s[i] = static_cast<char>(s[i] - 'a' + 'A');
	}
	return *this;
}

SString &SString::append(const char *sOther, lenpos_t sLenOther, char sep) {
	if (!sOther) {
		return *this;
	}
	if (sLenOther == measure_length) {
		sLenOther = strlen(sOther);
	}
	int lenSep = 0;
	if (sLen && sep) {	// Only add a separator if not empty
		lenSep = 1;
	}
	lenpos_t lenNew = sLen + sLenOther + lenSep;
	// Conservative about growing the buffer: don't do it, unless really needed
	if ((lenNew < sSize) || (grow(lenNew))) {
		if (lenSep) {
			s[sLen] = sep;
			sLen++;
		}
		memcpy(&s[sLen], sOther, sLenOther);
		sLen += sLenOther;
		s[sLen] = '\0';
	}
	return *this;
}

SString &SString::insert(lenpos_t pos, const char *sOther, lenpos_t sLenOther) {
	if (!sOther || pos > sLen) {
		return *this;
	}
	if (sLenOther == measure_length) {
		sLenOther = strlen(sOther);
	}
	lenpos_t lenNew = sLen + sLenOther;
	// Conservative about growing the buffer: don't do it, unless really needed
	if ((lenNew < sSize) || grow(lenNew)) {
		lenpos_t moveChars = sLen - pos + 1;
		for (lenpos_t i = moveChars; i > 0; i--) {
			s[pos + sLenOther + i - 1] = s[pos + i - 1];
		}
		memcpy(s + pos, sOther, sLenOther);
		sLen = lenNew;
	}
	return *this;
}

/**
 * Remove @a len characters from the @a pos position, included.
 * Characters at pos + len and beyond replace characters at pos.
 * If @a len is 0, or greater than the length of the string
 * starting at @a pos, the string is just truncated at @a pos.
 */
void SString::remove(lenpos_t pos, lenpos_t len) {
	if (pos >= sLen) {
		return;
	}
	if (len < 1 || pos + len >= sLen) {
		s[pos] = '\0';
		sLen = pos;
	} else {
		for (lenpos_t i = pos; i < sLen - len + 1; i++) {
			s[i] = s[i+len];
		}
		sLen -= len;
	}
}

bool SString::startswith(const char *prefix) {
	lenpos_t lenPrefix = strlen(prefix);
	if (lenPrefix > sLen) {
		return false;
	}
	return strncmp(s, prefix, lenPrefix) == 0;
}

bool SString::endswith(const char *suffix) {
	lenpos_t lenSuffix = strlen(suffix);
	if (lenSuffix > sLen) {
		return false;
	}
	return strncmp(s + sLen - lenSuffix, suffix, lenSuffix) == 0;
}

int SString::search(const char *sFind, lenpos_t start) const {
	if (start < sLen) {
		const char *sFound = strstr(s + start, sFind);
		if (sFound) {
			return sFound - s;
		}
	}
	return -1;
}

int SString::substitute(char chFind, char chReplace) {
	int c = 0;
	char *t = s;
	while (t) {
		t = strchr(t, chFind);
		if (t) {
			*t = chReplace;
			t++;
			c++;
		}
	}
	return c;
}

int SString::substitute(const char *sFind, const char *sReplace) {
	int c = 0;
	lenpos_t lenFind = strlen(sFind);
	lenpos_t lenReplace = strlen(sReplace);
	int posFound = search(sFind);
	while (posFound >= 0) {
		remove(posFound, lenFind);
		insert(posFound, sReplace, lenReplace);
		posFound = search(sFind, posFound + lenReplace);
		c++;
	}
	return c;
}

char *SContainer::StringAllocate(lenpos_t len) {
	if (len != measure_length) {
		return new char[len + 1];
	} else {
		return 0;
	}
}

char *SContainer::StringAllocate(const char *s, lenpos_t len) {
	if (s == 0) {
		return 0;
	}
	if (len == measure_length) {
		len = strlen(s);
	}
	char *sNew = new char[len + 1];
	if (sNew) {
		memcpy(sNew, s, len);
		sNew[len] = '\0';
	}
	return sNew;
}

// End SString functions

bool PropSet::caseSensitiveFilenames = false;

PropSet::PropSet() {
	superPS = 0;
	for (int root = 0; root < hashRoots; root++)
		props[root] = 0;
}

PropSet::~PropSet() {
	superPS = 0;
	Clear();
}

void PropSet::Set(const char *key, const char *val, int lenKey, int lenVal) {
	if (!*key)	// Empty keys are not supported
		return;
	if (lenKey == -1)
		lenKey = static_cast<int>(strlen(key));
	if (lenVal == -1)
		lenVal = static_cast<int>(strlen(val));
	unsigned int hash = HashString(key, lenKey);
	for (Property *p = props[hash % hashRoots]; p; p = p->next) {
		if ((hash == p->hash) &&
			((strlen(p->key) == static_cast<unsigned int>(lenKey)) &&
				(0 == strncmp(p->key, key, lenKey)))) {
			// Replace current value
			delete [](p->val);
			p->val = StringDup(val, lenVal);
			return;
		}
	}
	// Not found
	Property *pNew = new Property;
	if (pNew) {
		pNew->hash = hash;
		pNew->key = StringDup(key, lenKey);
		pNew->val = StringDup(val, lenVal);
		pNew->next = props[hash % hashRoots];
		props[hash % hashRoots] = pNew;
	}
}

void PropSet::Set(const char *keyVal) {
	while (IsASpace(*keyVal))
		keyVal++;
	const char *endVal = keyVal;
	while (*endVal && (*endVal != '\n'))
		endVal++;
	const char *eqAt = strchr(keyVal, '=');
	if (eqAt) {
		Set(keyVal, eqAt + 1, eqAt-keyVal, endVal - eqAt - 1);
	} else if (*keyVal) {	// No '=' so assume '=1'
		Set(keyVal, "1", endVal-keyVal, 1);
	}
}

void PropSet::Unset(const char *key, int lenKey) {
	if (!*key)	// Empty keys are not supported
		return;
	if (lenKey == -1)
		lenKey = static_cast<int>(strlen(key));
	unsigned int hash = HashString(key, lenKey);
	Property *pPrev = NULL;
	for (Property *p = props[hash % hashRoots]; p; p = p->next) {
		if ((hash == p->hash) &&
			((strlen(p->key) == static_cast<unsigned int>(lenKey)) &&
				(0 == strncmp(p->key, key, lenKey)))) {
			if (pPrev)
				pPrev->next = p->next;
			else
				props[hash % hashRoots] = p->next;
			if (p == enumnext)
				enumnext = p->next; // Not that anyone should mix enum and Set / Unset.
			delete [](p->key);
			delete [](p->val);
			delete p;
			return;
		} else {
			pPrev = p;
		}
	}
}

void PropSet::SetMultiple(const char *s) {
	const char *eol = strchr(s, '\n');
	while (eol) {
		Set(s);
		s = eol + 1;
		eol = strchr(s, '\n');
	}
	Set(s);
}

SString PropSet::Get(const char *key) {
	unsigned int hash = HashString(key, strlen(key));
	for (Property *p = props[hash % hashRoots]; p; p = p->next) {
		if ((hash == p->hash) && (0 == strcmp(p->key, key))) {
			return p->val;
		}
	}
	if (superPS) {
		// Failed here, so try in base property set
		return superPS->Get(key);
	} else {
		return "";
	}
}

bool PropSet::IncludesVar(const char *value, const char *key) {
	const char *var = strstr(value, "$(");
	while (var) {
		if (isprefix(var + 2, key) && (var[2 + strlen(key)] == ')')) {
			// Found $(key) which would lead to an infinite loop so exit
			return true;
		}
		var = strstr(var + 2, ")");
		if (var)
			var = strstr(var + 1, "$(");
	}
	return false;
}


// There is some inconsistency between GetExpanded("foo") and Expand("$(foo)").
// A solution is to keep a stack of variables that have been expanded, so that
// recursive expansions can be skipped.  For now I'll just use the C++ stack
// for that, through a recursive function and a simple chain of pointers.

struct VarChain {
	VarChain(const char*var_=NULL, const VarChain *link_=NULL): var(var_), link(link_) {}

	bool contains(const char *testVar) const {
		return (var && (0 == strcmp(var, testVar)))
			|| (link && link->contains(testVar));
	}

	const char *var;
	const VarChain *link;
};

static int ExpandAllInPlace(PropSet &props, SString &withVars, int maxExpands, const VarChain &blankVars = VarChain()) {
	int varStart = withVars.search("$(");
	while ((varStart >= 0) && (maxExpands > 0)) {
		int varEnd = withVars.search(")", varStart+2);
		if (varEnd < 0) {
			break;
		}

		// For consistency, when we see '$(ab$(cde))', expand the inner variable first,
		// regardless whether there is actually a degenerate variable named 'ab$(cde'.
		int innerVarStart = withVars.search("$(", varStart+2);
		while ((innerVarStart > varStart) && (innerVarStart < varEnd)) {
			varStart = innerVarStart;
			innerVarStart = withVars.search("$(", varStart+2);
		}

		SString var(withVars.c_str(), varStart + 2, varEnd);
		SString val = props.Get(var.c_str());

		if (blankVars.contains(var.c_str())) {
			val.clear(); // treat blankVar as an empty string (e.g. to block self-reference)
		}

		if (--maxExpands >= 0) {
			maxExpands = ExpandAllInPlace(props, val, maxExpands, VarChain(var.c_str(), &blankVars));
		}

		withVars.remove(varStart, varEnd-varStart+1);
		withVars.insert(varStart, val.c_str(), val.length());

		varStart = withVars.search("$(");
	}

	return maxExpands;
}


SString PropSet::GetExpanded(const char *key) {
	SString val = Get(key);
	ExpandAllInPlace(*this, val, 100, VarChain(key));
	return val;
}

SString PropSet::Expand(const char *withVars, int maxExpands) {
	SString val = withVars;
	ExpandAllInPlace(*this, val, maxExpands);
	return val;
}

int PropSet::GetInt(const char *key, int defaultValue) {
	SString val = GetExpanded(key);
	if (val.length())
		return val.value();
	return defaultValue;
}

bool isprefix(const char *target, const char *prefix) {
	while (*target && *prefix) {
		if (*target != *prefix)
			return false;
		target++;
		prefix++;
	}
	if (*prefix)
		return false;
	else
		return true;
}

static bool IsSuffix(const char *target, const char *suffix, bool caseSensitive) {
	size_t lentarget = strlen(target);
	size_t lensuffix = strlen(suffix);
	if (lensuffix > lentarget)
		return false;
	if (caseSensitive) {
		for (int i = static_cast<int>(lensuffix) - 1; i >= 0; i--) {
			if (target[i + lentarget - lensuffix] != suffix[i])
				return false;
		}
	} else {
	for (int i = static_cast<int>(lensuffix) - 1; i >= 0; i--) {
		if (MakeUpperCase(target[i + lentarget - lensuffix]) !=
		        MakeUpperCase(suffix[i]))
			return false;
	}
	}
	return true;
}

SString PropSet::GetWild(const char *keybase, const char *filename) {
	for (int root = 0; root < hashRoots; root++) {
		for (Property *p = props[root]; p; p = p->next) {
			if (isprefix(p->key, keybase)) {
				char * orgkeyfile = p->key + strlen(keybase);
				char *keyfile = NULL;

				if (strstr(orgkeyfile, "$(") == orgkeyfile) {
					char *cpendvar = strchr(orgkeyfile, ')');
					if (cpendvar) {
						*cpendvar = '\0';
						SString s = GetExpanded(orgkeyfile + 2);
						*cpendvar = ')';
						keyfile = StringDup(s.c_str());
					}
				}
				char *keyptr = keyfile;

				if (keyfile == NULL)
					keyfile = orgkeyfile;

				for (;;) {
					char *del = strchr(keyfile, ';');
					if (del == NULL)
						del = keyfile + strlen(keyfile);
					char delchr = *del;
					*del = '\0';
					if (*keyfile == '*') {
						if (IsSuffix(filename, keyfile + 1, caseSensitiveFilenames)) {
							*del = delchr;
							delete []keyptr;
							return p->val;
						}
					} else if (0 == strcmp(keyfile, filename)) {
						*del = delchr;
						delete []keyptr;
						return p->val;
					}
					if (delchr == '\0')
						break;
					*del = delchr;
					keyfile = del + 1;
				}
				delete []keyptr;

				if (0 == strcmp(p->key, keybase)) {
					return p->val;
				}
			}
		}
	}
	if (superPS) {
		// Failed here, so try in base property set
		return superPS->GetWild(keybase, filename);
	} else {
		return "";
	}
}



// GetNewExpand does not use Expand as it has to use GetWild with the filename for each
// variable reference found.
SString PropSet::GetNewExpand(const char *keybase, const char *filename) {
	char *base = StringDup(GetWild(keybase, filename).c_str());
	char *cpvar = strstr(base, "$(");
	int maxExpands = 1000;	// Avoid infinite expansion of recursive definitions
	while (cpvar && (maxExpands > 0)) {
		char *cpendvar = strchr(cpvar, ')');
		if (cpendvar) {
			int lenvar = cpendvar - cpvar - 2;  	// Subtract the $()
			char *var = StringDup(cpvar + 2, lenvar);
			SString val = GetWild(var, filename);
			if (0 == strcmp(var, keybase))
				val.clear(); // Self-references evaluate to empty string
			size_t newlenbase = strlen(base) + val.length() - lenvar;
			char *newbase = new char[newlenbase];
			strncpy(newbase, base, cpvar - base);
			strcpy(newbase + (cpvar - base), val.c_str());
			strcpy(newbase + (cpvar - base) + val.length(), cpendvar + 1);
			delete []var;
			delete []base;
			base = newbase;
		}
		cpvar = strstr(base, "$(");
		maxExpands--;
	}
	SString sret = base;
	delete []base;
	return sret;
}

void PropSet::Clear() {
	for (int root = 0; root < hashRoots; root++) {
		Property *p = props[root];
		while (p) {
			Property *pNext = p->next;
			p->hash = 0;
			delete []p->key;
			p->key = 0;
			delete []p->val;
			p->val = 0;
			delete p;
			p = pNext;
		}
		props[root] = 0;
	}
}

char *PropSet::ToString() {
	size_t len=0;
	for (int r = 0; r < hashRoots; r++) {
		for (Property *p = props[r]; p; p = p->next) {
			len += strlen(p->key) + 1;
			len += strlen(p->val) + 1;
		}
	}
	if (len == 0)
		len = 1;	// Return as empty string
	char *ret = new char [len];
	if (ret) {
		char *w = ret;
		for (int root = 0; root < hashRoots; root++) {
			for (Property *p = props[root]; p; p = p->next) {
				strcpy(w, p->key);
				w += strlen(p->key);
				*w++ = '=';
				strcpy(w, p->val);
				w += strlen(p->val);
				*w++ = '\n';
			}
		}
		ret[len-1] = '\0';
	}
	return ret;
}

/**
 * Initiate enumeration.
 */
bool PropSet::GetFirst(char **key, char **val) {
	for (int i = 0; i < hashRoots; i++) {
		for (Property *p = props[i]; p; p = p->next) {
			if (p) {
				*key = p->key;
				*val = p->val;
				enumnext = p->next; // GetNext will begin here ...
				enumhash = i;		  // ... in this block
				return true;
			}
		}
	}
	return false;
}

/**
 * Continue enumeration.
 */
bool PropSet::GetNext(char ** key, char ** val) {
	bool firstloop = true;

	// search begins where we left it : in enumhash block
	for (int i = enumhash; i < hashRoots; i++) {
		if (!firstloop)
			enumnext = props[i]; // Begin with first property in block
		// else : begin where we left
		firstloop = false;

		for (Property *p = enumnext; p; p = p->next) {
			if (p) {
				*key = p->key;
				*val = p->val;
				enumnext = p->next; // for GetNext
				enumhash = i;
				return true;
			}
		}
	}
	return false;
}

/**
 * Creates an array that points into each word in the string and puts \0 terminators
 * after each word.
 */
static char **ArrayFromWordList(char *wordlist, int *len, bool onlyLineEnds = false) {
	int prev = '\n';
	int words = 0;
	// For rapid determination of whether a character is a separator, build
	// a look up table.
	bool wordSeparator[256];
	for (int i=0;i<256; i++) {
		wordSeparator[i] = false;
	}
	wordSeparator['\r'] = true;
	wordSeparator['\n'] = true;
	if (!onlyLineEnds) {
		wordSeparator[' '] = true;
		wordSeparator['\t'] = true;
	}
	for (int j = 0; wordlist[j]; j++) {
		int curr = static_cast<unsigned char>(wordlist[j]);
		if (!wordSeparator[curr] && wordSeparator[prev])
			words++;
		prev = curr;
	}
	char **keywords = new char *[words + 1];
	if (keywords) {
		words = 0;
		prev = '\0';
		size_t slen = strlen(wordlist);
		for (size_t k = 0; k < slen; k++) {
			if (!wordSeparator[static_cast<unsigned char>(wordlist[k])]) {
				if (!prev) {
					keywords[words] = &wordlist[k];
					words++;
				}
			} else {
				wordlist[k] = '\0';
			}
			prev = wordlist[k];
		}
		keywords[words] = &wordlist[slen];
		*len = words;
	} else {
		*len = 0;
	}
	return keywords;
}

void WordList::Clear() {
	if (words) {
		delete []list;
		delete []words;
		delete []wordsNoCase;
	}
	words = 0;
	wordsNoCase = 0;
	list = 0;
	len = 0;
	sorted = false;
	sortedNoCase = false;
}

void WordList::Set(const char *s) {
	list = StringDup(s);
	sorted = false;
	sortedNoCase = false;
	words = ArrayFromWordList(list, &len, onlyLineEnds);
	wordsNoCase = new char * [len + 1];
	memcpy(wordsNoCase, words, (len + 1) * sizeof (*words));
}

char *WordList::Allocate(int size) {
	list = new char[size + 1];
	list[size] = '\0';
	return list;
}

void WordList::SetFromAllocated() {
	sorted = false;
	sortedNoCase = false;
	words = ArrayFromWordList(list, &len, onlyLineEnds);
	wordsNoCase = new char * [len + 1];
	memcpy(wordsNoCase, words, (len + 1) * sizeof (*words));
}

extern "C" int cmpString(const void *a1, const void *a2) {
	// Can't work out the correct incantation to use modern casts here
	return strcmp(*(char**)(a1), *(char**)(a2));
}

extern "C" int cmpStringNoCase(const void *a1, const void *a2) {
	// Can't work out the correct incantation to use modern casts here
	return CompareCaseInsensitive(*(char**)(a1), *(char**)(a2));
}

static void SortWordList(char **words, unsigned int len) {
	qsort(reinterpret_cast<void*>(words), len, sizeof(*words),
	      cmpString);
}

static void SortWordListNoCase(char **wordsNoCase, unsigned int len) {
	qsort(reinterpret_cast<void*>(wordsNoCase), len, sizeof(*wordsNoCase),
	      cmpStringNoCase);
}

bool WordList::InList(const char *s) {
	if (0 == words)
		return false;
	if (!sorted) {
		sorted = true;
		SortWordList(words, len);
		for (unsigned int k = 0; k < (sizeof(starts) / sizeof(starts[0])); k++)
			starts[k] = -1;
		for (int l = len - 1; l >= 0; l--) {
			unsigned char indexChar = words[l][0];
			starts[indexChar] = l;
		}
	}
	unsigned char firstChar = s[0];
	int j = starts[firstChar];
	if (j >= 0) {
		while (words[j][0] == firstChar) {
			if (s[1] == words[j][1]) {
				const char *a = words[j] + 1;
				const char *b = s + 1;
				while (*a && *a == *b) {
					a++;
					b++;
				}
				if (!*a && !*b)
					return true;
			}
			j++;
		}
	}
	j = starts['^'];
	if (j >= 0) {
		while (words[j][0] == '^') {
			const char *a = words[j] + 1;
			const char *b = s;
			while (*a && *a == *b) {
				a++;
				b++;
			}
			if (!*a)
				return true;
			j++;
		}
	}
	return false;
}

/** similar to InList, but word s can be a substring of keyword.
 * eg. the keyword define is defined as def~ine. This means the word must start
 * with def to be a keyword, but also defi, defin and define are valid.
 * The marker is ~ in this case.
 */
bool WordList::InListAbbreviated(const char *s, const char marker) {
	if (0 == words)
		return false;
	if (!sorted) {
		sorted = true;
		SortWordList(words, len);
		for (unsigned int k = 0; k < (sizeof(starts) / sizeof(starts[0])); k++)
			starts[k] = -1;
		for (int l = len - 1; l >= 0; l--) {
			unsigned char indexChar = words[l][0];
			starts[indexChar] = l;
		}
	}
	unsigned char firstChar = s[0];
	int j = starts[firstChar];
	if (j >= 0) {
		while (words[j][0] == firstChar) {
			bool isSubword = false;
			int start = 1;
			if (words[j][1] == marker) {
				isSubword = true;
				start++;
			}
			if (s[1] == words[j][start]) {
				const char *a = words[j] + start;
				const char *b = s + 1;
				while (*a && *a == *b) {
					a++;
					if (*a == marker) {
						isSubword = true;
						a++;
					}
					b++;
				}
				if ((!*a || isSubword) && !*b)
					return true;
			}
			j++;
		}
	}
	j = starts['^'];
	if (j >= 0) {
		while (words[j][0] == '^') {
			const char *a = words[j] + 1;
			const char *b = s;
			while (*a && *a == *b) {
				a++;
				b++;
			}
			if (!*a)
				return true;
			j++;
		}
	}
	return false;
}

/**
 * Returns an element (complete) of the wordlist array which has
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved (default).
 */
const char *WordList::GetNearestWord(const char *wordStart, int searchLen, bool ignoreCase /*= false*/, SString wordCharacters /*='/0' */, int wordIndex /*= -1 */) {
	int start = 0; // lower bound of the api array block to search
	int end = len - 1; // upper bound of the api array block to search
	int pivot; // index of api array element just being compared
	int cond; // comparison result (in the sense of strcmp() result)
	const char *word; // api array element just being compared

	if (0 == words)
		return NULL;
	if (ignoreCase) {
		if (!sortedNoCase) {
			sortedNoCase = true;
			SortWordListNoCase(wordsNoCase, len);
		}
		while (start <= end) { // binary searching loop
			pivot = (start + end) >> 1;
			word = wordsNoCase[pivot];
			cond = CompareNCaseInsensitive(wordStart, word, searchLen);
			if (!cond) {
				// find first word
				start = pivot;
				while (start > 0 && !CompareNCaseInsensitive(wordStart, wordsNoCase[start-1], searchLen)) {
					start--;
				}
				// find last word
				end = pivot;
				while (end < len-1 && !CompareNCaseInsensitive(wordStart, wordsNoCase[end+1], searchLen)) {
					end++;
				}

				// Finds first word in a series of equal words
				for (pivot = start; pivot <= end; pivot++) {
					word = wordsNoCase[pivot];
					if (!wordCharacters.contains(word[searchLen])) {
						if (wordIndex <= 0) // Checks if a specific index was requested
							return word; // result must not be freed with free()
						wordIndex--;
					}
				}
				return NULL;
			}
			else if (cond > 0)
				start = pivot + 1;
			else if (cond < 0)
				end = pivot - 1;
		}
	} else { // preserve the letter case
		if (!sorted) {
			sorted = true;
			SortWordList(words, len);
		}
		while (start <= end) { // binary searching loop
			pivot = (start + end) >> 1;
			word = words[pivot];
			cond = strncmp(wordStart, word, searchLen);
			if (!cond) {
				// find first word
				start = pivot;
				while (start > 0 && !strncmp(wordStart, words[start-1], searchLen)) {
					start--;
				}
				// find last word
				end = pivot;
				while (end < len-1 && !strncmp(wordStart, words[end+1], searchLen)) {
					end++;
				}

				// Finds first word in a series of equal words
				pivot = start;
				while (pivot <= end) {
					word = words[pivot];
					if (!wordCharacters.contains(word[searchLen])) {
						if (wordIndex <= 0) // Checks if a specific index was requested
							return word; // result must not be freed with free()
						wordIndex--;
					}
					pivot++;
				}
				return NULL;
			}
			else if (cond > 0)
				start = pivot + 1;
			else if (cond < 0)
				end = pivot - 1;
		}
	}
	return NULL;
}

/**
 * Find the length of a 'word' which is actually an identifier in a string
 * which looks like "identifier(..." or "identifier" and where
 * there may be extra spaces after the identifier that should not be
 * counted in the length.
 */
static unsigned int LengthWord(const char *word, char otherSeparator) {
	const char *endWord = 0;
	// Find an otherSeparator
	if (otherSeparator)
		endWord = strchr(word, otherSeparator);
	// Find a '('. If that fails go to the end of the string.
	if (!endWord)
		endWord = strchr(word, '(');
	if (!endWord)
		endWord = word + strlen(word);
	// Last case always succeeds so endWord != 0

	// Drop any space characters.
	if (endWord > word) {
		endWord--;	// Back from the '(', otherSeparator, or '\0'
		// Move backwards over any spaces
		while ((endWord > word) && (IsASpace(*endWord))) {
			endWord--;
		}
	}
	return endWord - word;
}

/**
 * Returns elements (first words of them) of the wordlist array which have
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved (default).
 * If there are more words meeting the condition they are returned all of
 * them in the ascending order separated with spaces.
 *
 * NOTE: returned buffer has to be freed with delete[].
 */
char *WordList::GetNearestWords(
    const char *wordStart,
    int searchLen,
    bool ignoreCase /*= false*/,
    char otherSeparator /*= '\0'*/,
    bool exactLen /*=false*/) {
	unsigned int wordlen; // length of the word part (before the '(' brace) of the api array element
	SString wordsNear;
	wordsNear.setsizegrowth(1000);
	int start = 0; // lower bound of the api array block to search
	int end = len - 1; // upper bound of the api array block to search
	int pivot; // index of api array element just being compared
	int cond; // comparison result (in the sense of strcmp() result)

	if (0 == words)
		return NULL;
	if (ignoreCase) {
		if (!sortedNoCase) {
			sortedNoCase = true;
			SortWordListNoCase(wordsNoCase, len);
		}
		while (start <= end) { // Binary searching loop
			pivot = (start + end) / 2;
			cond = CompareNCaseInsensitive(wordStart, wordsNoCase[pivot], searchLen);
			if (!cond) {
				// Find first match
				while ((pivot > start) &&
					(0 == CompareNCaseInsensitive(wordStart,
						wordsNoCase[pivot-1], searchLen))) {
					--pivot;
				}
				// Grab each match
				while ((pivot <= end) &&
					(0 == CompareNCaseInsensitive(wordStart,
						wordsNoCase[pivot], searchLen))) {
					wordlen = LengthWord(wordsNoCase[pivot], otherSeparator) + 1;
					++pivot;
					if (exactLen && wordlen != LengthWord(wordStart, otherSeparator) + 1)
						continue;
					wordsNear.append(wordsNoCase[pivot-1], wordlen, ' ');
				}
				return wordsNear.detach();
			} else if (cond < 0) {
				end = pivot - 1;
			} else if (cond > 0) {
				start = pivot + 1;
			}
		}
	} else {	// Preserve the letter case
		if (!sorted) {
			sorted = true;
			SortWordList(words, len);
		}
		while (start <= end) { // Binary searching loop
			pivot = (start + end) / 2;
			cond = strncmp(wordStart, words[pivot], searchLen);
			if (!cond) {
				// Find first match
				while ((pivot > start) &&
					(0 == strncmp(wordStart,
						words[pivot-1], searchLen))) {
					--pivot;
				}
				// Grab each match
				while ((pivot <= end) &&
					(0 == strncmp(wordStart,
						words[pivot], searchLen))) {
					wordlen = LengthWord(words[pivot], otherSeparator) + 1;
					++pivot;
					if (exactLen && wordlen != LengthWord(wordStart, otherSeparator) + 1)
						continue;
					wordsNear.append(words[pivot-1], wordlen, ' ');
				}
				return wordsNear.detach();
			} else if (cond < 0) {
				end = pivot - 1;
			} else if (cond > 0) {
				start = pivot + 1;
			}
		}
	}
	return NULL;
}
