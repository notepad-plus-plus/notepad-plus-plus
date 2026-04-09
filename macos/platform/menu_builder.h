// menu_builder.h — Menu bar construction
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#import <Cocoa/Cocoa.h>
#include "windows.h"

HMENU buildMenuBar();

// Returns the Plugins submenu handle (populated by MacPluginManager after loading)
HMENU getPluginsMenuHandle();
