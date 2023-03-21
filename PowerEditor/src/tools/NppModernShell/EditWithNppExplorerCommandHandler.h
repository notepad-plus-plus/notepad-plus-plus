#pragma once
#include "pch.h"

#include "ExplorerCommandBase.h"

namespace NppModernShell::CommandHandlers
{
    class EditWithNppExplorerCommandHandler : public ExplorerCommandBase
    {
    public:
        const wstring Title() override;
        const wstring Icon() override;

        IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept override;

    private:
        const wstring GetNppExecutableFullPath();
        const wstring GetCommandLine();
    };
}