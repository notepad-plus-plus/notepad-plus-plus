What is Notepad++ ?
===================

Notepad++ is a free (free as in both "free speech" and "free beer") source code editor and Notepad replacement that supports several programming languages and natural languages. Running in the MS Windows environment, its use is governed by GPL License.


To build Notepad++ package from source code:
============================================

There should be several ways to generate Notepad++ binaries, here we show you only the way with which Notepad++ official releases are generated.
* notepad++.exe: Visual Studio 2005
* SciLexer.dll: Visual Studio 2005 (with nmake)

notepad++.exe:
Double click on Notepad++\trunk\PowerEditor\visual.net\notepadPlus.vcproj to launch Notepad++ project in Visual Studio, then build it with the mode you want, that's it.

SciLexer.dll:
From version 6.0, SciLexer.dll comes with release contains boost's PCRE (Perl Compatible Regular Expressions) feature.
Therefore Boost (http://www.boost.org/) is needed to compile Scintilla in order to have PCRE support.
Here are the instructions to build SciLexer.dll for Notepad++:
1. Download source code of Boost from Boost site (http://www.boost.org/), unzip it. In my case, "boost_1_49_0" is copied in "C:\sources\"
2. go to Notepad++\trunk\scintilla\boostregex\ then run BuildBoost.bat with your boost path. In my case:
   BuildBoost.bat C:\sources\boost_1_49_0
3. go in Notepad++\trunk\scintilla\win32\ then run "nmake -f scintilla.mak"

You can build SciLexer.dll without Boost, ie. with its default POSIX regular expression support instead boost's PCRE one. It will work with notepad++.exe, however some functionalities in Notepad++ may be broken.
To build SciLexer.dll without PCRE support:
1. Go in Notepad++\trunk\scintilla\win32
2. Run nmake with an option:
   nmake NOBOOST=1 -f scintilla.mak

Notepad++ Unicode release binary (notepad++.exe) and Scintilla release binary (SciLexer.dll) will be built in the directories "notepad++\trunk\PowerEditor\bin" and "notepad++\trunk\scintilla\bin" respectively.
You have to copy SciLexer.dll in "notepad++\PowerEditor\bin" in order to launch notepad++.exe


Go to Notepad++ official site for more information :
http://notepad-plus-plus.org/


Notepad++ team
http://notepad-plus-plus.org/contributors
