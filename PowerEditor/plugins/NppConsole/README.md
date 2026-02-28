# Notepad++ Console

## Description

This Notepad++ plugin provides Windows console (Command Prompt, PowerShell) 
in Notepad++.  Original code:

https://sourceforge.net/projects/nppconsole/

The original code has an icon, but it doesn't show on the dockable tab.  
Such a minor annoyance, but fixed in this version.  I'd like to implement 
a tabbed version where multiple console windows can be run in a tabbed 
interface within the single dockable window already provided.

## Compiling

I compiled with MS Visual Studio Community 2017 and this seems to work OK.

For 32-bit:
```
    [x86 Native Tools Command Prompt for VS 2017]
    C:\> set Configuration=Release
    C:\> set Platform=x86
    C:\> msbuild
```

For 64-bit:
```
    [x64 Native Tools Command Prompt for VS 2017]
    C:\> set Configuration=Release
    C:\> set Platform=x64
    C:\> msbuild
```

## Installation

Copy the:

+ 32-bit:  ./bin/NppConsole.dll
+ 64-bit:  ./bin64/NppConsole.dll

to the Notepad++ plugins folder:
  + In N++ <7.6, directly in the plugins/ folder
  + In N++ >=7.6, in a directory called NppConsole in the plugins/ folder (plugins/NppConsole/)
