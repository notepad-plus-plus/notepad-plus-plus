#pragma once
#include "pch.h"

#include "Helpers.h"

namespace NppShell::Installer
{
    HRESULT RegisterOldContextMenu();
    HRESULT UnregisterOldContextMenu();

    HRESULT RegisterSparsePackage();
    HRESULT UnregisterSparsePackage();

    HRESULT Install();
    HRESULT Uninstall();
}