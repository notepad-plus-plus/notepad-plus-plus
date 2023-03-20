#pragma once
#include "pch.h"

namespace NppShell::CommandHandlers
{
    class ExplorerCommandBase : public winrt::implements<ExplorerCommandBase, IExplorerCommand>
    {
    public:
        virtual const wstring Title() = 0;
        virtual const wstring Icon() = 0;
        virtual const EXPCMDFLAGS Flags();
        virtual const EXPCMDSTATE State(IShellItemArray* psiItemArray);

        IFACEMETHODIMP GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName);
        IFACEMETHODIMP GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon);
        IFACEMETHODIMP GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip);
        IFACEMETHODIMP GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState);
        IFACEMETHODIMP GetFlags(EXPCMDFLAGS* flags);
        IFACEMETHODIMP GetCanonicalName(GUID* pguidCommandName);
        IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** ppEnum);

        virtual IFACEMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) noexcept = 0;
    };
}