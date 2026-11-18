// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Native update entry points, created 2026-11-18. No editor content is passed to the updater.
#pragma once
#include "Parameters.h"
#include "../../updater/Version.h"
#include <filesystem>
#include <fstream>

inline void noteQuUpdate(const wchar_t* mode)
{
    const auto& p = NppParameters::getInstance();
    const std::wstring helper = p.getNppPath() + L"\\NoteQuUpdater.exe";
    std::wstring args = L"\"" + helper + L"\" -mode " + mode + L" -root \"" + p.getNppPath()
        + L"\" -profile \"" + p.getUserPath() + L"\" -version " NOTEQU_UPDATE_VERSION_W
        + L" -parent " + std::to_wstring(GetCurrentProcessId());
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (CreateProcessW(helper.c_str(), args.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
        p.getNppPath().c_str(), &startup, &process))
    { CloseHandle(process.hThread); CloseHandle(process.hProcess); }
    else if (wcscmp(mode, L"auto") != 0)
        MessageBoxW(nullptr, L"The NoteQu updater could not be started. Reinstall the complete NoteQu package.", L"NoteQu Updates", MB_OK | MB_ICONERROR);
}
inline void noteQuUpdateHealthy()
{
    std::error_code ec;
    auto dir = std::filesystem::path(NppParameters::getInstance().getNppPath()) / L".notequ-update";
    std::filesystem::create_directories(dir, ec);
    if (!ec) { std::ofstream file(dir / L"healthy-" NOTEQU_UPDATE_VERSION_W, std::ios::binary); file << NOTEQU_UPDATE_VERSION; }
}
