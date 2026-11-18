What is the URL detection test
------------------------------

The URL detection test is designed to check,
whether the clickable detection works as expected or not.

It uses text files as input. These text files contain a pair of
lines for each URL to test. The first line contains the URL, the
second line contains a sequence of characters, which are a mask,
consisting of '0' or '1' characters, describing whether this
position in the line should be detected as URL.

Example:

u http://dot.com u
m 11111111111111 m

As can be seen, the URL is enclosed in "u " and " u", the mask
is enclosed in "m " and " m". The mask line must follow the URL
line immediately. A '1' in the mask line says, that this position
should be detected as URL, a '0' says the opposite.

It is possible, to put arbitrary text between the pairs of test
lines, to comment the test cases. Obviously, none of the comment
lines should start and end with a single 'u' or 'm' character.

The test files can also be regarded as living document about what
is currently detected as URL and what not.



How to conduct the URL detection test
-------------------------------------

Since the URL detection only takes place in the visible part of
the edit windows, the test can only be run from inside an opened
Notepad++ with a visible edit window.

Since the test script is written in Lua, a prerequisite to conduct
the test is, that the Lua plugin is installed. The plugin can be found
at https://github.com/dail8859/LuaScript/releases.

To conduct the test:

1. Open the verifyUrlDetection.lua with Notepad++.
2. Display the Lua console window (Plugins --> LuaScript --> Show Console)
3. Execute the opened script file (Plugins --> LuaScript --> Execute Current File)

The test results will be displayed in the Lua console window.

!!! Please be aware, that there should NO keyboard or mouse or whatever
    input be applied to Notepad++ while this test is running.




How to extend the URL detection test
------------------------------------

It is possible to add test files to the URL detection test
by copying the new files into this directory and include the filenames
in the testFiles list at the beginning of verifyUrlDetection.lua.



Automated version of the URL detection test
-------------------------------------------

An automated version of the test can be conducted by running verifyUrlDetection.ps1.
This is mainly to run the test in Appveyor.
