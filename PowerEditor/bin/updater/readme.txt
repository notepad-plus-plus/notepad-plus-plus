What is GUP?
------------

GUP for w32 is a Generic Updater running under Windows environment.
The aim of GUP is to provide a ready to use and configurable updater
which downloads a update package then installs it. By using cURL library
and TinyXml module, GUP is capable to deal with http protocol and process XML data.


Why GUP?
--------

Originally GUP was made for the need of Notepad++ (a generic source code editor under MS Windows).
During its conception, the idea came up in my mind : if it can fit Notepad++, it can fit for any Windows program.
So here it is, with LGPL license to have no (almost not) restriction for integration in any project.



How does it work?
------------------

GUP can be launched by your program or manually. It reads from a xml configuration file
for getting the current version of your program and url where GUP gets update information,
checks the url (with given current version) to get the update package location,
downloads the update package, then run the update package (it should be a msi or an exe) in question.



Who will need it?
-----------------

Being LGPLed, GUP can be integrated in both commercial (or close source) and open source project.
So if you run a commercial or open a source project under MS Windows
and you release your program at regular intervals, then you may need GUP
to notice your users the new update.



What do you need to use it?
---------------------------

A url to provide the update information to your GUP and an another url location
to store your update package, that's it!



How is GUP easy to use?
-----------------------

All you have to do is point GUP to your url update page (by modifying gup.xml), 
then work on your pointed url update page (see getDownLoadUrl.php comes with the release)
to make sure it responds to your GUP with the correct xml data.



To whom should you say "thank you"?
-----------------------------------

Don HO
<don.h@free.fr>