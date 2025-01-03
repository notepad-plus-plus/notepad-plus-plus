// Scintilla source code edit control
/** @file KeyMap.cxx
 ** Defines a mapping between keystrokes and commands.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cstdint>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <memory>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "KeyMap.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

KeyMap::KeyMap() {
	for (int i = 0; static_cast<int>(MapDefault[i].key); i++) {
		AssignCmdKey(MapDefault[i].key,
			MapDefault[i].modifiers,
			MapDefault[i].msg);
	}
}

void KeyMap::Clear() noexcept {
	kmap.clear();
}

void KeyMap::AssignCmdKey(Keys key, KeyMod modifiers, Message msg) {
	kmap[KeyModifiers(key, modifiers)] = msg;
}

Message KeyMap::Find(Keys key, KeyMod modifiers) const {
	std::map<KeyModifiers, Message>::const_iterator it = kmap.find(KeyModifiers(key, modifiers));
	return (it == kmap.end()) ? static_cast<Message>(0) : it->second;
}

const std::map<KeyModifiers, Message> &KeyMap::GetKeyMap() const noexcept {
	return kmap;
}

#if PLAT_GTK_MACOSX
#define OS_X_KEYS 1
#else
#define OS_X_KEYS 0
#endif

// Define a modifier that is exactly Ctrl key on all platforms
// Most uses of Ctrl map to Cmd on macOS but some can't so use SCI_[S]CTRL_META
#if OS_X_KEYS
#define SCI_CTRL_META SCI_META
#define SCI_SCTRL_META (SCI_META | SCI_SHIFT)
#else
#define SCI_CTRL_META SCI_CTRL
#define SCI_SCTRL_META (SCI_CTRL | SCI_SHIFT)
#endif

namespace {

constexpr Keys Key(char ch) noexcept {
    return static_cast<Keys>(ch);
}

}

const KeyToCommand KeyMap::MapDefault[] = {

#if OS_X_KEYS
    {Keys::Down,		SCI_CTRL,	Message::DocumentEnd},
    {Keys::Down,		SCI_CSHIFT,	Message::DocumentEndExtend},
    {Keys::Up,		    SCI_CTRL,	Message::DocumentStart},
    {Keys::Up,		    SCI_CSHIFT,	Message::DocumentStartExtend},
    {Keys::Left,		SCI_CTRL,	Message::VCHome},
    {Keys::Left,		SCI_CSHIFT,	Message::VCHomeExtend},
    {Keys::Right,		SCI_CTRL,	Message::LineEnd},
    {Keys::Right,		SCI_CSHIFT,	Message::LineEndExtend},
#endif

    {Keys::Down,		SCI_NORM,	Message::LineDown},
    {Keys::Down,		SCI_SHIFT,	Message::LineDownExtend},
    {Keys::Down,		SCI_CTRL_META,	Message::LineScrollDown},
    {Keys::Down,		SCI_ASHIFT,	Message::LineDownRectExtend},
    {Keys::Up,		    SCI_NORM,	Message::LineUp},
    {Keys::Up,			SCI_SHIFT,	Message::LineUpExtend},
    {Keys::Up,			SCI_CTRL_META,	Message::LineScrollUp},
    {Keys::Up,		    SCI_ASHIFT,	Message::LineUpRectExtend},
    {Key('['),			SCI_CTRL,	Message::ParaUp},
    {Key('['),			SCI_CSHIFT,	Message::ParaUpExtend},
    {Key(']'),			SCI_CTRL,	Message::ParaDown},
    {Key(']'),			SCI_CSHIFT,	Message::ParaDownExtend},
    {Keys::Left,		SCI_NORM,	Message::CharLeft},
    {Keys::Left,		SCI_SHIFT,	Message::CharLeftExtend},
    {Keys::Left,		SCI_CTRL_META,	Message::WordLeft},
    {Keys::Left,		SCI_SCTRL_META,	Message::WordLeftExtend},
    {Keys::Left,		SCI_ASHIFT,	Message::CharLeftRectExtend},
    {Keys::Right,		SCI_NORM,	Message::CharRight},
    {Keys::Right,		SCI_SHIFT,	Message::CharRightExtend},
    {Keys::Right,		SCI_CTRL_META,	Message::WordRight},
    {Keys::Right,		SCI_SCTRL_META,	Message::WordRightExtend},
    {Keys::Right,		SCI_ASHIFT,	Message::CharRightRectExtend},
    {Key('/'),		    SCI_CTRL,	Message::WordPartLeft},
    {Key('/'),		    SCI_CSHIFT,	Message::WordPartLeftExtend},
    {Key('\\'),		    SCI_CTRL,	Message::WordPartRight},
    {Key('\\'),		    SCI_CSHIFT,	Message::WordPartRightExtend},
    {Keys::Home,		SCI_NORM,	Message::VCHome},
    {Keys::Home, 		SCI_SHIFT, 	Message::VCHomeExtend},
    {Keys::Home, 		SCI_CTRL, 	Message::DocumentStart},
    {Keys::Home, 		SCI_CSHIFT, Message::DocumentStartExtend},
    {Keys::Home, 		SCI_ALT, 	Message::HomeDisplay},
    {Keys::Home,		SCI_ASHIFT,	Message::VCHomeRectExtend},
    {Keys::End,	 	    SCI_NORM,	Message::LineEnd},
    {Keys::End,	 	    SCI_SHIFT, 	Message::LineEndExtend},
    {Keys::End, 		SCI_CTRL, 	Message::DocumentEnd},
    {Keys::End, 		SCI_CSHIFT, Message::DocumentEndExtend},
    {Keys::End, 		SCI_ALT, 	Message::LineEndDisplay},
    {Keys::End,		    SCI_ASHIFT,	Message::LineEndRectExtend},
    {Keys::Prior,		SCI_NORM,	Message::PageUp},
    {Keys::Prior,		SCI_SHIFT, 	Message::PageUpExtend},
    {Keys::Prior,		SCI_ASHIFT,	Message::PageUpRectExtend},
    {Keys::Next, 		SCI_NORM, 	Message::PageDown},
    {Keys::Next, 		SCI_SHIFT, 	Message::PageDownExtend},
    {Keys::Next,		SCI_ASHIFT,	Message::PageDownRectExtend},
    {Keys::Delete,      SCI_NORM,	Message::Clear},
    {Keys::Delete, 	    SCI_SHIFT,	Message::Cut},
    {Keys::Delete, 	    SCI_CTRL,	Message::DelWordRight},
    {Keys::Delete,	    SCI_CSHIFT,	Message::DelLineRight},
    {Keys::Insert, 		SCI_NORM,	Message::EditToggleOvertype},
    {Keys::Insert, 		SCI_SHIFT,	Message::Paste},
    {Keys::Insert, 		SCI_CTRL,	Message::Copy},
    {Keys::Escape,  	SCI_NORM,	Message::Cancel},
    {Keys::Back,		SCI_NORM, 	Message::DeleteBack},
    {Keys::Back,		SCI_SHIFT, 	Message::DeleteBack},
    {Keys::Back,		SCI_CTRL, 	Message::DelWordLeft},
    {Keys::Back, 		SCI_ALT,	Message::Undo},
    {Keys::Back,		SCI_CSHIFT,	Message::DelLineLeft},
    {Key('Z'), 			SCI_CTRL,	Message::Undo},
#if OS_X_KEYS
    {Key('Z'), 			SCI_CSHIFT,	Message::Redo},
#else
    {Key('Y'), 			SCI_CTRL,	Message::Redo},
#endif
    {Key('X'), 			SCI_CTRL,	Message::Cut},
    {Key('C'), 			SCI_CTRL,	Message::Copy},
    {Key('V'), 			SCI_CTRL,	Message::Paste},
    {Key('A'), 			SCI_CTRL,	Message::SelectAll},
    {Keys::Tab,		    SCI_NORM,	Message::Tab},
    {Keys::Tab,		    SCI_SHIFT,	Message::BackTab},
    {Keys::Return, 	    SCI_NORM,	Message::NewLine},
    {Keys::Return, 	    SCI_SHIFT,	Message::NewLine},
    {Keys::Add, 		SCI_CTRL,	Message::ZoomIn},
    {Keys::Subtract,	SCI_CTRL,	Message::ZoomOut},
    {Keys::Divide,	    SCI_CTRL,	Message::SetZoom},
    {Key('L'), 			SCI_CTRL,	Message::LineCut},
    {Key('L'), 			SCI_CSHIFT,	Message::LineDelete},
    {Key('T'), 			SCI_CSHIFT,	Message::LineCopy},
    {Key('T'), 			SCI_CTRL,	Message::LineTranspose},
    {Key('D'), 			SCI_CTRL,	Message::SelectionDuplicate},
    {Key('U'), 			SCI_CTRL,	Message::LowerCase},
    {Key('U'), 			SCI_CSHIFT,	Message::UpperCase},
    {Key(0),SCI_NORM,static_cast<Message>(0)},
};

