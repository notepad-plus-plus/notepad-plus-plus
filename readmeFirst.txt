What is Notepad++ ?

Notepad++ is a free source editor with the syntax highlighting. It gives also the extra functionality 
to the general user : colourise the user defined words. You can print your source code in color 
(or whatever you want) if you have a color printer (WYSIWYG).Furthermore, Notepad++ have 
the multi-view feature, that allows user to edit the different document in the same time, 
and even to edit the same document synchronizely in 2 different views. Notepad++ support 
the fully drag and drop : not only you can drop the file from explorer to open it, but also you 
can drag and drop a document from a view to another. With all the functionalities, 
Notepad++ runs as fast as Notepad provided by MS Windows.


Notepad++ source note:

To build this package:

For generating the executable file, you can use VC++ 7 or MinGW 3.0 / 2.X 
For generating the the dll files, you have 2 choices as well : VC++ 6 (from v2.5) or MinGW 3.0 / 2.X 


All the binaries will be builded in the directory notepad++\PowerEditor\bin

Note that the executable file npp.exe builded by MinGW 3.0, for the reason of 
the runtime lib static-link, has almost 160KB more than the one builded by VC++ 7.

There's no remedy for the moment. If you can reduce the exe size by adding/changing
the compiler flag or linker flag, please let me know.

Go to Notepad++ official site for more information :
http://notepad-plus.sourceforge.net/

Don
don.h@free.fr
