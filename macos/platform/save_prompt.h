// save_prompt.h — Save-before-close prompts
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#include <string>
#include <vector>

enum class SavePromptResult
{
	Save,
	DontSave,
	Cancel
};

// Prompt for a single dirty document. Returns user's choice.
SavePromptResult promptSaveSingleDocument(const std::wstring& title);

// Check if a specific document (by view + tab index) is dirty.
bool isDocumentDirty(int viewIndex, int tabIndex);

// Save a document by view + tab index. Returns true if save succeeded.
bool saveDocumentAt(int viewIndex, int tabIndex);

// Prompt-and-handle for closing a single tab. Returns true if close should proceed.
bool promptAndHandleClose(int viewIndex, int tabIndex);

// Prompt-and-handle for closing all tabs in a view. Returns true if close-all should proceed.
bool promptAndHandleCloseAll(int viewIndex);

// Prompt-and-handle for quitting the app (all tabs in all views). Returns true if quit should proceed.
bool promptAndHandleQuit();
