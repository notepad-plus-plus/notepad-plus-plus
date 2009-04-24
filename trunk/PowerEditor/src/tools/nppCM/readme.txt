
Overview
--------
nppcm.dll is a shell extension component for Notepad++. 
This component is modified from the MIT open source project WSciTEcm
(Context Menu Extension for SciTE), made by Andre Burgaud <andre@burgaud.com>,
to meet the need of Notepad++.
Installing this Context Menu Handler (nppcm.dll) creates a new command
"Edit with Notepad++" in the context menu of Windows Explorer.
You can quickly  open one or several selected files in Windows Explorer: 
right click on the selection and click on the command "Edit with Notepad++".

The manual installation is describe in the following sections.


Installation
------------
1) Copy nppcm.dll in Notepad++ directory.
2) In Notepad++ directory installation, type the command "regsvr32 nppcm.dll".
This will register the dll.

If everything goes well, you should have "Edit with Notepad++" 
when you right click on selected file(s) in Windows Explorer.

Uninstallation
--------------
In Notepad++ directory installation, type the command "regsvr32 /u nppcm.dll".


Unload the dll
--------------
If you try to delete or override the dll file and you get the error "Access is
denied.", the library is already loaded.
There are several options to workaround this issue:

Solution 1:
-  Close all the Windows Explorer instances open on your desktop and copy
nppcm.dll using the command line (Example : "C:/>cp nppcm.dll <npp_directory>").

Solution 2:
- Reboot the computer and delete or override nppcm.dll (with the command line)
before starting Windows Explorer and using the context menu (right-click).

Solution 3:
- Open a command line window
- Type CTRL+ALT+DEL to display the Windows Task Manager, display the  Process tab 
and "kill" the explorer.exe process.
- If your exlorer did not restart automatically, start it manually from the command line window 
(c:/>explorer)
- Delete or override nppcm.dll before using the context menu (Example: "C:/>cp nppcm.dll <npp_directory>").

Build
-----
nppcm.dll is built with Visual C++ 6.0. A Makefile is provided with the
sources: in the source directory, type "nmake". Ensure that all the
environment variables and paths are set correctly. To do so, use the command
file "VCVARS32.BAT" available in the bin directory of Visual C++ installation.

License
-------
Copyright 2002-2003 by Andre Burgaud <andre@burgaud.com>
See license.txt