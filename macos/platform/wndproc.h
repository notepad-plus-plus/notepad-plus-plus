// wndproc.h — Main window procedure (command dispatch)
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#include <cstdint>

// Win32 types from the shim — these are the actual typedefs
typedef void* HWND;
typedef unsigned int UINT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;

#ifndef CALLBACK
#define CALLBACK
#endif

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
