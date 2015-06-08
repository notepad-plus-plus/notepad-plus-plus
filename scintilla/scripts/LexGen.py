#!/usr/bin/env python
# LexGen.py - implemented 2002 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

# Regenerate the Scintilla source files that list all the lexers.
# Should be run whenever a new lexer is added or removed.
# Requires Python 2.5 or later
# Files are regenerated in place with templates stored in comments.
# The format of generation comments is documented in FileGenerator.py.

from FileGenerator import Regenerate, UpdateLineInFile, ReplaceREInFile
import ScintillaData
import HFacer

def UpdateVersionNumbers(sci, root):
    UpdateLineInFile(root + "win32/ScintRes.rc", "#define VERSION_SCINTILLA",
        "#define VERSION_SCINTILLA \"" + sci.versionDotted + "\"")
    UpdateLineInFile(root + "win32/ScintRes.rc", "#define VERSION_WORDS",
        "#define VERSION_WORDS " + sci.versionCommad)
    UpdateLineInFile(root + "qt/ScintillaEditBase/ScintillaEditBase.pro",
        "VERSION =",
        "VERSION = " + sci.versionDotted)
    UpdateLineInFile(root + "qt/ScintillaEdit/ScintillaEdit.pro",
        "VERSION =",
        "VERSION = " + sci.versionDotted)
    UpdateLineInFile(root + "doc/ScintillaDownload.html", "       Release",
        "       Release " + sci.versionDotted)
    ReplaceREInFile(root + "doc/ScintillaDownload.html",
        r"/scintilla/([a-zA-Z]+)\d\d\d",
        r"/scintilla/\g<1>" +  sci.version)
    UpdateLineInFile(root + "doc/index.html",
        '          <font color="#FFCC99" size="3"> Release version',
        '          <font color="#FFCC99" size="3"> Release version ' +\
        sci.versionDotted + '<br />')
    UpdateLineInFile(root + "doc/index.html",
        '           Site last modified',
        '           Site last modified ' + sci.mdyModified + '</font>')
    UpdateLineInFile(root + "doc/ScintillaHistory.html",
        '	Released ',
        '	Released ' + sci.dmyModified + '.')

def RegenerateAll(root):
    
    sci = ScintillaData.ScintillaData(root)

    Regenerate(root + "src/Catalogue.cxx", "//", sci.lexerModules)
    Regenerate(root + "win32/scintilla.mak", "#", sci.lexFiles)

    UpdateVersionNumbers(sci, root)
    
    HFacer.RegenerateAll(root, False)

if __name__=="__main__":
    RegenerateAll("../")
