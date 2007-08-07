/* QuickText - Quick editing tags for Notepad++
    Copyright (C) 2006  João Moreno (alph.pt@gmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

Name: QuickText
Version: 0.2
Link: http://sourceforge.net/projects/quicktext/
Author: João Moreno <alph.pt@gmail.com>

DESCRIPTION

Quicktext is a Notepad++ plugin for quick text substitution, including multi
field inputs. It's similar to Tab Triggers in TextMate.

INSTALLATION

Just copy the QuickText.dll into Notepad++'s plugin directory
(mine is C:\Program Files\Notepad++\plugins), and the QuickText.ini file into
Notepad++'s directory (C:\Program Files\Notepad++).

USAGE

Use the key shortcut CTRL+Enter to use QuickText tags.

CUSTOMIZATION

To make you're own tags:
	- First make sure the tag's Language Section already exists. This is done by
	creating a new section with the code corresponding to the Language.
	(See LANGUAGE CODES).
	- Then, for the key of the tag, use only lower/upper case and numbers.
	- Special chars:
		- $ hotspots
		- \$ for writing actual '$'
		- \n break line.

Or just use the Options GUI. (v0.2) :)

EXAMPLE

 *** (8 is the Language Code for HTML)

[8]  
link=<a href="$">$</a>

LANGUAGE CODES

00 TXT
01 PHP
02 C
03 CPP
04 CS
05 OBJC
06 JAVA
07 RC
08 HTML
09 XML
10 MAKEFILE
11 PASCAL
12 BATCH
13 INI
14 NFO
15 USER
16 ASP
17 SQL
18 VB
19 JS
20 CSS
21 PERL
22 PYTHON
23 LUA
24 TEX
25 FORTRAN
26 BASH
27 FLASH
28 NSIS
29 TCL
30 LISP
31 SCHEME
32 ASM
33 DIFF
34 PROPS
35 PS
36 RUBY
37 SMALLTALK
38 VHDL