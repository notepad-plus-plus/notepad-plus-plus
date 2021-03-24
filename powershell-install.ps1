# Downloads the latest portable x64 notepad++.zip 
# This will work if execution policy is  `Set-ExecutionPolicy Bypass -Scope Process -Force`
# one line installer:
# ===================
# iex (New-Object System.Net.WebClient).DownloadString('https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/powershell-install.ps1')
# OR
# Invoke-Expression -Command (New-Object System.Net.WebClient).DownloadString('https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/powershell-install.ps1')

# File browser for install path seleciton
if ($null -eq $args[0])
{	
	Add-Type -AssemblyName System.Windows.Forms
    $FolderBrowserDialog = New-Object System.Windows.Forms.FolderBrowserDialog
    $FolderBrowserDialog.RootFolder = 'MyComputer'
    $FolderBrowserDialog.ShowDialog()
    $install_dir = $FolderBrowserDialog.SelectedPath
}
# Specify install dir as cmdline parameter
else
{
    $install_dir = $args[0]
}

if ('' -eq $install_dir)
{
    exit
}

If(!(test-path $install_dir))
{
    New-Item -ItemType Directory -Force -Path $install_dir
}

$repo_name = "notepad-plus-plus/notepad-plus-plus"
$release = Invoke-RestMethod "https://api.github.com/repos/${repo_name}/releases/latest"
$version_tag =  $release.tag_name

$version_string = $version_tag.replace("v", "")

if ([System.Environment]::Is64BitOperatingSystem)
{
$download_url = "https://github.com/${repo_name}/releases/download/${version_tag}/npp.${version_string}.portable.x64.zip"
$portable_zip = "npp.${version_string}.portable.x64.zip"
}
else
{
$download_url = "https://github.com/${repo_name}/releases/download/${version_tag}/npp.${version_string}.portable.zip"
$portable_zip = "npp.${version_string}.portable.zip"
}


Import-Module BitsTransfer
Start-BitsTransfer $download_url $install_dir



$zip_path = "${install_dir}\${portable_zip}"

Expand-Archive -Path $zip_path -DestinationPath $install_dir
Remove-Item "${install_dir}\${portable_zip}"
