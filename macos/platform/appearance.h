// appearance.h — Dark/light mode, theme switching
// Part of the Notepad++ macOS port modular refactor.

#pragma once

void applyFoldMarkerColorsToView(void* sci, bool isDark);
void applyAppearanceToView(void* sci, int langIdx, bool isDark);
void applyAppearance();
