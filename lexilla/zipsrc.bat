cd ..
del/q lexilla.zip
zip lexilla.zip lexilla\*.* lexilla\*\*.* lexilla\*\*\*.* lexilla\*\*\*\*.* lexilla\*\*\*\*\*.* -x *.bak -x *.o -x *.obj -x *.iobj -x *.dll -x *.exe -x *.a -x *.lib -x *.res -x *.exp -x *.sarif -x *.pdb -x *.ipdb -x *.idb -x *.sbr -x *.ilk
zip lexilla.zip -d lexilla/src/ARM64/*
zip lexilla.zip -d lexilla/src/x64/*
zip lexilla.zip -d lexilla/src/Release/*
zip lexilla.zip -d lexilla/src/Debug/*
zip lexilla.zip -d lexilla/src/__pycache__/*
zip lexilla.zip -d lexilla/test/ARM64/*
zip lexilla.zip -d lexilla/test/x64/*
zip lexilla.zip -d lexilla/test/Release/*
zip lexilla.zip -d lexilla/test/Debug/*
zip lexilla.zip -d lexilla/test/unit/ARM64/*
zip lexilla.zip -d lexilla/test/unit/x64/*
zip lexilla.zip -d lexilla/test/unit/Release/*
zip lexilla.zip -d lexilla/test/unit/Debug/*
zip lexilla.zip -d lexilla/scripts/__pycache__/*
cd lexilla
