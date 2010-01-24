What is Notepad++ ?
===================

Notepad++ is a free (as in "free speech" and also as in "free beer") source code editor and Notepad replacement that supports several programming languages and natural languages. Running in the MS Windows environment, its use is governed by GPL License.


To build Notepad++ package from source code:
============================================

For generating the executable file (notepad++.exe), you can use Visual Studio 2005 or MinGW 3.0 / 2.X (a makefile is available, but not maintained).
A CMakeLists.txt (located in the PowerEditor\src directory) is available (but not maintained anymore) for generating the different VC project and MinGW makefile via cmake.

For generating the the dll file (SciLexer.dll), you have 2 choices : VC++ 6 (from v2.5) or MinGW 3.0 / 2.X 

Notepad++ Unicode release binary (notepad++.exe) and Scintilla release binary (SciLexer.dll) will be built in the directories "notepad++\trunk\PowerEditor\bin" and "notepad++\trunk\scintilla\bin" respectively.
You have to copy SciLexer.dll in "notepad++\PowerEditor\bin" in order to launch notepad++.exe


Go to Notepad++ official site for more information :
http://notepad-plus.sourceforge.net/


Notepad++ team
http://sourceforge.net/project/memberlist.php?group_id=95717
