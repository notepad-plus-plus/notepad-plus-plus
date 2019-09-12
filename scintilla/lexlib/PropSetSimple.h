// Scintilla source code edit control
/** @file PropSetSimple.h
 ** A basic string to string map.
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PROPSETSIMPLE_H
#define PROPSETSIMPLE_H

namespace Scintilla {

class PropSetSimple {
	void *impl;
	void Set(const char *keyVal);
public:
	PropSetSimple();
	virtual ~PropSetSimple();
	void Set(const char *key, const char *val, size_t lenKey, size_t lenVal);
	void SetMultiple(const char *);
	const char *Get(const char *key) const;
	int GetExpanded(const char *key, char *result) const;
	int GetInt(const char *key, int defaultValue=0) const;
};

}

#endif
