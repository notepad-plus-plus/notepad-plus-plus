// find_replace.h — Find/Replace dialog and search logic
// Part of the Notepad++ macOS port modular refactor.

#pragma once

void updateFindStatus(const wchar_t* msg);
int buildSearchFlags();
bool doFindNext(bool forward);
int doCount();
void doReplaceOne();
void doReplaceAll();
void readFindDlgState();
void createFindReplaceDlg(bool replaceMode);
void showGoToLineDlg();
