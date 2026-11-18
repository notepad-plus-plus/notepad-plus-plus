// Copyright (C) 2026 Berk Kivircik.
// SPDX-License-Identifier: GPL-3.0-only
// Created for NoteX; publication date: 2026-10-28.

#pragma once
#include <windows.h>
#include "MISC/sha2/sha-256.h"
#include "NoteXFormat.h"
#include "Scintilla.h"
#include <array>
#include <atomic>

namespace NoteX
{
inline std::array<unsigned char, 32> textDigest(HWND editor)
{
    auto length = static_cast<size_t>(SendMessage(editor, SCI_GETLENGTH, 0, 0));
    auto text = reinterpret_cast<const char *>(SendMessage(editor, SCI_GETCHARACTERPOINTER, 0, 0));
    std::array<unsigned char, 32> hash{};
    calc_sha_256(hash.data(), text, length);
    return hash;
}
inline std::wstring sidecarPath(const wchar_t *file) { return std::wstring(file) + L".notex"; }
inline std::wstring previousSidecarPath(const wchar_t *file) { return std::wstring(file) + previousSidecarExtension(); }
inline bool validSidecarMagic(const char *magic) { return memcmp(magic, "NOTEXSC1", 8) == 0 || memcmp(magic, "NPPSIDE1", 8) == 0; }
inline bool saveSidecar(HWND editor, const wchar_t *file)
{
    State state;
    SendMessage(editor, GetState, 0, reinterpret_cast<LPARAM>(&state));
    auto path = sidecarPath(file);
    if (state.runs.empty() && GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(previousSidecarPath(file).c_str()) == INVALID_FILE_ATTRIBUTES)
        return true;
    // Never replace an unrelated file occupying the companion filename.
    HANDLE existing =
        CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (existing != INVALID_HANDLE_VALUE)
    {
        char magic[8]{};
        DWORD read = 0;
        bool ours =
            ReadFile(existing, magic, 8, &read, nullptr) && read == 8 && validSidecarMagic(magic);
        CloseHandle(existing);
        if (!ours)
        {
            SetLastError(ERROR_FILE_EXISTS);
            return false;
        }
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND)
        return false;
    // Remove unused historical formats from the persisted representation.
    State compact;
    for (auto r : state.runs)
    {
        auto &f = state.formats[r.style];
        auto it = std::find(compact.formats.begin(), compact.formats.end(), f);
        if (it == compact.formats.end())
        {
            compact.formats.push_back(f);
            r.style = static_cast<unsigned>(compact.formats.size() - 1);
        }
        else
            r.style = static_cast<unsigned>(it - compact.formats.begin());
        compact.runs.push_back(r);
    }
    auto hash = textDigest(editor);
    std::string bytes = "NOTEXSC1";
    bytes.append(reinterpret_cast<const char *>(hash.data()), hash.size());
    bytes += encode(compact);
    static std::atomic<unsigned> serial{0};
    auto temp = path + L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++serial);
    HANDLE h =
        CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    bool ok = bytes.size() <= MAXDWORD &&
              WriteFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
              written == bytes.size() && FlushFileBuffers(h);
    DWORD error = GetLastError();
    CloseHandle(h);
    if (ok)
    {
        ok = MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        error = GetLastError();
    }
    if (!ok)
    {
        DeleteFileW(temp.c_str());
        SetLastError(error ? error : ERROR_WRITE_FAULT);
    }
    return ok;
}
inline void loadSidecar(HWND editor, const wchar_t *file)
{
    Restore restore;
    SendMessage(editor, SetState, 0, reinterpret_cast<LPARAM>(&restore));
    const auto currentPath = sidecarPath(file);
    auto path = currentPath;
    bool previousFile = false;
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        path = previousSidecarPath(file);
        previousFile = true;
    }
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(h, &size) && size.QuadPart >= 64 && size.QuadPart <= 64 * 1024 * 1024;
    std::string bytes;
    if (ok)
    {
        bytes.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        ok = ReadFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) &&
             read == bytes.size();
    }
    CloseHandle(h);
    auto hash = textDigest(editor);
    ok = ok && validSidecarMagic(bytes.data()) && memcmp(bytes.data() + 8, hash.data(), 32) == 0;
    if (ok)
        ok = decode(bytes.substr(40), restore.state, SendMessage(editor, SCI_GETLENGTH, 0, 0));
    if (ok) {
        SendMessage(editor, SetState, 0, reinterpret_cast<LPARAM>(&restore));
        // Rename only a validated companion, without replacing a newer file.
        if (previousFile) MoveFileExW(path.c_str(), currentPath.c_str(), MOVEFILE_WRITE_THROUGH);
    }
    else
        MessageBoxW(
            GetAncestor(editor, GA_ROOT),
            (L"BiÃ§im dosyasÄ± geÃ§ersiz veya metin deÄŸiÅŸmiÅŸ. YanlÄ±ÅŸ aralÄ±klara biÃ§im uygulanmadÄ±.\n\n" + path)
                .c_str(),
            L"NoteX â€” BiÃ§imlendirme", MB_OK | MB_ICONINFORMATION);
}
} // namespace NoteX
