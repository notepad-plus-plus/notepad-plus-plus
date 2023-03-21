cd ..
del/q lexilla.zip
zip lexilla.zip lexilla\*.* lexilla\*\*.* lexilla\*\*\*.* lexilla\*\*\*\*.* lexilla\*\*\*\*\*.* ^
 -x *.bak *.o *.obj *.iobj *.dll *.exe *.a *.lib *.res *.exp *.sarif *.pdb *.ipdb *.idb *.sbr *.ilk *.tgz ^
 **/__pycache__/* **/Debug/* **/Release/* **/x64/* **/ARM64/* **/cov-int/* **/.vs/* **/.vscode/* @
cd lexilla
