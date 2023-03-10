cd ..
del/q scintilla.zip
zip scintilla.zip scintilla\*.* scintilla\*\*.* scintilla\*\*\*.* scintilla\*\*\*\*.* scintilla\*\*\*\*\*.* ^
 -x *.o *.obj *.dll *.lib *.res *.exp *.bak *.tgz ^
 **/__pycache__/* **/Debug/* **/Release/* **/x64/* **/ARM64/* **/cov-int/* */.hg/* @
cd scintilla
