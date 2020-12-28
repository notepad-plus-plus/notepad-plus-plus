README for building of Scintilla and SciTE

Scintilla can be built by itself.
To build SciTE, Scintilla must first be built.


*** GTK+/Linux version ***

You must first have GTK+ 2.24 or later and GCC (7.1 or better) installed.
Clang may be used by adding CLANG=1 to the make command line.
Other C++ compilers may work but may require tweaking the make file.
Either GTK+ 2.x or 3.x may be used with 2.x the default and 3.x
chosen with the make argument GTK3=1.

To build Scintilla, use the makefile located in the scintilla/gtk directory
	cd scintilla/gtk
	make
	cd ../..

To build and install SciTE, use the makefile located in the scite/gtk directory
	cd scite/gtk
	make
	sudo make install

This installs SciTE into $prefix/bin. The value of $prefix is determined from
the location of Gnome if it is installed. This is usually /usr if installed
with Linux or /usr/local if built from source. If Gnome is not installed
/usr/bin is used as the prefix. The prefix can be overridden on the command
line like "make prefix=/opt" but the same value should be used for both make
and make install as this location is compiled into the executable. The global
properties file is installed at $prefix/share/scite/SciTEGlobal.properties.
The language specific properties files are also installed into this directory.

To remove SciTE
	sudo make uninstall

To clean the object files which may be needed to change $prefix
	make clean

The current make file only supports static linking between SciTE and Scintilla.


*** Windows version ***

A C++ 17 compiler is required.
Visual Studio 2017 is the development system used for most development
although Mingw-w64 7.1 is also supported.

To build Scintilla, make in the scintilla/win32 directory
		cd scintilla\win32
GCC:		mingw32-make
Visual C++:	nmake -f scintilla.mak
		cd ..\..

To build SciTE, use the makefiles located in the scite/win32 directory
		cd scite\win32
GCC:		mingw32-make
Visual C++: 	nmake -f scite.mak

An executable SciTE will now be in scite/bin.

*** GTK+/Windows version ***

Mingw-w64 is known to work. Other compilers will probably not work.

Only Scintilla will build with GTK+ on Windows. SciTE will not work.

To build Scintilla, make in the scintilla/gtk directory
	cd scintilla\gtk
	mingw32-make

*** macOS Cocoa version ***

Xcode 9.2 or later may be used to build Scintilla on macOS.

There is no open source version of SciTE for macOS but there is a commercial
version available through the App Store.

To build Scintilla, run xcodebuild in the scintilla/cocoa/ScintillaFramework directory
        cd cocoa/ScintillaFramework
	xcodebuild

*** Qt version ***

See the qt/README file to build Scintilla with Qt.
