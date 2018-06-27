What is Notepad++ ?
===================

[![Join the disscussions at https://notepad-plus-plus.org/community/](https://notepad-plus-plus.org/assets/images/NppCommunityBadge.svg)](https://notepad-plus-plus.org/community/)
&nbsp;&nbsp;&nbsp;&nbsp;[![Join the chat at https://gitter.im/notepad-plus-plus/notepad-plus-plus](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/notepad-plus-plus/notepad-plus-plus?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Notepad++ is a free (free as in both "free speech" and "free beer") source code
editor and Notepad replacement that supports several programming languages and
natural languages. Running in the MS-Windows environment, its use is governed by
the GNU General Public License.

Build Status
------------

[![Appveyor build status](https://ci.appveyor.com/api/projects/status/github/notepad-plus-plus/notepad-plus-plus?branch=master&svg=true)](https://ci.appveyor.com/project/donho/notepad-plus-plus)
[![GitHub release](https://img.shields.io/github/release/notepad-plus-plus/notepad-plus-plus.svg)]()

To build Notepad++ from source:
-------------------------------

There are two components that need to be built separately:

 - `notepad++.exe`: (depends on `SciLexer.dll`)
 - `SciLexer.dll` : (with nmake)

You can build Notepad++ with *or* without the Boost libraries (the release
build of Notepad++ is built **with** Boost).

Since `Notepad++` version 6.0, the build of `SciLexer.dll` that is distributed
uses features from Boost's `Boost.Regex` library.

You can build SciLexer.dll without Boost, i.e., with its default POSIX Regular
Expression support instead of Boost's PCRE (Perl Compatible Regular Expressions)
version. This is useful if you would like to debug Notepad++, but don't have Boost.

## To build `notepad++.exe`:

 1. Open [`PowerEditor\visual.net\notepadPlus.vcxproj`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/visual.net/notepadPlus.vcxproj)
 2. Build Notepad++ [like a normal Visual Studio project](https://msdn.microsoft.com/en-us/library/7s88b19e.aspx)



## To build `SciLexer.dll` with Boost:

These are the instructions for building SciLexer.dll (for both 32-bit & 64-bit) for Notepad++:

 1. Download the [Boost source code](https://sourceforge.net/projects/boost/files/boost/1.55.0/)
    (v1.55 should be used with VS 2013), then unzip it (in my case, `boost_1_55_0` is copied in `C:\sources\`).
 2. Change directory to `scintilla\boostregex\` then run BuildBoost.bat with your Boost path
    In my case: `BuildBoost.bat C:\sources\boost_1_55_0`
	If you are compiling a 64-bit Scintilla under your *VS2013 x64 Native tool command prompt*, add the `-x64` flag.
	In my case, the command looks like this: `BuildBoost.bat C:\sources\boost_1_55_0 -x64`
 3. Change directory to `scintilla\win32\` then run `nmake -f scintilla.mak`



## To build `SciLexer.dll` *without* boost:

This will work with `notepad++.exe`, however some functionality in Notepad++ will be broken.

To build SciLexer.dll without PCRE support (for both 32-bit & 64-bit):

 1. For 32-bit Windows, open a command prompt *for building* ([a.k.a. the *Developer Command Prompt for VS2013*](https://msdn.microsoft.com/en-us/library/f2ccy3wt.aspx))
    - From the IDE, you can do this by right-clicking on a file in Solution Explorer,
      then clicking "Open Command Prompt" (this will open a command prompt with all the proper environment variables)
    - From the Windows Start screen/menu, type `Developer Command Prompt for VS2013`,
      then click/select the result
    - From an *already open* command prompt, run `vcvarsall.bat`
      (e.g., "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat")

	For 64-bit Windows, open *VS2013 x64 Native tool command prompt*.

 2. Change directory (`cd` or `pushd`) to `scintilla\win32\`

 3. Build `SciLexer.dll` with one of the following commands:
    - `nmake NOBOOST=1 -f scintilla.mak`              (normal build)
    - `nmake NOBOOST=1 DEBUG=1 -f scintilla.mak`      (debugging build)

 4. Copy `SciLexer.dll` from `scintilla\bin\` to the same directory as `notepad++.exe`
    - For the `Unicode Release` configuration, the output directory
      (where `notepad++.exe` is located) is `PowerEditor\bin\`
    - For the `Unicode Debug` configuration, the output directory
      (where `notepad++.exe` is located) is `PowerEditor\visual.net\Unicode Debug\`


See the [Notepad++ official site](https://notepad-plus-plus.org/) for additional information.

[Notepad++ Contributors](https://notepad-plus-plus.org/contributors)
