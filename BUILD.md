# Building Notepad++

## Microsoft Visual Studio

**Pre-requisites:**

- Microsoft Visual Studio 2022 version 17.5 (C/C++ Compiler, v143 toolset for win32, x64, arm64)

There are three components which are built from one visual studio solution:

- `notepad++.exe`: (contains `libSciLexer.lib`)
- `libScintilla.lib` : static library based on [Scintilla](https://www.scintilla.org/)
- `libLexilla.lib` : static library based on [Lexilla](https://www.scintilla.org/Lexilla.html)

Notepad++ is always built **with** Boost regex PCRE support instead of default c++11 regex ECMAScript used by plain Scintilla.

### Build `notepad++.exe`

 1. Open [`PowerEditor\visual.net\notepadPlus.sln`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/visual.net/notepadPlus.sln)
 2. Select a solution configuration (Debug or Release) and a solution platform (x64 or Win32 or ARM64)
 3. Build Notepad++ solution like a normal Visual Studio project. This will also build the dependent Scintilla and Lexilla projects.

### Build `libScintilla.lib` and `libLexilla.lib`

As mentioned above, you'll need `libScintilla.lib` and `libLexilla.lib` for the Notepad++ build. This is done automatically on building the whole solution. So normally you don't need to care about this.

#### Build `libScintilla.lib` with boost and `libLexilla.lib` via nmake

This is not necessary any more and just here for completeness as this option is still available.
Boost is taken from [boost 1.85.0](https://www.boost.org/users/history/version_1_85_0.html) and stripped down to the project needs available at [boost](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/boostregex/boost) in this repo.

1. Open the Developer Command Prompt for Visual Studio
2. Go into the [`scintilla\win32\`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/scintilla/win32/)
3. Build the same configuration as notepad++:
   - Release: `nmake -f scintilla.mak`
   - Debug: `nmake DEBUG=1 -f scintilla.mak`
   - Example:
   `nmake -f scintilla.mak`
4. Go into the [`lexilla\src\`](https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/lexilla/src/)
5. Build the same configuration as notepad++:
   - Release: `nmake -f lexilla.mak`
   - Debug: `nmake DEBUG=1 -f lexilla.mak`

### History

More about the previous build process: <https://community.notepad-plus-plus.org/topic/13959/building-notepad-with-visual-studio-2015-2017>

Since `Notepad++` version 6.0 - 7.9.5, the build of dynamic linked `SciLexer.dll` that is distributed
uses features from Boost's `Boost.Regex` library.

## GCC

If you have [MinGW-w64](https://www.mingw-w64.org/) installed, you can compile Notepad++ with GCC. Otherwise MinGW-w64 can be downloaded [here](https://sourceforge.net/projects/mingw-w64/files/). You can also download some collection of tools which supports MinGW-w64, like [MSYS2](https://www.msys2.org/) or [WinLibs](https://winlibs.com/).

Building Notepad++ is regularly tested on a Windows system by using [MSYS2](https://www.msys2.org/) project. Current versions of tools used to building (such as GCC, Clang, Make or Bash) can be checked by looking at some logs from the finished building (for example in the [current-build page](https://github.com/notepad-plus-plus/notepad-plus-plus/actions/workflows/CI_build.yml). Other versions may also work but are untested.

**Note:** Before building make sure that the system `PATH` environment variable contains `$MinGW-root$\bin` directory. Otherwise you have to set this directory yourself in Windows settings. You can also use a command like `set PATH=$MinGW-root$\bin;%PATH%` each time `cmd` is launched. But beware that if `PATH` contains several versions of MinGW-w64 GCC, only the first one will be usable.

### Compiling Notepad++ binary

1. Launch `cmd` and add `$MinGW-root$\bin` to `PATH` if necessary.
2. `cd` into `notepad-plus-plus\PowerEditor\gcc`.
3. Run `mingw32-make`.
4. The 32-bit or 64-bit `notepad++.exe` will be generated either in `bin.i686` or in `bin.x86_64` directory respectively, depending on the target CPU of the compiler — look for the full path to the resulting binary at the end of the build process.

#### Some additional information

- The directory containing `notepad++.exe` will also contain everything needed for Notepad++ to start.
- To have a debug build just add `DEBUG=1` to the `mingw32-make` invocation above. The output directory then will be suffixed with `-debug`.
- To see commands being executed add `VERBOSE=1` to the same command.
- When a project is built outside of the `PowerEditor/gcc` directory, for example when using `-f` option, then the entire project path must not contain any spaces. Additionally, the path to `makefile` of this project should be listed as first.
- When a project is built through MinGW-w64 with multilib support, a specific target can be forced by passing `TARGET_CPU` variable with `x86_64` or `i686` as value.
- To use Clang instead of GCC for compilation provide `CXX` variable with `clang++` as value.
- To use [Clang analyzer](https://clang-analyzer.llvm.org/) together with Clang provide `CLANGANALYZE=1` to the `mingw32-make` invocation.
