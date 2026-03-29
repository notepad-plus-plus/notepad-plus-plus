// edit_commands.h — Edit menu commands
// Part of the Notepad++ macOS port modular refactor.

#pragma once

void doTitleCase();
void doTrimTrailingWhitespace();
void doRemoveEmptyLines();
void doToggleLineComment();
void doSortLines(bool ascending);
void doJoinLines();
void doTabsToSpaces();
void doSpacesToTabs();
void insertDateTimeShort();
void insertDateTimeLong();

// Sprint P2 — additional case conversions
void doSentenceCase();
void doInvertCase();
void doCamelCase();
void doSnakeCase();

// Sprint P2 — sort / line operation variants
void doSortLinesCaseInsensitive();
void doSortLinesReverse();
void doRemoveDuplicateLines();
void doSortLinesNumeric();
void doSortLinesRandom();
