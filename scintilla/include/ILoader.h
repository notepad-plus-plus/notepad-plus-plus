// Scintilla source code edit control
/** @file ILoader.h
 ** Interface for loading into a Scintilla document from a background thread.
 ** Interface for manipulating a document without a view.
 **/
// Copyright 1998-2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ILOADER_H
#define ILOADER_H

#include "Sci_Position.h"

namespace Scintilla {

class ILoader {
public:
	virtual int SCI_METHOD Release() = 0;
	// Returns a status code from SC_STATUS_*
	virtual int SCI_METHOD AddData(const char *data, Sci_Position length) = 0;
	virtual void * SCI_METHOD ConvertToDocument() = 0;
};

static constexpr int deRelease0 = 0;

class IDocumentEditable {
public:
	// Allow this interface to add methods over time and discover whether new methods available.
	virtual int SCI_METHOD DEVersion() const noexcept = 0;

	// Lifetime control
	virtual int SCI_METHOD AddRef() noexcept = 0;
	virtual int SCI_METHOD Release() = 0;
};

}

#endif
