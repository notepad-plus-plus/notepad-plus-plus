// Scintilla source code edit control
/** @file KeyMap.h
 ** Defines a mapping between keystrokes and commands.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef KEYMAP_H
#define KEYMAP_H

namespace Scintilla::Internal {

#define SCI_NORM KeyMod::Norm
#define SCI_SHIFT KeyMod::Shift
#define SCI_CTRL KeyMod::Ctrl
#define SCI_ALT KeyMod::Alt
#define SCI_META KeyMod::Meta
#define SCI_SUPER KeyMod::Super
#define SCI_CSHIFT (KeyMod::Ctrl | KeyMod::Shift)
#define SCI_ASHIFT (KeyMod::Alt | KeyMod::Shift)

/**
 */
class KeyModifiers {
public:
	Scintilla::Keys key;
	Scintilla::KeyMod modifiers;
	KeyModifiers() noexcept : key{}, modifiers(KeyMod::Norm) {
	};
	KeyModifiers(Scintilla::Keys key_, Scintilla::KeyMod modifiers_) noexcept : key(key_), modifiers(modifiers_) {
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
	Scintilla::Keys key;
	Scintilla::KeyMod modifiers;
	Scintilla::Message msg;
};

/**
 */
class KeyMap {
	std::map<KeyModifiers, Scintilla::Message> kmap;
	static const KeyToCommand MapDefault[];

public:
	KeyMap();
	void Clear() noexcept;
	void AssignCmdKey(Scintilla::Keys key, Scintilla::KeyMod modifiers, Scintilla::Message msg);
	Scintilla::Message Find(Scintilla::Keys key, Scintilla::KeyMod modifiers) const;	// 0 returned on failure
	const std::map<KeyModifiers, Scintilla::Message> &GetKeyMap() const noexcept;
};

}

#endif
