# Building Notepad++ with Microsoft Visual Studio

**Pre-requisites:**

 - Microsoft Visual Studio 2019 (C/C++ Compiler, v142 toolset for win32, x64, arm64)

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

If you have [MinGW-w64](https://www.mingw-w64.org/) installed, you can compile Notepad++ with GCC.

MinGW-w64 can be downloaded [here](https://sourceforge.net/projects/mingw-w64/files/). Building Notepad++ is regularly tested on a Windows system with [x86_64-8.1.0-release-posix-seh-rt_v6-rev0](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z) for building 64-bits binary and with [i686-8.1.0-release-posix-dwarf-rt_v6-rev0](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/dwarf/i686-8.1.0-release-posix-dwarf-rt_v6-rev0.7z) versions for building 32-bits binary. Other versions may also work but are untested.

**Note:** If you use MinGW-w64 GCC from a package (7z), you need to manually add the `$MinGW-root$\bin` directory to the system `PATH` environment variable for the `mingw32-make` invocation below to work. One can use a command like `set PATH=$MinGW-root$\bin;%PATH%` each time `cmd` is launched. But beware that if `PATH` contains several versions of MinGW-w64 GCC, only the first one will be usable.

## Compiling Notepad++ binary

1. Launch `cmd` and add `$MinGW-root$\bin` to `PATH` if necessary.
2. `cd` into `notepad-plus-plus\PowerEditor\gcc`.
3. Run `mingw32-make`.
4. The 32-bit or 64-bit `notepad++.exe` will be generated either in `bin.i686` or in `bin.x86_64` directory respectively, depending on the target CPU of the compiler — look for the full path to the resulting binary at the end of the build process.

### Some additional information:

* The directory containing `notepad++.exe` will also contain everything needed for Notepad++ to start.
* To have a debug build just add `DEBUG=1` to the `mingw32-make` invocation above. The output directory then will be suffixed with `-debug`.
* To see commands being executed add `VERBOSE=1` to the same command.
* When a project is built outside of the `PowerEditor/gcc` directory, for example when using `-f` option, then the entire project path must not contain any spaces. Additionally, the path to `makefile` of this project should be listed as first.
* When a project is built through MinGW-w64 with multilib support, a specific target can be forced by passing `TARGET_CPU` variable with `x86_64` or `i686` as value.
