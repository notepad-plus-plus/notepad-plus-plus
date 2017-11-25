What is WinGup?
---------------

WinGup is a Generic Updater running under Windows environment.
The aim of WinGup is to provide a ready to use and configurable updater
which downloads a update package then installs it. By using cURL library
and TinyXml module, WinGup is capable to deal with http protocol and process XML data.


Why WinGup?
-----------

Originally WinGup was made for the need of Notepad++ (a generic source code editor under MS Windows).
During its conception, the idea came up in my mind: if it can fit Notepad++, it can fit for any Windows program.
So here it is, with LGPL license to have no (almost not) restriction for integration in any project.



How does it work?
-----------------

WinGup can be launched by your program or manually. It reads from a xml configuration file
for getting the current version of your program and url where WinGup gets update information,
checks the url (with given current version) to get the update package location,
downloads the update package, then run the update package (it should be a msi or an exe) in question.



Who will need it?
-----------------

Being LGPLed, WinGup can be integrated in both commercial (or close source) and open source project.
So if you run a commercial or open a source project under MS Windows and you release your program at
regular intervals, then you may need WinGup to notice your users the new update.



What do you need to use it?
---------------------------

A url to provide the update information to your WinGup and an another url location
to store your update package, that's it!



How is WinGup easy to use?
--------------------------

All you have to do is point WinGup to your url update page (by modifying gup.xml), 
then work on your pointed url update page (see getDownLoadUrl.php comes with the release)
to make sure it responds to your WinGup with the correct xml data.



How to build it?
----------------

 0. You have to build cURL before building WinGup:
 ```
    a. Open VS2015 Native Tool Command for 32/64 bits
    b. go to curl winbuild directory:
       cd <your wingup source path>\curl\winbuild
	c. compile cURL by using one of the following commands, according the mode and archetecture of wingup you want to build.
       x64 release: nmake /f Makefile.vc mode=dll vc=14 RTLIBCFG=static MACHINE=x64
       x64 debug: nmake /f Makefile.vc mode=dll vc=14 RTLIBCFG=static DEBUG=yes MACHINE=x64
       x86 release: nmake /f Makefile.vc mode=dll vc=14 RTLIBCFG=static MACHINE=x86
       x86 debug: nmake /f Makefile.vc mode=dll vc=14 RTLIBCFG=static DEBUG=yes MACHINE=x86
```
 1. Open [`vcproj\GUP.vcxproj`](https://github.com/gup4win/wingup/blob/master/vcproj/GUP.vcxproj)
 
 2. Build WinGup [like a normal Visual Studio project](https://msdn.microsoft.com/en-us/library/7s88b19e.aspx) with VS2015



To whom should you say "thank you"?
-----------------------------------

Don HO
<don.h@free.fr>
