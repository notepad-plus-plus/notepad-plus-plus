#pragma once
#include "pch.h"

#include "ExplorerCommandBase.h"

namespace NppShell::CommandHandlers
{
#ifdef WIN64
    class __declspec(uuid("B298D29A-A6ED-11DE-BA8C-A68E55D89593")) EditWithNppExplorerCommandHandler : public ExplorerCommandBase
#else
    class __declspec(uuid("00F3C2EC-A6EE-11DE-A03A-EF8F55D89593")) EditWithNppExplorerCommandHandler : public ExplorerCommandBase
#endif
    {
    public:
        const wstring Title() override;
        const wstring Icon() override;

        IFACEMETHODIMP Invoke(IShellItemArray* psiItemArray, IBindCtx* pbc) noexcept override;

    private:
        const wstring GetNppExecutableFullPath();
        const wstring GetCommandLine();
    };
}