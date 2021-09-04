# Building Notepad++ with Microsoft Visual Studio

**Pre-requisites:**

 - Microsoft Visual Studio 2017 (C/C++ Compiler, v141 toolset for win32, x64, arm64)

There are two components which are built from one visual studio solution:

 - `notepad++.exe`: (contains `libSciLexer.lib`)
 - `libSciLexer.lib` : static library based on Scintilla

Notepad++ is always built **with** Boost regex PCRE support instead of default c++11 regex ECMAScript used by plain Scintilla\SciLexer.

## Build `notepad++.exe`:

 1. Open [`PowerEditor\visual.net\notepadPlus.sln`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/visual.net/notepadPlus.sln)
 2. Select a solution configuration (Debug or Release) and a solution platform (x64 or Win32 or ARM64)
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

# Building Notepad++ with GCC

If you have [MinGW-w64](https://mingw-w64.org/doku.php/start) installed, then you can compile Notepad++ with GCC.

MinGW-w64 can be downloaded from [SourceForge](https://sourceforge.net/projects/mingw-w64/files/). Notepad++ is regularly tested with [x86_64-8.1.0-release-posix-seh-rt_v6-rev0](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z) and with [i686-8.1.0-release-posix-dwarf-rt_v6-rev0](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/i686-8.1.0-release-posix-dwarf-rt_v6-rev0.7z). Other versions of compilers may also work but are untested.

**Note:** if you use MinGW-w64 GCC from a package (7z), you need to manually add the `$MinGW-root$\bin` directory to the system `PATH` environment variable for the `mingw32-make` invocation below to work (one can use a command like `set PATH=%PATH%;$MinGW-root$\bin` each time `cmd` is launched).

## Comping Notepad++ binary

1. Launch `cmd` and add `$MinGW-root$\bin` to `PATH` if necessary.
2. `cd` into `notepad-plus-plus\PowerEditor\gcc`.
3. Run `mingw32-make`.
4. The file `notepad++.exe` will be generated in `bin.x86_64` or in `bin.i686` depending on the compiler used. The path to the directory is displayed at the end of the build process.

To have a debug build just add `DEBUG=1` to the `mingw32-make` invocation above. The output directory then will be suffixed with `-debug`.

To see commands being executed add `VERBOSE=1` to the same command.

### Notes on using GCC targeting `i686`

Building the 32-bit binary of Notepad++ currently works only on Windows because `%windir%\system32\SensApi.dll` is required for the build process to succeed — the tested `i686` version of GCC doesn't have a linking library for this system DLL, so it is generated during the build process.
