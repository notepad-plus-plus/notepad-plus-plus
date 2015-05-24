// Scintilla source code edit control
/** @file KeyMap.h
 ** Defines a mapping between keystrokes and commands.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef KEYTOCOMMAND_H
#define KEYTOCOMMAND_H

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

#define SCI_NORM 0
#define SCI_SHIFT SCMOD_SHIFT
#define SCI_CTRL SCMOD_CTRL
#define SCI_ALT SCMOD_ALT
#define SCI_META SCMOD_META
#define SCI_CSHIFT (SCI_CTRL | SCI_SHIFT)
#define SCI_ASHIFT (SCI_ALT | SCI_SHIFT)

/**
 */
class KeyToCommand {
public:
	int key;
	int modifiers;
	unsigned int msg;
};

/**
 */
class KeyMap {
	std::vector<KeyToCommand> kmap;
	static const KeyToCommand MapDefault[];

public:
	KeyMap();
	~KeyMap();
	void Clear();
	void AssignCmdKey(int key, int modifiers, unsigned int msg);
	unsigned int Find(int key, int modifiers) const;	// 0 returned on failure
};

#ifdef SCI_NAMESPACE
}
#endif

#endif
