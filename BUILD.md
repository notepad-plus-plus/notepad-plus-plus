How to build Notepad++
----------------------

**Pre-requisites:**

 - Microsoft Visual Studio 2017 (C/C++ Compiler, v141 toolset for win32, x64, arm64)

There are two components which are built from one visual studio solution:

 - `notepad++.exe`: (contains `libSciLexer.lib`)
 - `libSciLexer.lib` : static library based on Scintilla

Notepad++ is always built **with** Boost regex PCRE support instead of default c++11 regex ECMAScript used by plain Scintilla\SciLexer.

## Build `notepad++.exe`:

 1. Open [`PowerEditor\visual.net\notepadPlus.sln`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/visual.net/notepadPlus.sln)
 2. Select a solution configuration (Unicode Debug or Unicode Release) and a solution platform (x64 or Win32 or ARM64)
 3. Build Notepad++ solution like a normal Visual Studio project. This will also build the dependent SciLexer project.

## Build `libSciLexer.lib`:

As mentioned above, you'll need `libSciLexer.lib` for the Notepad++ build. This is done automatically on building the whole solution. So normally you don't need to care about this.

### Build `libSciLexer.lib` with boost via nmake:

This is not necessary any more and just here for completeness as this option is still available.
Boost is taken from [boost 1.76.0](https://www.boost.org/users/history/version_1_76_0.html) and stripped down to the project needs available at [boost](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/boostregex/boost) in this repo.

1. Open the Developer Command Prompt for Visual Studio
2. Go into the `scintilla\win32\`
3. Build the same configuration as notepad++:
   - Release: `nmake -f scintilla.mak`
   - Debug: `nmake DEBUG=1 -f scintilla.mak`
   - Example: 
   `nmake -f scintilla.mak`

## History:
More about the previous build process: https://community.notepad-plus-plus.org/topic/13959/building-notepad-with-visual-studio-2015-2017

Since `Notepad++` version 6.0 - 7.9.5, the build of dynamic linked `SciLexer.dll` that is distributed
uses features from Boost's `Boost.Regex` library.

## Build 64 bits binaries with GCC:

If you have installed [MinGW-w64](https://mingw-w64.org/doku.php/start), then you can compile Notepad++ & libscilexer.a 64 bits binaries with GCC.

* Compile libscilexer.a

1. Launch cmd.
2. Change dir into `notepad-plus-plus\scintilla\win32`.
3. Type `mingw32-make.exe -j%NUMBER_OF_PROCESSORS%`
4. `libscilexer.a` is generated in `notepad-plus-plus\scintilla\bin\`.

* Compile Notepad++ binary

1. Launch cmd.
2. Change dir into `notepad-plus-plus\PowerEditor\gcc`.
3. Type `mingw32-make.exe -j%NUMBER_OF_PROCESSORS%`
4. `NotepadPP.exe` is generated in `notepad-plus-plus\PowerEditor\bin\`.


You can download MinGW-w64 from https://sourceforge.net/projects/mingw-w64/files/. On Notepad++ Github page (this project), the build system use [MinGW 8.1](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z).


Note 1: if you use MinGW from the package (7z), you need manually add the MinGW/bin folder path to system Path variable to make mingw32-make.exe invoke works (or you can use command :`set PATH=%PATH%;C:\xxxx\mingw64\bin` for adding it on each time you launch cmd). 

Note 2: For 32-bit build, https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/sjlj/i686-8.1.0-release-posix-sjlj-rt_v6-rev0.7z could be used. The rest of the instructions are still valid. 
