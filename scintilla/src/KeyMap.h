// Scintilla source code edit control
/** @file KeyMap.h
 ** Defines a mapping between keystrokes and commands.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef KEYMAP_H
#define KEYMAP_H

namespace Scintilla {

#define SCI_NORM 0
#define SCI_SHIFT SCMOD_SHIFT
#define SCI_CTRL SCMOD_CTRL
#define SCI_ALT SCMOD_ALT
#define SCI_META SCMOD_META
#define SCI_SUPER SCMOD_SUPER
#define SCI_CSHIFT (SCI_CTRL | SCI_SHIFT)
#define SCI_ASHIFT (SCI_ALT | SCI_SHIFT)

/**
 */
class KeyModifiers {
public:
	int key;
	int modifiers;
	KeyModifiers(int key_, int modifiers_) noexcept : key(key_), modifiers(modifiers_) {
	}
	bool operator<(const KeyModifiers &other) const noexcept {
		if (key == other.key)
			return modifiers < other.modifiers;
		else
			return key < other.key;
	}
};

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
	std::map<KeyModifiers, unsigned int> kmap;
	static const KeyToCommand MapDefault[];

public:
	KeyMap();
	~KeyMap();
	void Clear() noexcept;
	void AssignCmdKey(int key, int modifiers, unsigned int msg);
	unsigned int Find(int key, int modifiers) const;	// 0 returned on failure
	const std::map<KeyModifiers, unsigned int> &GetKeyMap() const noexcept;
};

}

#endif
