copy /Y ".\localization\*.*" ..\bin\localization
cd ..\bin\

del /F /S /Q .\zipped.package.release\unicode\*.*
copy /Y license.txt .\zipped.package.release\unicode\
copy /Y readme.txt .\zipped.package.release\unicode\
copy /Y NppHelp.chm .\zipped.package.release\unicode\
copy /Y change.log .\zipped.package.release\unicode\
copy /Y config.model.xml .\zipped.package.release\unicode\
copy /Y langs.model.xml .\zipped.package.release\unicode\
copy /Y stylers.model.xml .\zipped.package.release\unicode\
copy /Y contextMenu.xml .\zipped.package.release\unicode\
copy /Y shortcuts.xml .\zipped.package.release\unicode\
copy /Y doLocalConf.xml .\zipped.package.release\unicode\
copy /Y LINEDRAW.TTF .\zipped.package.release\unicode\
copy /Y "notepad++.exe" .\zipped.package.release\unicode\
copy /Y SciLexer.dll .\zipped.package.release\unicode\
copy /Y ".\plugins\*.*" .\zipped.package.release\unicode\plugins\
copy /Y ".\plugins\APIs\*.xml" .\zipped.package.release\unicode\plugins\APIs
copy /Y ".\plugins\doc\*.*" .\zipped.package.release\unicode\plugins\doc
copy /Y ".\plugins\Config\tidy\*.*" .\zipped.package.release\unicode\plugins\Config\tidy
copy /Y ".\localization\*.*" .\zipped.package.release\unicode\localization


del /F /S /Q .\zipped.package.release\ansi\config.xml
del /F /S /Q .\zipped.package.release\ansi\langs.xml
del /F /S /Q .\zipped.package.release\ansi\stylers.xml
del /F /S /Q .\zipped.package.release\ansi\session.xml
del /F /S /Q .\zipped.package.release\ansi\plugins\Config\*.*
copy /Y license.txt .\zipped.package.release\ansi\
copy /Y readme.txt .\zipped.package.release\ansi\
copy /Y NppHelp.chm .\zipped.package.release\ansi\
copy /Y change.log .\zipped.package.release\ansi\
copy /Y config.model.xml .\zipped.package.release\ansi\
copy /Y langs.model.xml .\zipped.package.release\ansi\
copy /Y stylers.model.xml .\zipped.package.release\ansi\
copy /Y contextMenu.xml .\zipped.package.release\ansi\
copy /Y shortcuts.xml .\zipped.package.release\ansi\
copy /Y doLocalConf.xml .\zipped.package.release\ansi\
copy /Y LINEDRAW.TTF .\zipped.package.release\ansi\
copy /Y ".\plugins\Config\tidy\*.*" .\zipped.package.release\ansi\plugins\Config\tidy

"C:\Program Files\7-Zip\7z.exe" a -tzip -r npp.bin.zip .\zipped.package.release\*
"C:\Program Files\NSIS\makensis.exe" ..\installer\nppSetup.nsi

cd ..\installer\

