#include "pch.h"
#include "Installer.h"

#include "EditWithNppExplorerCommandHandler.h"

#define GUID_STRING_SIZE 40

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Management::Deployment;

using namespace NppShell::Helpers;
using namespace NppShell::Installer;

const wstring SparsePackageName = L"NotepadPlusPlus";
constexpr int FirstWindows11BuildNumber = 22000;

#ifdef WIN64
const wstring ShellExtensionKey = L"Software\\Classes\\*\\shell\\ANotepad++64";
#else
const wstring ShellExtensionKey = L"Software\\Classes\\*\\shell\\ANotepad++";
#endif

struct DOREGSTRUCT {
    HKEY hive;
    wstring key;
    wstring name;
    wstring value;
};

bool IsWindows11Installation()
{
    wstring keyName = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    wstring valueName = L"CurrentBuildNumber";

    bool result = false;

    HKEY hkey;
    HRESULT hResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName.c_str(), 0, KEY_READ, &hkey);

    if (hResult == ERROR_SUCCESS)
    {
        DWORD cbData = 0;
        hResult = RegGetValue(hkey, nullptr, valueName.c_str(), RRF_RT_REG_SZ, nullptr, nullptr, &cbData);

        if (hResult == ERROR_SUCCESS)
        {
            vector<BYTE> buffer(cbData + 1);

            cbData = (DWORD)buffer.size();
            hResult = RegGetValue(hkey, nullptr, valueName.c_str(), RRF_RT_REG_SZ, nullptr, &buffer[0], &cbData);

            wstring buildNumberString(reinterpret_cast<wchar_t*>(&buffer[0]));

            const int buildNumber = stoi(buildNumberString);

            result = buildNumber >= FirstWindows11BuildNumber;
        }
    }

    RegCloseKey(hkey);

    return result;
}

wstring GetCLSIDString()
{
    const auto uuid = __uuidof(NppShell::CommandHandlers::EditWithNppExplorerCommandHandler);

    LPOLESTR guidString = 0;
    const HRESULT result = StringFromCLSID(uuid, &guidString);

    if (FAILED(result))
    {
        if (guidString)
        {
            CoTaskMemFree(guidString);
        }

        throw "Failed to parse GUID from command handler";
    }

    wstring guid(guidString);

    if (guidString)
    {
        CoTaskMemFree(guidString);
    }

    return guid;
}

LRESULT RemoveRegistryKeyIfFound(HKEY hive, wstring keyName)
{
    HKEY hkey;
    const LRESULT lResult = RegOpenKeyEx(hive, keyName.c_str(), 0, KEY_READ, &hkey);

    if (lResult == ERROR_FILE_NOT_FOUND)
    {
        // Does not exist, so nothing to remove
        return ERROR_SUCCESS;
    }

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegCloseKey(hkey);

    return RegDeleteTree(hive, keyName.c_str());
}

LRESULT CreateRegistryKey(const HKEY hive, const wstring& key, const wstring& name, const wstring& value)
{
    HKEY regKey;
    LRESULT lResult = RegCreateKeyEx(hive, key.data(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &regKey, NULL);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    lResult = RegSetKeyValue(regKey, NULL, name.empty() ? NULL : name.data(), REG_SZ, value.data(), static_cast<DWORD>(value.length() * sizeof TCHAR));
    
    RegCloseKey(regKey);

    return lResult;
}

LRESULT CleanupRegistry(const wstring guid)
{
    constexpr int bufferSize = MAX_PATH + GUID_STRING_SIZE;
    WCHAR buffer[bufferSize];
    const auto arraySize = bufferSize * sizeof(WCHAR);

    LRESULT result;

    result = RemoveRegistryKeyIfFound(HKEY_CLASSES_ROOT, ShellExtensionKey);

    if (result != ERROR_SUCCESS)
    {
        return result;
    }

    StringCbPrintf(buffer, arraySize, L"Notepad++_file\\shellex");
    result = RemoveRegistryKeyIfFound(HKEY_CLASSES_ROOT, buffer);
    if (result != ERROR_SUCCESS)
    {
        return result;
    }

    StringCbPrintf(buffer, arraySize, L"Software\\Classes\\CLSID\\%s", guid.c_str());
    result = RemoveRegistryKeyIfFound(HKEY_LOCAL_MACHINE, buffer);
    if (result != ERROR_SUCCESS)
    {
        return result;
    }

    return ERROR_SUCCESS;
}

LRESULT CleanupHack()
{
    wstring keyName = L"SOFTWARE\\Classes\\*\\shell\\pintohome";
    wstring valueName = L"MUIVerb";

    LRESULT result = 0;
    bool found = false;

    HKEY hkey;
    HRESULT hResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName.c_str(), 0, KEY_READ, &hkey);

    if (hResult == ERROR_SUCCESS)
    {
        DWORD cbData = 0;
        hResult = RegGetValue(hkey, nullptr, valueName.c_str(), RRF_RT_REG_SZ, nullptr, nullptr, &cbData);

        if (hResult == ERROR_SUCCESS)
        {
            vector<BYTE> buffer(cbData + 1);

            cbData = (DWORD)buffer.size();
            hResult = RegGetValue(hkey, nullptr, valueName.c_str(), RRF_RT_REG_SZ, nullptr, &buffer[0], &cbData);

            wstring valueString(reinterpret_cast<wchar_t*>(&buffer[0]));

            found = valueString.find(L"Notepad++") != wstring::npos;
        }
    }

    RegCloseKey(hkey);

    if (found)
    {
        result = RegDeleteTree(HKEY_LOCAL_MACHINE, keyName.c_str());
    }

    return result;
}

HRESULT NppShell::Installer::RegisterSparsePackage()
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

HRESULT NppShell::Installer::UnregisterSparsePackage()
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

HRESULT NppShell::Installer::RegisterOldContextMenu()
{
    const wstring installationPath = GetInstallationPath();
    const wstring guid = GetCLSIDString();

    CreateRegistryKey(HKEY_LOCAL_MACHINE, ShellExtensionKey, L"ExplorerCommandHandler", guid.c_str());
    CreateRegistryKey(HKEY_LOCAL_MACHINE, ShellExtensionKey, L"", L"Notepad++ Context menu");
    CreateRegistryKey(HKEY_LOCAL_MACHINE, ShellExtensionKey, L"NeverDefault", L"");
    CreateRegistryKey(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\" + guid, L"", L"notepad++");
    CreateRegistryKey(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\" + guid + L"\\InProcServer32", L"", installationPath + L"\\NppShell.dll");
    CreateRegistryKey(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\" + guid + L"\\InProcServer32", L"ThreadingModel", L"Apartment");

    return S_OK;
}

HRESULT NppShell::Installer::UnregisterOldContextMenu()
{
    const wstring guid = GetCLSIDString();

    // Clean up registry entries.
    CleanupRegistry(guid);

    // Clean up the 8.5 Windows 11 hack if present.
    CleanupHack();

    return S_OK;
}

HRESULT NppShell::Installer::Install()
{
    const bool isWindows11 = IsWindows11Installation();

    HRESULT result;

    UnregisterOldContextMenu();

    if (isWindows11)
    {
        UnregisterSparsePackage();

        result = RegisterSparsePackage();
    }
    else
    {
        result = RegisterOldContextMenu();
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);

    return result;
}

HRESULT NppShell::Installer::Uninstall()
{
    const bool isWindows11 = IsWindows11Installation();

    HRESULT result;
    result = UnregisterOldContextMenu();

    if (result != S_OK)
    {
        return result;
    }

    if (isWindows11)
    {
        result = UnregisterSparsePackage();

        if (result != S_OK)
        {
            return result;
        }
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);

    return S_OK;
}