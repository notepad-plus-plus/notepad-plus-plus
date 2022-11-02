
Write-Output "Tools version:"
Write-Output (gcc --version) | select-object -first 1
Write-Output (mingw32-make --version) | select-object -first 1
Write-Output (sh --version) | select-object -first 1