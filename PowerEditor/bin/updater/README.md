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

Before building WinGup, you have to build curl lib.
Launch your Visual Studio Command Prompt then go to wingup\curl\winbuild, then launch the makefile:

 *cd wingup\curl\winbuild*
 
 *nmake /f Makefile.vc mode=dll*
 
Once curl lib is generated, you can use VS2005 to build your WinGup.



To whom should you say "thank you"?
-----------------------------------

Don HO
<don.h@free.fr>
