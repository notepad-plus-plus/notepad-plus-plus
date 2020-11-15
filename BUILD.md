How to build Notepad++
----------------------

There are two components that need to be built separately:

 - `notepad++.exe`: (depends on `SciLexer.dll`)
 - `SciLexer.dll` : (with nmake)

You can build Notepad++ with *or* without Boost - The release build of
Notepad++ is built **with** Boost.

Since `Notepad++` version 6.0, the build of `SciLexer.dll` that is distributed
uses features from Boost's `Boost.Regex` library.

You can build SciLexer.dll without Boost, ie. with its default POSIX regular
expression support instead of boost's PCRE one. This is useful if you would
like to debug Notepad++, but don't have boost.

## Build `notepad++.exe`:

 1. Open [`PowerEditor\visual.net\notepadPlus.vcxproj`](https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/visual.net/notepadPlus.vcxproj)
 2. Build Notepad++ like a normal Visual Studio project.

As mentioned above, you'll need `SciLexer.dll` to run Notepad++. Please check the following sections for building `SciLexer.dll`.
Once `SciLexer.dll`is generated, copy it from `scintilla\bin\` to the same directory as `notepad++.exe`.

## Build `SciLexer.dll` with boost:

Here are the instructions to build SciLexer.dll (for both 32-bit & 64-bit) for Notepad++:

 1. Download the [Boost source code](https://www.boost.org/users/history/version_1_70_0.html).
 2. Unzip boost. In my case, It's unzipped in `C:\sources\boost_1_70_0`
 3. Build regex of boost. With the version 1.70, launch `bootstrap.bat` under the boost root, `b2.exe` will be generated beside of `bootstrap.bat`. For building boost PCRE lib, go into regex build directory by typing `cd C:\sources\boost_1_70_0\libs\regex\build` then launch `C:\sources\boost_1_70_0\b2.exe toolset=msvc link=static threading=multi runtime-link=static address-model=64 release stage`.
 Note that **address-model=64** is optional if you want to build lib in 64 bits. For 32 bits build, just remove **address-model=64** from the command line.
 4. Copy generated static link library (*.lib) from  `C:\sources\boost_1_70_0\bin.v2\libs\regex\build\msvc-14.1\release\address-model-64\link-static\runtime-link-static\threading-multi\libboost_regex-vc141-mt-s-x64-1_70.lib` to `C:\tmp\boostregexLib\x64\`
 5. Go in `scintilla\win32\` then run `nmake BOOSTPATH=your_boost_root_path BOOSTREGEXLIBPATH=your_built_lib_path -f scintilla.mak`. For example `nmake BOOSTPATH=C:\sources\boost_1_70_0\ BOOSTREGEXLIBPATH=C:\tmp\boostregexLib\x64\ -f scintilla.mak`



## Build `SciLexer.dll` *without* boost:

This will work with `notepad++.exe`, however some functionality in Notepad++ will be broken.

To build SciLexer.dll without PCRE support (for both 32-bit & 64-bit):

 1. For 32-bit, open a command prompt *for building* ([a.k.a. the *Developer Command Prompt for VS2017*](https://msdn.microsoft.com/en-us/library/f2ccy3wt.aspx))
    - From the IDE, you can do this by right clicking on a file in Solution Explorer,
      and clicking "Open Command Prompt". This will open up a command prompt with all the proper environment variables.
    - From the Windows Start screen/menu, type `Developer Command Prompt for VS2017`,
      and click/select the result.
    - From an *already open* command prompt, run `vcvarsall.bat`
      (e.g. "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat").

    For 64-bit, open *VS2017 x64 Native tool command prompt*.

 2. Change directory (`cd` or `pushd`) to `scintilla\win32\`

 3. Build `SciLexer.dll` with one of the following commands:
    - `nmake -f scintilla.mak`         (normal build)
    - `nmake DEBUG=1 -f scintilla.mak` (debugging build)


Note: If building the 32-bit and 64-bit versions in the same folder structure, after building one, it may be necessary to do a `nmake -f scintilla.mak clean` before building the other.


## Build 64 bits binaries with GCC:

If you have installed [MinGW-w64](https://mingw-w64.org/doku.php/start), then you can compile Notepad++ & SciLexer.dll 64 bits binaries with GCC.

* Compile Notepad++ binary

1. Launch cmd.
2. Change dir into `notepad-plus-plus\PowerEditor\gcc`.
3. Type `mingw32-make.exe -j%NUMBER_OF_PROCESSORS%`
4. `NotepadPP.exe` is generated in `notepad-plus-plus\PowerEditor\bin\`.

* Compile SciLexer.dll

1. Launch cmd.
2. Change dir into `notepad-plus-plus\scintilla\win32`.
3. Type `mingw32-make.exe -j%NUMBER_OF_PROCESSORS%`
4. `SciLexer.dll` is generated in `notepad-plus-plus\scintilla\bin\`.

You can download MinGW-w64 from https://sourceforge.net/projects/mingw-w64/files/. On Notepad++ Github page (this project), the build system use [MinGW 8.1](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z).


Note 1: if you use MinGW from the package (7z), you need manually add the MinGW/bin folder path to system Path variable to make mingw32-make.exe invoke works (or you can use command :`set PATH=%PATH%;C:\xxxx\mingw64\bin` for adding it on each time you launch cmd). 

Note 2: For 32bit build, https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/8.1.0/threads-posix/sjlj/i686-8.1.0-release-posix-sjlj-rt_v6-rev0.7z could be used. The rest of the instructions are still valid. 
