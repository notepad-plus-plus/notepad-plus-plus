// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Synthetic integration-test plugin. Never included in application distributions.
#include <windows.h>
#include <filesystem>
#include <fstream>
#include "PluginInterface.h"
static HINSTANCE instance;
static void probe() {
    wchar_t path[32768]{};
    GetModuleFileNameW(instance,path,32768);
    std::ofstream out(std::filesystem::path(path).parent_path()/"fixture-ran.txt");
    out << "NoteX fixture plugin command executed.\n";
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH)instance=module;return TRUE;}
extern "C" __declspec(dllexport) void setInfo(NppData) {}
extern "C" __declspec(dllexport) const wchar_t* getName() {return L"NoteX Update Fixture";}
extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* count) {static FuncItem item{L"Update Fixture Check",probe,0,false,nullptr};*count=1;return &item;}
extern "C" __declspec(dllexport) void beNotified(SCNotification*) {}
extern "C" __declspec(dllexport) LRESULT messageProc(UINT,WPARAM,LPARAM) {return TRUE;}
extern "C" __declspec(dllexport) BOOL isUnicode() {return TRUE;}
