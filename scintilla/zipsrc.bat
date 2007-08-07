cd ..
del/q scintilla.zip
zip scintilla.zip scintilla\*.* scintilla\*\*.* -x *.o -x *.obj -x *.dll -x *.lib -x *.res -x *.exp
cd scintilla
