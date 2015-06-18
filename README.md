What is Notepad++ ?
===================

[![Join the chat at https://gitter.im/notepad-plus-plus/notepad-plus-plus](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/notepad-plus-plus/notepad-plus-plus?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Notepad++ is a free (free as in both "free speech" and "free beer") source code editor and Notepad replacement that supports several programming languages and natural languages. Running in the MS Windows environment, its use is governed by GPL License.


To build Notepad++ package from source code:
============================================

There should be several ways to generate Notepad++ binaries, here we show you only the way with which Notepad++ official releases are generated.
* notepad++.exe: Visual Studio 2013
* SciLexer.dll: Visual Studio 2013 (with nmake)

notepad++.exe:
Double click on `PowerEditor\visual.net\notepadPlus.vcproj` to launch Notepad++ project in Visual Studio, then build it with the mode you want, that's it.

SciLexer.dll:
From version 6.0, SciLexer.dll comes with release contains boost's PCRE (Perl Compatible Regular Expressions) feature.
Therefore [Boost](http://www.boost.org/) is needed to compile Scintilla in order to have PCRE support.
Here are the instructions to build SciLexer.dll for Notepad++:
 1. Download the [Boost source code](http://sourceforge.net/projects/boost/files/boost/1.55.0/). v1.55 should be used with VS 2013. Then unzip it. In my case, "boost_1_55_0" is copied in `C:\sources\`
 2. Go to `scintilla\boostregex\` then run BuildBoost.bat with your boost path. In my case: `BuildBoost.bat C:\sources\boost_1_55_0`
 3. Go in `scintilla\win32\` then run `nmake -f scintilla.mak`

You can build SciLexer.dll without Boost, ie. with its default POSIX regular expression support instead boost's PCRE one. It will work with notepad++.exe, however some functionalities in Notepad++ may be broken.
To build SciLexer.dll without PCRE support:
 1. Go in `scintilla\win32\`
 2. Run nmake with an option `nmake NOBOOST=1 -f scintilla.mak`

Notepad++ Unicode release binary (notepad++.exe) and Scintilla release binary (SciLexer.dll) will be built in the directories `PowerEditor\bin\` and `scintilla\bin\` respectively.
You have to copy SciLexer.dll in `PowerEditor\bin\` in order to launch notepad++.exe


Go to [Notepad++ official site](http://notepad-plus-plus.org/) for more information.

[Notepad++ Contributors](http://notepad-plus-plus.org/contributors)
