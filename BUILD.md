# How to build Notepad++ with Microsoft Visual Studio

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

# Building 64-bit binaries with GCC

If you have [MinGW-w64](https://mingw-w64.org/doku.php/start) installed, then you can compile Notepad++ and optionally libscilexer.a 64-bit binaries with GCC.

You can download MinGW-w64 from https://sourceforge.net/projects/mingw-w64/files/. Notepad++ uses [MinGW 8.1 x86_64-8.1.0-release-posix-seh-rt_v6-rev0](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z).

## Compile libscilexer.a (optional)

This step is not necessary anymore as the library will be build in the process of compiling the Notepad++ binary. It is here just for completeness sake as this option is still available.

1. Launch `cmd`.
2. Change dir into `notepad-plus-plus\scintilla\win32`.
3. Type `mingw32-make -j%NUMBER_OF_PROCESSORS%`
4. `libscilexer.a` is generated in `notepad-plus-plus\scintilla\bin\`.

## Compile Notepad++ binary

1. Launch `cmd`.
2. Change dir into `notepad-plus-plus\PowerEditor\gcc`.
3. Type `mingw32-make`
4. `NotepadPP-release.exe` is generated in `notepad-plus-plus\PowerEditor\bin\`.

To have a debug build just add `DEBUG=1` to the `mingw32-make` invocation. The binary will be called `NotepadPP-debug.exe` in this case.

To see commands being executed add `VERBOSE=1` to the same command.

**Note:** if you use MinGW from a package (7z), you need to manually add the `$MinGW-root$\bin` directory to the system `PATH` environment variable for `mingw32-make` invocation to work (one can use a command like `set PATH=%PATH%;$MinGW-root$\bin` each time `cmd` is launched).

# Building 32-bit binaries with GCC

Building a 32-bit binary of Notepad++ is tested with [MinGW 8.1 i686-8.1.0-release-posix-dwarf-rt_v6-rev0](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/i686-8.1.0-release-posix-dwarf-rt_v6-rev0.7z).

All instructions from the ***Building 64-bit binaries with GCC*** section are applicable. However building 32-bit binaries currently requires `SensApi.dll` to be available as `%windir%\System32\SensApi.dll`. If this is the case, add `target=i686` to the `mingw32-make` command line and everything will just work. The requirement exists because the available 32-bit versions of MinGW don't have the linking library for this DLL and it is generated in the build process.

The missing linking library can also be generated manually to fix a MinGW installation permanently by executing the following commands via `cmd` in a writable directory with `$MinGW-root$\bin` added to `PATH`:

```
gendef %windir%\System32\SensApi.dll
dlltool -d SensApi.def -k -l libsensapi.a
del SensApi.def
```

The resulting `libsensapi.a` then can be moved from the current directory to `$MinGW-root$\i686-w64-mingw32\lib`.
