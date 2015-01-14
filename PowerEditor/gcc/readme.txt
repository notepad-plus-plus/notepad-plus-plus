makefile was renewed after the v6.7.9.2 release.

It was tested with a MinGW-w64 distribution containing
GCC 4.9.2 (natively on Windows 7) and
GCC 5.1.0 (cross-compiled with i686-w64-mingw32 on Arch Linux).

The default make rule should suffice for building Notepad++,
and copying the required .xml config files.
If cross-compiling you may have to pass a custom CROSS_COMPILE
variable depending on your Linux distribution.
The clean rule removes .o/.d and .res and the copied .xml files

For localization support in the editor preferences you have to copy the
PowerEditor/installer/nativeLang/ directory to PowerEditor/bin/ with the new
directory name "localization".
On Linux a symbolic link suffices:
  $ ln -s ../installer/nativeLang localization

For execution with Wine some additional DLLs from the MinGW package have to be
found inside of the PowerEditor/bin/ directory, which can be achieved by using
links again, e.g.:
  $ ln -s /usr/i686-w64-mingw32/bin/libgcc_s_sjlj-1.dll
  $ ln -s /usr/i686-w64-mingw32/bin/libstdc++-6.dll
  $ ln -s /usr/i686-w64-mingw32/bin/libwinpthread-1.dll
