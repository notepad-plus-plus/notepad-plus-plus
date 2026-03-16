// session_manager.h — Session save/restore
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#include <string>

std::string sessionPath();
void saveSession();
void restoreSession();
