// menu_builder.h — Menu bar construction
// Part of the Notepad++ macOS port modular refactor.

#pragma once

// HMENU is void* in the Win32 shim
typedef void* HMENU;

HMENU buildMenuBar();
