Getting Notepad++ to work (build)

There are three stages to this:

1) Configuring VS 2015 

2) Building SciLexer.dll

3) Building Notepad++


Configuring VS 2015

It turns out that VS 2015 isn't installed with C++ as default, if this is the case you need to do the following:

1) Open Visual Studio 2015
2) Goto File -> New -> Project
3) Select Visual C++ from the left menu
3a) If C++ is NOT installed you will see two options to install Visual C++ desktop addons
3b) Install these
3c) Otherwise, if they have already been installed close this window

4) Now you need to set the document path variables, click start and type 'Developer Command Prompt' 
5) Right click on this and select Run as Administrator
6) Navigate to the location Visual Studio is installed:
C:\Program Files(x86)\Microsoft Visual Studio 14.0\VC
7) run the batch file:     vcvarsall.bat


Build SciLexer

1) Now navigate to the directory where you installed Notepad++, in here will be scintilla\win32

2) type     nmake NOBOOST=1 -f scintilla.mak
3) This will build the program in the directory ..\bin 


Build Notepad++

1) Open VS2015
2) Click File -> Open -> Project/Solution
3) Navigate to Notepad++ install directory -> PowerEditor -> Visual.net
4) Open the file: notepadPlus.vs2015.vcxproj
5) Pick Unicode Release as the Build Type and pick either x64 or x86
6) Click Build -> Build Solution


This will now build, there maybe errors but it will still built the Notepad++.exe file which you can find in either: PowerEditor\bin or PowerEditor\bin64 

Last thing to do is copy across the contents of the scintilla\bin directory to where notepad++.exe is, and this will run!

Any problems just ask!

John
