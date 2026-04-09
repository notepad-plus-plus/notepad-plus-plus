// nppm_handler.h — NPPM_* and RUNCOMMAND_USER message handlers
// Handles plugin messages sent via SendMessage(nppHandle, NPPM_*, ...)

#pragma once

#include "windows.h"

LRESULT handleNppmMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT handleRunCommandMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
