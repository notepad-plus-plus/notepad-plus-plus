// Scintilla source code edit control
/** @file PropSet.h
 ** A Java style properties file module.
 **/
// Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PROPSET_H
#define PROPSET_H
#include "SString.h"

bool EqualCaseInsensitive(const char *a, const char *b);

bool isprefix(const char *target, const char *prefix);

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

struct Property {
	unsigned int hash;
	char *key;
	char *val;
	Property *next;
	Property() : hash(0), key(0), val(0), next(0) {}
};

/**
 */
class PropSet {
protected:
	enum { hashRoots=31 };
	Property *props[hashRoots];
	Property *enumnext;
	int enumhash;
	static unsigned int HashString(const char *s, size_t len) {
		unsigned int ret = 0;
		while (len--) {
			ret <<= 4;
			ret ^= *s;
			s++;
		}
		return ret;
	}

public:
	PropSet *superPS;
	PropSet();
	~PropSet();
	void Set(const char *key, const char *val, int lenKey=-1, int lenVal=-1);
	void Set(const char *keyVal);
	void Unset(const char *key, int lenKey=-1);
	void SetMultiple(const char *s);
	SString Get(const char *key) const;
	SString GetExpanded(const char *key) const;
	SString Expand(const char *withVars, int maxExpands=100) const;
	int GetInt(const char *key, int defaultValue=0) const;
	void Clear();
	char *ToString() const;	// Caller must delete[] the return value

private:
	// copy-value semantics not implemented
	PropSet(const PropSet &copy);
	void operator=(const PropSet &assign);
};

/**
 */
class WordList {
public:
	// Each word contains at least one character - a empty word acts as sentinel at the end.
	char **words;
	char *list;
	int len;
	bool onlyLineEnds;	///< Delimited by any white space or only line ends
	bool sorted;
	int starts[256];
	WordList(bool onlyLineEnds_ = false) :
		words(0), list(0), len(0), onlyLineEnds(onlyLineEnds_),
		sorted(false)
		{}
	~WordList() { Clear(); }
	operator bool() { return len ? true : false; }
	void Clear();
	void Set(const char *s);
	bool InList(const char *s);
	bool InListAbbreviated(const char *s, const char marker);
};

inline bool IsAlphabetic(unsigned int ch) {
	return ((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'));
}

#ifdef SCI_NAMESPACE
}
#endif

#ifdef _MSC_VER
// Visual C++ doesn't like the private copy idiom for disabling
// the default copy constructor and operator=, but it's fine.
#pragma warning(disable: 4511 4512)
#endif

#endif
