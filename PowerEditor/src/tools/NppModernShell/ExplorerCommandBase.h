#pragma once
#include "pch.h"

namespace NppModernShell::CommandHandlers
{
    class ExplorerCommandBase : public winrt::implements<ExplorerCommandBase, IExplorerCommand>
    {
    public:
        virtual const wstring Title() = 0;
        virtual const wstring Icon() = 0;
        virtual const EXPCMDFLAGS Flags();
        virtual const EXPCMDSTATE State(_In_opt_ IShellItemArray* selection);

        IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name);
        IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon);
        IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip);
        IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState);
        IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* flags);
        IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName);
        IFACEMETHODIMP EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands);

        virtual IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept = 0;
    };
}