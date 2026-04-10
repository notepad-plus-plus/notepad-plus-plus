ComparePlus plugin for Notepad++
-------------------------------

ComparePlus is a plugin for Notepad++ that allows the user to:

-  Compare two files and show differences side by side
-  Compare only parts (selections) of two files
-  Find unique lines between two files
-  Diff a file against Git (with the help of [libgit2](https://github.com/libgit2/libgit2) )
-  Diff a file against SVN (with the help of [sqlite](https://sqlite.org) )
-  Diff a changed file since it was last saved
-  Diff a file or parts of it against clipboard text content

It is highly customizable, can ignore spaces, empty lines, letter cases, regexes, can find moves and show character diffs.
Several compared file pairs can be active and displayed at the same time.


![ComparePlus_screenshot1](https://user-images.githubusercontent.com/6064913/233389533-02125b89-60a8-451f-984b-dc3bfd1f2fc3.png)

Build Status
-------------------------------

- [![Build status](https://ci.appveyor.com/api/projects/status/github/pnedev/comparePlus?svg=true)](https://ci.appveyor.com/project/pnedev/comparePlus)


Installation
-------------------------------

** IMPORTANT NOTE: **
** ComparePlus plugin is available for Notepad++ versions above v8.4.2 (included) **

To install the plugin automatically use the Notepad++ PluginAdmin dialog (find it in the `Plugins` menu in Notepad++ versions above v8.4.5).

To install the plugin manually:

1. Create `ComparePlus` folder in Notepad++'s plugins installation folder (`%Notepad++_program_folder%\Plugins`).
2. Copy the contents of the desired ComparePlus [release](https://github.com/pnedev/comparePlus/releases) zip file
into the newly created folder. Please use the correct archive version based on your Notepad++ architecture - x86, x64 or ARM64.
- ComparePlus.dll : The core plugin DLL.
- `libs` sub-folder : Contains the libs libgit2.dll and sqlite.dll needed for the Diff against Git and SVN commands.
3. Restart Notepad++.


-------------------------------
** IMPORTANT NOTE: **
** This GitHub project is also the home of the latest [source](https://github.com/pnedev/comparePlus/tree/Compare_v2) and [releases](https://github.com/pnedev/comparePlus/releases) of Compare-plugin for Notepad++. ComparePlus is its highly advanced successor and is meant to be its replacement so Compare-plugin will no longer be supported by me **

To install Compare-plugin you can either use the Notepad++ PluginAdmin dialog that will do it automatically
or you can do it manually as described in the following steps based on your Notepad++ version:

v7.6.3 and above:

1. Create `ComparePlugin` folder in Notepad++'s plugins installation folder (`%Notepad++_program_folder%\Plugins`).
2. Copy the contents of the desired Compare-plugin [release](https://github.com/pnedev/comparePlus/releases) zip file
into the newly created folder. Please use the correct archive version based on your Notepad++ architecture - x86 or x64.
- ComparePlugin.dll : The core plugin DLL.
- `ComparePlugin` sub-folder : Contains the libs libgit2.dll and sqlite.dll needed for the Diff against Git and SVN commands.
3. Restart Notepad++.

Pre v7.6.0:

1. Copy the contents of the desired Compare-plugin [release](https://github.com/pnedev/comparePlus/releases) zip file
into Notepad++'s plugins installation folder (`%Notepad++_program_folder%\Plugins`).
Please use the correct archive version based on your Notepad++ architecture - x86 or x64.
- ComparePlugin.dll : The core plugin DLL.
- ComparePlugin sub-folder : Contains the libs libgit2.dll and sqlite.dll needed for the Diff against Git and SVN commands.
2. Restart Notepad++.


Releases and continuous builds
-------------------------------

- [Releases](https://github.com/pnedev/comparePlus/releases)
- [Continuous builds](https://ci.appveyor.com/project/pnedev/comparePlus/history)


Manually building ComparePlus
-------------------------------

 1. Open [`comparePlus\projects\2022\ComparePlus.vcxproj`](https://github.com/pnedev/comparePlus/blob/master/projects/2022/ComparePlus.vcxproj)
 2. Build ComparePlus plugin like a normal Visual Studio project. Available platforms are x86 (Win32) and x64 for Unicode Release and Debug. ARM64 build is also available.
 3. CMake config is available and tested for the generators MinGW Makefiles, Visual Studio and NMake Makefiles


Additional information
-------------------------------

- [Contributors](https://github.com/pnedev/comparePlus/graphs/contributors)
- Check also the official [Notepad++ web site](https://notepad-plus-plus.org/).


Changelog
-------------------------------

See the [`ReleaseNotes.txt`](https://github.com/pnedev/comparePlus/blob/master/ReleaseNotes.txt)
