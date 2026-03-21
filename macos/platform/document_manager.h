// document_manager.h — Tab/document management, state save/restore
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#include <string>
#include <vector>
#include "document_data.h"

void saveViewState(void* sci, std::vector<DocumentData>& docs, int tabIdx);
void saveScintillaState();
void restoreViewToScintilla(void* sci, std::vector<DocumentData>& docs, int tabIndex);
void restoreScintillaState(int tabIndex);

void switchToTabInView(int viewIndex, int tabIndex);
void switchToTab(int tabIndex);
int addNewTabToView(int viewIndex, const std::wstring& title, const std::string& content,
                    const std::wstring& filePath = L"", int langIndex = 2);
int addNewTab(const std::wstring& title, const std::string& content,
              const std::wstring& filePath = L"", int langIndex = 2);
void closeTabFromView(int viewIndex, int tabIndex);
void closeTab(int tabIndex);

void reorderTabInView(int viewIndex, int fromIndex, int toIndex);

void updateTabModifiedIndicator(int viewIndex, int tabIndex);
void updateWindowDocumentEdited();
