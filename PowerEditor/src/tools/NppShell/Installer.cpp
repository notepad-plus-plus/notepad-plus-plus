#include "pch.h"
#include "Installer.h"

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Management::Deployment;

using namespace NppShell::Helpers;
using namespace NppShell::Installer;

const wstring SparsePackageName = L"NotepadPlusPlus";

STDAPI NppShell::Installer::RegisterSparsePackage()
{
    PackageManager packageManager;
    AddPackageOptions options;

    const wstring externalLocation = GetInstallationPath();
    const wstring sparsePkgPath = externalLocation + L"\\NppShell.msix";

    Uri externalUri(externalLocation);
    Uri packageUri(sparsePkgPath);

    options.ExternalLocationUri(externalUri);

    auto deploymentOperation = packageManager.AddPackageByUriAsync(packageUri, options);
    auto deployResult = deploymentOperation.get();

    if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
    {
        return deployResult.ExtendedErrorCode();
    }

    return S_OK;
}

STDAPI NppShell::Installer::UnregisterSparsePackage()
{
    PackageManager packageManager;
    IIterable<Package> packages;

    try
    {
        packages = packageManager.FindPackagesForUser(L"");
    }
    catch (winrt::hresult_error const& ex)
    {
        return ex.code();
    }

    for (const Package& package : packages)
    {
        if (package.Id().Name() != SparsePackageName)
        {
            continue;
        }

        winrt::hstring fullName = package.Id().FullName();
        auto deploymentOperation = packageManager.RemovePackageAsync(fullName, RemovalOptions::None);
        auto deployResult = deploymentOperation.get();

        if (!SUCCEEDED(deployResult.ExtendedErrorCode()))
        {
            return deployResult.ExtendedErrorCode();
        }

        break;
    }

    return S_OK;
}